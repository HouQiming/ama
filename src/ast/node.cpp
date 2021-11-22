#include <stdlib.h>
#include <unordered_map>
#include <stdio.h>
#include <string>
#include <vector>
#include "../util/jc_array.h"
#include "../../modules/cpp/json/json.h"
#include "../parser/literal.hpp"
#include "../util/mempool.hpp"
#include "node.hpp"

static const int DEBUG_NODE_ALLOCATOR = 0;
namespace ama {
	///////////////////
	//the node pool
	static const intptr_t NODE_BLOCK_SIZE = (intptr_t(1L) << 21) - intptr_t(64L);
	static ama::TMemoryPool g_node_pool{};
	ama::Node* g_free_nodes{};
	static ama::gcstring g_empty_comment = "";
	ama::Node* AllocNode() {
		ama::Node* ret{};
		if ( g_free_nodes ) {
			ret = g_free_nodes;
			g_free_nodes = ret->s;
			memset((void*)(ret), 0, sizeof(ama::Node));
		} else {
			ret = (ama::Node*)(ama::poolAllocAligned(&g_node_pool, sizeof(ama::Node), sizeof(void*), NODE_BLOCK_SIZE));
		}
		assert(!(ret->tmp_flags & ama::TMPF_IS_NODE));
		ret->tmp_flags = ama::TMPF_IS_NODE;
		//non-NULL guarantee for comments
		ret->comments_before = g_empty_comment;
		ret->comments_after = g_empty_comment;
		return ret;
	}
	ama::Node* g_placeholder = AllocNode();
	int isValidNodePointer(ama::Node const* nd_tentative) {
		int in_block = 0;
		for (ama::TBlockHeader const* block = g_node_pool.block; block; block = block->next) {
			uintptr_t ofs = uintptr_t(nd_tentative) - (uintptr_t(block) + sizeof(ama::TBlockHeader));
			if ( ofs < uintptr_t(block->size) && (ofs + uintptr_t(sizeof(ama::Node))) < uintptr_t(block->size) && (ofs % uintptr_t(sizeof(ama::Node))) == 0 ) {
				in_block = 1;
				break;
			}
		}
		return in_block && (nd_tentative->tmp_flags & ama::TMPF_IS_NODE);
	}
	std::vector<ama::Node*> GetAllPossibleNodeRanges() {
		std::vector<ama::Node*> ret{};
		for (ama::TBlockHeader const* block = g_node_pool.block; block; block = block->next) {
			ret.push_back((ama::Node*)(uintptr_t(block) + sizeof(ama::TBlockHeader)));
			if (block == g_node_pool.block) {
				ret.push_back((ama::Node*)g_node_pool.front);
			} else {
				ret.push_back((ama::Node*)(uintptr_t(block) + sizeof(ama::TBlockHeader) + (block->size - sizeof(ama::TBlockHeader)) / sizeof(ama::Node) * sizeof(ama::Node)));
			}
		}
		return std::move(ret);
	}
	///////////////////
	ama::Node* CreateNode(uint8_t node_class, ama::Node* child) {
		ama::Node* ret = ama::AllocNode();
		ret->node_class = node_class;
		ret->c = child;
		ama::Node* nd_last = child;
		for (ama::Node* ndi = child; ndi; ndi = ndi->s) {
			assert(ndi != ret);
			ndi->p = ret;
			nd_last = ndi;
		}
		if ( child ) {
			child->v = ama::PackTailPointer(nd_last);
		}
		return ret;
	}
	ama::Node* CreateNodeFromChildren(uint8_t node_class, std::span<ama::Node*> children) {
		ama::Node* nd = AllocNode();
		nd->node_class = node_class;
		for (intptr_t I = children.size() - intptr_t(1L); I >= intptr_t(0L); I -= intptr_t(1L)) {
			nd->c = ama::cons(children[I], nd->c);
			children[I]->p = nd;
		}
		if ( nd->c ) {
			nd->c->v = ama::PackTailPointer(children.back());
		}
		return nd;
	}
	ama::Node* FixParents(ama::Node* nd_parent, ama::Node* nd) {
		nd->p = nd_parent;
		ama::Node* nd_last{};
		for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
			if ( ndi->s ) { ndi->s->v = ndi; }
			nd_last = ndi;
			FixParents(nd, ndi);
			//post-order subtraction
			//ndi.indent_level -= nd.indent_level;
		}
		if ( nd_last ) {
			nd->c->v = ama::PackTailPointer(nd_last);
		}
		return nd;
	}
	int8_t ClampIndentLevel(intptr_t level) {
		if ( level > ama::MAX_INDENT ) {
			level = ama::MAX_INDENT;
		} else if ( level < -ama::MAX_INDENT ) {
			level = -ama::MAX_INDENT;
		}
		return int8_t(level);
	}
	ama::Node* toSingleNode(ama::Node* nd_child) {
		if ( !nd_child ) {
			return ama::nAir();
		} else if ( nd_child->s ) {
			ama::Node* ret = CreateNode(ama::N_RAW, nd_child);
			if ( nd_child ) {
				ret->indent_level = nd_child->indent_level;
				std::swap(ret->comments_before, nd_child->comments_before);
				std::swap(ret->comments_after, ret->LastChild()->comments_after);
				for (ama::Node* ndi = nd_child; ndi; ndi = ndi->s) {
					ndi->AdjustIndentLevel(-ret->indent_level);
				}
			}
			return ret;
		} else {
			return nd_child;
		}
	}
	ama::Node* UnparseRaw(ama::Node* nd_raw) {
		assert(nd_raw->isRawNode(0, 0));
		ama::Node* nd_ret = nd_raw->ReplaceWith(nd_raw->c);
		nd_raw->c = nullptr;
		nd_raw->FreeASTStorage();
		return nd_ret;
	}
	///////////////////////////////////////
	int ValidateChildRange(ama::Node* p0, ama::Node* p1) {
		for (ama::Node* ndi = p0; ndi; ndi = ndi->s) {
			if ( ndi == p1 ) { return 1; }
			//if( ndi == p0 ) { return 0; }
		}
		return 0;
	}
	//both nd0 and nd1 are inclusive
	void DeleteChildRange(ama::Node* nd0, ama::Node* nd1) {
		assert(ValidateChildRange(nd0, nd1));
		if ( nd1->s ) {
			nd1->s->v = nd0->v;
		} else if ( nd1->p->c != nd0 ) {
			//update tail pointer, here nd0.v is always a valid node since nd1.p.c != nd0
			//the nd1.p.c == nd0 case is handled in the `assert(nd0.p.c == nd0);` scope
			nd1->p->c->v = ama::PackTailPointer(nd0->v);
		}
		if ( ama::isValidPreviousSibling(nd0->v) ) {
			nd0->v->s = nd1->s;
		} else if ( nd0->p ) {
			assert(nd0->p->c == nd0);
			nd0->p->c = nd1->s;
			//update tail pointer
			if ( nd1->s ) {
				nd1->s->v = nd0->v;
			}
		}
		//break links for safety
		for (ama::Node* ndi = nd0; ndi; ndi = ndi->s) {
			ndi->p = nullptr;
			if ( ndi == nd1 ) { break; }
		}
		nd0->v = nullptr;
		nd1->s = nullptr;
	}
	//both nd0 and nd1 are inclusive
	ama::Node* ReplaceChildRange(ama::Node* nd0, ama::Node* nd1, ama::Node* nd_new) {
		if ( !nd_new ) {
			DeleteChildRange(nd0, nd1);
		} else {
			ama::Node* nd_deleted_head{};
			if ( nd0 != nd1 ) {
				nd_deleted_head = nd0->s;
				DeleteChildRange(nd_deleted_head, nd1);
				//during ReplaceWith, comments_after gets moved
				std::swap(nd0->comments_after, nd1->comments_after);
			}
			nd_new = nd0->ReplaceWith(nd_new);
			if ( nd_deleted_head ) {
				ama::cons(nd0, nd_deleted_head);
				std::swap(nd0->comments_after, nd1->comments_after);
			}
		}
		return nd_new;
	}
};
///////////////////
//basic tree operations
void ama::Node::AdjustIndentLevel(intptr_t delta) {
	this->indent_level = ama::ClampIndentLevel(this->indent_level + delta);
	//if( !(this.node_class == ama::N_RAW && (this.flags & 0xffff)) && this.node_class != ama::N_SCOPE ) {
	//	for(ama::Node*+! ndi : this) {
	//		ndi.AdjustIndentLevel(delta);
	//	}
	//}
}
static ama::Node* dfsClone(std::unordered_map<ama::Node const*, ama::Node*>& mapping, ama::Node const* nd) {
	ama::Node* ret = mapping--->get(nd);
	if ( ret ) {
		return ret;
	}
	ret = ama::AllocNode();
	mapping--->set(nd, ret);
	ret->node_class = nd->node_class;
	ret->flags = nd->flags;
	ret->indent_level = nd->indent_level;
	//ret.tmp_flags |= nd.tmp_flags & (SYSF_OPT_OUT_NEWLINE_BEFORE | SYSF_NONJC_UNTOUCHED | SYSF_MULTIVAR_DECL);
	ret->data = nd->data;
	//ret.opaque = nd.opaque;
	ret->comments_before = nd->comments_before;
	ret->comments_after = nd->comments_after;
	//ret.file = nd.file;
	//ret.ofs0 = nd.ofs0;
	//ret.ofs1 = nd.ofs1;
	//ret.type = nd.type;
	//ret.def = nd.def;
	if ( nd->c ) {
		ama::Node* nd_last{};
		for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
			ama::Node* ndi_clone = dfsClone(mapping, ndi);
			ndi_clone->p = ret;
			if ( !nd_last ) {
				ret->c = ndi_clone;
			} else {
				nd_last->s = ndi_clone;
				ndi_clone->v = nd_last;
			}
			nd_last = ndi_clone;
		}
		if ( nd_last ) {
			ret->c->v = ama::PackTailPointer(nd_last);
		}
	}
	return ret;
}
ama::TCloneResult ama::Node::CloneEx() const {
	std::unordered_map<ama::Node const*, ama::Node*> mapping{};
	ama::Node* nd = dfsClone(mapping, this);
	return ama::TCloneResult{.nd = nd, .mapping = std::move(mapping)};
}
ama::Node * ama::Node::Clone() const {
	if ( !intptr_t(this) ) {
		return nullptr;
	}
	std::unordered_map<ama::Node const*, ama::Node*> mapping{};
	return dfsClone(mapping, this);
}
//auto ama::Node::ReplaceLinkToThis(ama::Node*+! this, ama::Node*+! nd_new) {
//	ama::Node*! nd = this;
//	ama::Node*+! nd_prev = this->v;
//	if( !nd_prev ) {
//		//if !this->p, we access null, which is the best we can do
//		assert(this->p.c==this)
//		this->p.c = nd_new;
//		return;
//	}
//	assert(nd_prev.s == this)
//	nd_prev.s = nd_new;
//}
ama::Node * ama::Node::ReplaceWith(ama::Node* nd_new) {
	if ( !nd_new ) {
		this->Unlink();
		return nullptr;
	} else {
		this->Insert(ama::POS_REPLACE, nd_new);
		return nd_new;
	}
}
ama::Node * ama::Node::Unlink() {
	if ( this->s ) {
		//this.s.v=this.v should work whether this.v is a tail pointer or not
		//this.s.MergeCommentsBefore(this);
		this->s->v = this->v;
	} else {
		if ( this->p ) {
			//this.p.comments_after = new char[|]!(this.comments_after + this.p.comments_after);
			//this.comments_after = "";
			if ( this->p->c != this ) {
				//update tail pointer
				this->p->c->v = ama::PackTailPointer(this->v);
			}
		}
	}
	if ( ama::isValidPreviousSibling(this->v) ) {
		//this.v.MergeCommentsAfter(this);
		this->v->s = this->s;
	} else if ( this->p ) {
		//this.p.comments_before = new char[|]!(this.p.comments_before + this.comments_before);
		//this.comments_before = "";
		assert(this->p->c == this);
		this->p->c = this->s;
	}
	//break links for safety
	this->p = nullptr;
	this->s = nullptr;
	this->v = nullptr;
	return this;
}
ama::Node * ama::Node::Insert(int pos, ama::Node* nd_new) {
	assert(this->tmp_flags & ama::TMPF_IS_NODE);
	ama::Node* nd_new_parent = pos == ama::POS_FRONT || pos == ama::POS_BACK ? this : this->p;
	ama::Node* nd_tail = nd_new;
	for (; ;) {
		assert(nd_tail != nd_new_parent);
		nd_tail->p = nd_new_parent;
		//jcc::FillErrorInfo(nd_tail, this);
		if ( !nd_tail->s ) {
			break;
		}
		nd_tail = nd_tail->s;
	}
	if ( pos == ama::POS_REPLACE ) {
		if ( this->comments_before != "" && nd_new->comments_before == "" ) {
			nd_new->comments_before = this->comments_before;
			this->comments_before = "";
		}
		if ( this->comments_after != "" && nd_tail->comments_after == "" ) {
			nd_tail->comments_after = this->comments_after;
			this->comments_after = "";
		}
		for (ama::Node* ndi = nd_new; ndi; ndi = ndi->s) { 
			ndi->indent_level = this->indent_level;
			//ndi->AdjustIndentLevel(this->indent_level);
		}
		this->indent_level = 0;
		//if( nd_new.indent_level != this.indent_level ) {
		//	nd_new.AdjustIndentLevel(this.indent_level - this->indent_level);
		//}
	}
	//if( nd_new_parent ) {
	//	nd_new_parent.tmp_flags &= ~SYSF_NONJC_UNTOUCHED;
	//}
	switch ( pos ) {
		case ama::POS_BEFORE: {
			ama::Node* nd_prev = this->v;
			if ( ama::isValidPreviousSibling(nd_prev) ) {
				assert(nd_prev->s == this);
				nd_prev->s = nd_new;
				nd_new->v = nd_prev;
			} else {
				assert(this->p->c == this);
				this->p->c = nd_new;
				nd_new->v = nd_prev;
			}
			nd_tail->s = this;
			this->v = nd_tail;
			break;
		}
		case ama::POS_AFTER: {
			ama::Node* nd_next = this->s;
			nd_tail->s = nd_next;
			if ( nd_next ) {
				nd_next->v = nd_tail;
			} else if ( this->p ) {
				//insert after last, update tail pointer
				this->p->c->v = ama::PackTailPointer(nd_tail);
			}
			this->s = nd_new;
			nd_new->v = this;
			break;
		}
		case ama::POS_FRONT: {
			nd_tail->s = this->c;
			if ( this->c ) {
				nd_new->v = this->c->v;
				this->c->v = nd_tail;
			} else {
				nd_new->v = ama::PackTailPointer(nd_tail);
			}
			this->c = nd_new;
			break;
		}
		case ama::POS_BACK: {
			//nd_tail.s = NULL;
			if ( !this->c ) {
				this->c = nd_new;
			} else {
				ama::Node* nd_my_tail = ama::UnpackTailPointer(this->c->v);
				nd_my_tail->s = nd_new;
				nd_new->v = nd_my_tail;
			}
			this->c->v = ama::PackTailPointer(nd_tail);
			break;
		}
		case ama::POS_REPLACE: {
			nd_tail->s = this->s;
			if ( this->s ) {
				this->s->v = nd_tail;
			} else if ( this->p && this->p->c != this ) {
				//update parent's last pointer if we weren't the first child
				this->p->c->v = ama::PackTailPointer(nd_tail);
			}
			nd_new->v = this->v;
			if ( ama::isValidPreviousSibling(this->v) ) {
				this->v->s = nd_new;
			} else {
				if ( this->p ) {
					assert(this->p->c == this);
					this->p->c = nd_new;
				}
				if ( this->v && ama::UnpackTailPointer(this->v) == this ) {
					//we were the first AND last node
					nd_new->v = ama::PackTailPointer(nd_tail);
				} else {
					//we were the first node, keep the old last node
					nd_new->v = this->v;
				}
			}
			//break links for safety
			this->p = nullptr;
			this->s = nullptr;
			this->v = nullptr;
			break;
		}
	}
	//if int(nd_new.node_class) == builder.N_DECLARATION && int(nd_new.p.node_class) == builder.N_VIRTUAL_FUNCTION_DECL: {
	//	assert(0);
	//}
	return nd_new;
}
ama::Node * ama::Node::Root() const {
	ama::Node* ndi = (ama::Node*)(this);
	while ( ndi->p ) {
		ndi = ndi->p;
	}
	return ndi;
}
ama::Node * ama::Node::RootStatement() const {
	ama::Node* ndi = (ama::Node*)(this);
	while ( ndi->p && ndi->p->p ) {
		ndi = ndi->p;
	}
	return ndi;
}
int ama::Node::isAncestorOf(ama::Node const* nd_maybe_child) const {
	ama::Node const* nd_this = this;
	for (ama::Node const* ndi = nd_maybe_child; ndi; ndi = ndi->p) {
		if ( ndi == nd_this ) {
			return 1;
		}
	}
	return 0;
}
ama::Node * ama::Node::Owning(int nc) const {
	for (ama::Node* ndi = (ama::Node*)(this); ndi; ndi = ndi->p) {
		if ( ndi->node_class == nc ) {
			return ndi;
		}
	}
	return nullptr;
}
ama::Node * ama::Node::Owner() const {
	//|| int(ndi.node_class) == ama::N_RAW_DECLARATION
	for (ama::Node* ndi = this->p; ndi; ndi = ndi->p) {
		if ( ndi->node_class == ama::N_FUNCTION || ndi->node_class == ama::N_CLASS || !ndi->p ) {
			return ndi;
		}
	}
	return nullptr;
}
//for simppair.jc only
ama::Node * ama::Node::LastChildSP() const {
	ama::Node* ndi = this->c;
	if ( ndi ) {
		while ( ndi->s ) {
			ndi = ndi->s;
		}
		return ndi;
	} else {
		return nullptr;
	}
}
ama::Node * ama::Node::LastChild() const {
	ama::Node* ndi = this->c;
	if ( ndi ) {
		//while( ndi.s ) {
		//	ndi = ndi.s;
		//}
		//return ndi;
		return ama::UnpackTailPointer(ndi->v);
	} else {
		return nullptr;
	}
}
ama::Node * ama::Node::CommonAncestor(ama::Node const* b) const {
	ama::Node* a = (ama::Node*)(this);
	std::vector<ama::Node*> ancestors_a{};
	for (ama::Node* ndi = a; ndi; ndi = ndi->p) {
		ancestors_a--->push(ndi);
	}
	std::vector<ama::Node*> ancestors_b{};
	for (ama::Node* ndi = (ama::Node*)(b); ndi; ndi = ndi->p) {
		ancestors_b--->push(ndi);
	}
	ama::Node* ret{};
	while ( ancestors_a.size() && ancestors_b.size() ) {
		ama::Node* nda = ancestors_a--->pop();
		ama::Node* ndb = ancestors_b--->pop();
		if ( nda != ndb ) {
			break;
		}
		ret = nda;
	}
	return ret;
}
ama::gcstring ama::Node::GetStringValue() {
	ama::Node* nd_this = (ama::Node*)(this);
	assert(nd_this->node_class == ama::N_STRING);
	if ( nd_this->node_class != ama::N_STRING || nd_this->data.empty() || (nd_this->flags & ama::LITERAL_PARSED) ) {
		return nd_this->data;
	} else {
		//process on demand
		assert(int(nd_this->node_class) == ama::N_STRING);
		if ( nd_this->data.size() && nd_this->data[intptr_t(0L)] == '\'' ) {
			nd_this->flags |= ama::STRING_SINGLE_QUOTED;
		}
		nd_this->data = ama::gcstring(ama::ParseJCString(nd_this->data));
		nd_this->flags |= ama::LITERAL_PARSED;
		return nd_this->data;
	}
}
ama::Node * ama::Node::dot(ama::gcstring name) {
	ama::Node* nd_ret = ama::CreateNode(ama::N_DOT, this);
	nd_ret->data = name;
	nd_ret->indent_level = this->indent_level;
	nd_ret->comments_before = this->comments_before;
	this->comments_before = "";
	return nd_ret;
}
static void dfsFreeChildrenStorage(ama::Node* nd) {
	assert(nd->tmp_flags & ama::TMPF_IS_NODE);
	ama::Node* ndi_last{};
	ama::Node* ndi0 = nd->c;
	intptr_t n_steps = intptr_t(0L);
	for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
		if ( n_steps > 0 ) {
			assert(ndi != ndi0);
		}
		ndi_last = ndi;
		dfsFreeChildrenStorage(ndi);
		ndi->tmp_flags &= ~ama::TMPF_IS_NODE;
		ndi->p = nullptr;
		if ( n_steps & 1 ) {
			ndi0 = ndi0->s;
		}
		n_steps += 1;
	}
	if ( DEBUG_NODE_ALLOCATOR ) {
		return;
	}
	if ( ndi_last != nullptr ) {
		ndi_last->s = ama::g_free_nodes;
		ama::g_free_nodes = nd->c;
		nd->c = nullptr;
	}
}
void ama::Node::FreeASTStorage() {
	//do not wipe .data and stuff, we could use them again in a C++ expr
	assert(!this->p);
	dfsFreeChildrenStorage(this);
	if ( DEBUG_NODE_ALLOCATOR ) {
		return;
	}
	this->tmp_flags &= ~ama::TMPF_IS_NODE;
	this->s = ama::g_free_nodes;
	ama::g_free_nodes = this;
}
/////////////////////////
ama::gcstring ama::Node::GetName() const {
	if ( this->node_class == ama::N_FUNCTION ) {
		//COULDDO: re-query the function name
		return this->data;
	} else if ( this->node_class == ama::N_CALL || this->node_class == ama::N_CALL_TEMPLATE || this->node_class == ama::N_CALL_CUDA_KERNEL ) {
		if ( this->c && this->c->node_class != this->node_class && this->c->node_class != ama::N_CALL ) {
			return this->c->GetName();
		} else {
			return "";
		}
	} else if ( this->node_class == ama::N_ASSIGNMENT || this->node_class == ama::N_DEPENDENCY ) /*||int(this.node_class) == ama::N_YIELD || int(this.node_class) == ama::N_NO_INFER_TYPE */ {
		return this->c->GetName();
	} else if (this->node_class == ama::N_CLASS) {
		return this->c->s->GetName();
	} else if (this->node_class == ama::N_STRING) {
		ama::Node* nd_hack = (ama::Node*)(this);
		return nd_hack->GetStringValue();
	} else if (this->node_class == ama::N_RAW) {
		for (ama::Node* ndi = this->c; ndi; ndi = ndi->s) {
			if (ndi->node_class == ama::N_REF && (ndi->flags & ama::REF_DECLARED)) {return ndi->data;}
		}
		return this->data;
	} else {
		return this->data;
	}
}
static ama::Node* PreorderSkipChildren(ama::Node* nd_self, ama::Node* nd_root) {
	for (ama::Node* ndi = nd_self; ndi && ndi != nd_root; ndi = ndi->p) {
		if ( ndi->s ) { return ndi->s; }
	}
	return nullptr;
}

static ama::Node* FindImpl(ama::Node* nd_root, ama::Node* nd_before, int32_t boundary, int node_class, ama::gcstring data, std::vector<ama::Node*>* ret) {
	ama::Node* nd_ret{};
	for (ama::Node* nd = nd_root; nd; nd = nd->PreorderNext(nd_root)) {
		skip_children:
		if ( nd == nd_before ) {
			break;
		}
		int32_t my_boundary = ama::BOUNDARY_ONE_LEVEL;
		if ( int(nd->node_class) == node_class || node_class == ama::N_NONE ) {
			int matched = 1;
			if ( !data.empty() ) {
				matched = nd->GetName() == data;
			}
			//matched
			if ( matched ) {
				if ( ret ) {
					ret->push_back(nd);
					if ( boundary & ama::BOUNDARY_MATCH ) {
						//note: if we stop at a match, we SHOULD stop at the root, unlike the other boundary types
						nd = PreorderSkipChildren(nd, nd_root);
						goto skip_children;
					}
				} else {
					nd_ret = nd;
					break;
				}
			}
		}
		//don't test boundary on root: always allow the root to match
		if ( boundary && nd != nd_root ) {
			//} else if( int(nd.node_class) == ama::N_C_FOR || int(nd.node_class) == ama::N_JC_FOR_IN || int(nd.node_class) == ama::N_DO || int(nd.node_class) == ama::N_WHILE || int(nd.node_class) == ama::N_JC_RANGE_FOR ) {
			//	my_boundary |= ama::BOUNDARY_LOOP;
			//} else if( int(nd.node_class) == ama::N_SWITCH ) {
			//	my_boundary |= ama::BOUNDARY_SWITCH;
			//} else if( int(nd.node_class) == ama::N_MACRO_DECL ) {
			//	my_boundary |= ama::BOUNDARY_MACRO_DECL;
			//} else 
			// && !(nd.p && int(nd.p.node_class) == ama::N_STATIC_IF)
			if ( int(nd->node_class) == ama::N_FUNCTION ) {
				my_boundary |= ama::BOUNDARY_FUNCTION;
			} else if ( int(nd->node_class) == ama::N_CLASS ) {
				my_boundary |= ama::BOUNDARY_CLASS;
			} else if ( int(nd->node_class) == ama::N_NODEOF ) {
				my_boundary |= ama::BOUNDARY_NODEOF;
			} else if (int(nd->node_class) == ama::N_SCOPE) {
				my_boundary |= ama::BOUNDARY_SCOPE;
			}
			//} else if( int(nd.node_class) == ama::N_CALL && nd.p && nd.p.node_class == ama::N_RAW_DECLARATION ) {
			//	my_boundary |= ama::BOUNDARY_PROTOTYPE;
			if ( boundary & my_boundary ) {
				nd = PreorderSkipChildren(nd, nd_root);
				goto skip_children;
			}
		}
	}
	return nd_ret;
}
ama::Node * ama::Node::Find(int node_class, ama::gcstring data) const {
	return FindImpl((ama::Node*)(this), nullptr, 0, node_class, data, nullptr);
}
std::vector<ama::Node*> ama::Node::FindAll(int node_class, ama::gcstring data) const {
	std::vector<ama::Node*> ret{};
	FindImpl((ama::Node*)(this), nullptr, ama::BOUNDARY_DEFAULT, node_class, data, &ret);
	return std::move(ret);
}
std::vector<ama::Node*> ama::Node::FindAllWithin(int32_t boundary, int node_class, ama::gcstring data) const {
	std::vector<ama::Node*> ret{};
	FindImpl((ama::Node*)(this), nullptr, boundary, node_class, data, &ret);
	return std::move(ret);
}
std::vector<ama::Node*> ama::Node::FindAllBefore(ama::Node const* nd_before, int32_t boundary, int node_class, ama::gcstring data) const {
	std::vector<ama::Node*> ret{};
	FindImpl((ama::Node*)(this), (ama::Node*)(nd_before), boundary, node_class, data, &ret);
	return std::move(ret);
}
int ama::Node::isRawNode(char ch_open, char ch_close) const {
	return this->node_class == ama::N_RAW && (this->flags & 0xffff) == (int32_t(ch_open) | int32_t(ch_close) << 8);
}
int ama::Node::isSymbol(std::span<char> name) const {
	return this->node_class == ama::N_SYMBOL && this->data == name;
}
int ama::Node::isRef(std::span<char> name) const {
	return this->node_class == ama::N_REF && this->data == name;
}
int ama::Node::isMethodCall(std::span<char> name) const {
	return this->node_class == ama::N_CALL && this->c->node_class == ama::N_DOT && this->c->data == name;
}
///dependency is global: have to re-ParseDependency after you InsertDependency
ama::Node * ama::Node::InsertDependency(uint32_t flags, ama::gcstring name) {
	for ( ama::Node* const &ndi: this->FindAll(ama::N_DEPENDENCY, name) ) {
		if ( ndi->flags == flags ) { return ndi; }
	}
	return this->Insert(ama::POS_FRONT, ama::nDependency(ama::nString(name))->setFlags(flags));
}
ama::Node * ama::Node::InsertCommentBefore(std::span<char> s) {
	this->comments_before = (ama::gcscat(s, this->comments_before));
	return this;
}
///////////////////////////////////////
ama::Node * ama::Node::MergeCommentsAfter(ama::Node* nd_after) {
	this->comments_after = (this->comments_after + nd_after->comments_before);
	nd_after->comments_before = "";
	return this;
}
ama::Node * ama::Node::MergeCommentsAndIndentAfter(ama::Node* nd_after) {
	std::string ret = JC::string_concat(this->comments_after, nd_after->comments_before);
	if ( nd_after->comments_before--->indexOf('\n') >= 0 && nd_after->indent_level > 0 ) {
		for (intptr_t i = intptr_t(0L); i < intptr_t(nd_after->indent_level); i += 1) {
			ret.push_back(' ');
		}
	}
	this->comments_after = ama::gcstring(ret);
	nd_after->comments_before = "";
	return this;
}
ama::Node * ama::Node::MergeCommentsBefore(ama::Node* nd_before) {
	this->comments_before = (nd_before->comments_after + this->comments_before);
	nd_before->comments_after = "";
	return this;
}
ama::gcstring ama::Node::DestroyForSymbol() {
	ama::gcstring ret = this->data;
	this->p = nullptr;
	this->FreeASTStorage();
	return ret;
}
int ama::Node::ValidateChildCount(int n_children, int quiet) {
	ama::Node* ndi = this->c;
	for (int i = 0; i < n_children; i += 1) {
		if ( !ndi ) {
			if (!quiet) {printf("need %d children but only got %d : %lld\n", n_children, i + 1, ((long long)(intptr_t(this))));}
			this->comments_before = ama::gcstring(JC::string_concat("/*bad children count*/ ", this->comments_before));
			return 0;
		}
		ndi = ndi->s;
	}
	if ( ndi ) {
		if (!quiet) {printf("too many children %lld\n", ((long long)(intptr_t(this))));}
		this->comments_before = ama::gcstring(JC::string_concat("/*too many children*/ ", this->comments_before));
		return 0;
	}
	return 1;
}
int ama::Node::ValidateEx(intptr_t max_depth, int quiet) {
	intptr_t error_count = intptr_t(0L);
	std::unordered_map<ama::Node*, int> g_validate_dedup{};
	g_validate_dedup.clear();
	//note: we CANNOT use PreorderNext here: it depends on p which could be invalid
	//we cannot dfs either: it could have an acyclic long chain that triggers a stack overflow
	std::vector<ama::Node*> stack{};
	stack.push_back(this);
	while ( stack.size() ) {
		ama::Node* nd = stack--->pop();
		if ( g_validate_dedup--->get(nd) ) {
			if ( !quiet ) {
				printf("found cycle to %lld %s\n", ((long long)(intptr_t(nd))), JSON::stringify(nd->data).c_str());
			}
			nd->comments_before = ama::gcstring(JC::string_concat("/*cycle ", JSON::stringify(intptr_t(nd)), "*/ ", nd->comments_before));
			error_count += 1;
			continue;
		}
		intptr_t depth = (nd->p ? g_validate_dedup--->get(nd->p) : 0) + 1;
		g_validate_dedup--->set(nd, depth);
		if ( intptr_t(stack.size()) >= max_depth || depth >= max_depth ) {
			if ( !quiet ) {
				printf("node too deep\n");
			}
			nd->comments_before = ama::gcstring(JC::string_concat("/*too deep*/ ", nd->comments_before));
			error_count += 1;
		}
		if ( nd->s && (nd->tmp_flags & ama::TMPF_IS_NODE) ) {
			//validate sibling next: but don't do so for freed nodes since .s is now the freelist next
			stack.push_back(nd->s);
		}
		//validate children even if the current node is freed:
		//  we're not touching the node pool so its content won't change
		//  we'll terminate as long as it's not a duplicate
		if ( nd->c ) { stack.push_back(nd->c); }
		if ( !(nd->tmp_flags & ama::TMPF_IS_NODE) ) {
			if ( !quiet ) {
				printf("found freed node %lld\n", ((long long)(intptr_t(nd))));
			}
			nd->comments_before = ama::gcstring(JC::string_concat("/*freed ", JSON::stringify(intptr_t(nd)), "*/ ", nd->comments_before));
			error_count += 1;
			//other immediate validations do not make sense for a freed node
			continue;
		}
		if ( nd->p && nd->node_class == ama::N_FILE ) {
			if ( !quiet ) {
				printf("the file node has a parent: %u\n", nd->p->node_class);
			}
			error_count += 1;
		}
		if ( nd->node_class == ama::N_FUNCTION ) {
			if ( !nd->ValidateChildCount(4, quiet) ) {
				error_count += 1;
			}
		}
		if ( nd->node_class == ama::N_CLASS ) {
			if ( !nd->ValidateChildCount(4, quiet) ) {
				error_count += 1;
			}
		}
		ama::Node* nd_last{};
		for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
			if ( ndi->p != nd ) {
				if ( !quiet ) {
					printf("bad parent %lld %u %s under %u\n", ((long long)(intptr_t(ndi))), ndi->node_class, JSON::stringify(ndi->data).c_str(), nd->node_class);
				}
				ndi->comments_before = ama::gcstring(JC::string_concat("/*bad parent ", JSON::stringify(intptr_t(ndi)), "*/ ", ndi->comments_before));
				error_count += 1;
			}
			if ( ndi->s && ndi->s->v != ndi ) {
				if ( !quiet ) {
					printf("bad previous sibling %lld %u %s after %u\n", ((long long)(intptr_t(ndi->s))), ndi->s->node_class, JSON::stringify(ndi->s->data).c_str(), ndi->node_class);
				}
				ndi->s->comments_before = ama::gcstring(JC::string_concat("/*bad previous sibling ", JSON::stringify(intptr_t(ndi->s)), "*/ ", ndi->s->comments_before));
				error_count += 1;
			}
			nd_last = ndi;
		}
		if ( nd_last && nd->c->v != ama::PackTailPointer(nd_last) ) {
			if ( !quiet ) {
				printf("bad tail pointer %lld %u %s %lld\n", ((long long)(intptr_t(nd))), nd->node_class, JSON::stringify(nd->data).c_str(), ((long long)(intptr_t(nd->c->v))));
			}
			for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
				if ( !quiet ) {
					printf("    %lld\n", ((long long)(intptr_t(ndi))));
				}
			}
			nd->comments_before = ama::gcstring(JC::string_concat("/*bad tail pointer ", JSON::stringify(intptr_t(nd)), "*/ ", nd->comments_before));
			error_count += 1;
		}
	}
	if ( !quiet && error_count ) {
		printf("=== AST %lld errors\n", ((long long)(error_count)));
		ama::DumpASTAsJSON(this);
		fflush(stdout);
		printf("=== source code\n");
		fflush(stdout);
		{
			auto const& object0 = this->toSource();
			printf("%.*s\n", int(object0.size()), object0.data());
		}
		assert(0);
	}
	return error_count;
}
void ama::Node::Validate() {
	this->ValidateEx(intptr_t(0x3fffffffL), 0);
}
ama::Node * ama::Node::PreorderNext(ama::Node* nd_root) {
	if ( this->c ) { return this->c; } else { return PreorderSkipChildren(this, nd_root); }
}
ama::Node * ama::Node::PreorderSkip() {
	ama::Node* ndi = this;
	while ( ndi && ndi->c ) {
		ndi = ndi->LastChild();
	}
	return ndi;
}
ama::Node * ama::Node::PostorderFirst() {
	ama::Node* nd = this;
	while ( nd->c ) {
		nd = nd->c;
	}
	return nd;
}
ama::Node * ama::Node::PostorderNext(ama::Node* nd_root) {
	if ( this == nd_root ) {
		return nullptr;
	} else if ( this->s ) {
		return this->s->PostorderFirst();
	} else {
		return this->p;
	}
}
ama::Node * ama::Node::ParentStatement() {
	ama::Node* nd = (ama::Node*)(this);
	while ( nd->p != nullptr && nd->p->node_class != ama::N_SCOPE && nd->p->node_class != ama::N_FILE ) {
		nd = nd->p;
	}
	return nd;
}
ama::Node * ama::Node::Prev()const {
	return ama::isValidPreviousSibling(this->v) ? this->v : nullptr;
}
ama::Node * ama::Node::Next()const {
	return this->s;
}
ama::Node * ama::Node::FirstChild()const {
	return this->c;
}
ama::Node * ama::Node::Parent()const {
	return this->p;
}
ama::Node * ama::Node::BreakSibling() {
	ama::Node* ret = this->s;
	this->s = nullptr;
	if ( ret ) { ret->v = nullptr; }
	//update parent's last child
	if ( this->p ) {
		this->p->c->v = ama::PackTailPointer(this);
		//for consistency
		for (ama::Node* ndi = ret; ndi; ndi = ndi->s) {
			ndi->p = nullptr;
		}
	}
	return ret;
}
ama::Node * ama::Node::BreakChild() {
	ama::Node* ret = this->c;
	this->c = nullptr;
	if ( ret ) { ret->p = nullptr; ret->v = nullptr; }
	return ret;
}
ama::Node * ama::Node::BreakSelf() {
	if ( ama::isValidPreviousSibling(this->v) ) {
		assert(this->v->s == this);
		return this->v->BreakSibling();
	} else if ( this->p && this->p->c == this ) {
		return this->p->BreakChild();
	} else {
		//we're root, we're not attached to anything else
		return this;
	}
}
ama::Node * ama::Node::ReplaceUpto(ama::Node* nd_upto, ama::Node* nd_new) {
	return ama::ReplaceChildRange(this, nd_upto, nd_new);
}
ama::Node * ama::Node::toSingleNode() {
	return ama::toSingleNode(this);
}
uint8_t ama::Node::GetCFGRole() const {
	switch ( this->node_class ) {
		case ama::N_SCOPED_STATEMENT: {
			if ( this->data == "if" || this->data == "switch" ) {
				return ama::CFG_BRANCH;
			} else if ( this->data == "for" || this->data == "while" || this->data == "do" ) {
				return ama::CFG_LOOP;
			} else if ( this->data == "enum" ) {
				return ama::CFG_DECL;
			}
			break;
		}
		case ama::N_KEYWORD_STATEMENT: {
			if ( this->data == "return" || this->data == "throw" || this->data == "goto" || this->data == "break" || this->data == "continue" ) {
				return ama::CFG_JUMP;
			}
			break;
		}
		case ama::N_FUNCTION: case ama::N_CLASS: {
			return ama::CFG_DECL;
		}
		case ama::N_CONDITIONAL: {
			return ama::CFG_BRANCH;
		}
		case ama::N_BINOP: {
			if ( this->data == "&&" || this->data == "||" ) {
				return ama::CFG_BRANCH;
			}
			break;
		}
	}
	return ama::CFG_BASIC;
}
///returns true if whether `nd_child` is reached depends on some aspect of `this`
///result is undefined if `nd_child` is not an immediate child of `this`
int ama::Node::isChildCFGDependent(ama::Node const* nd_child) const {
	uint8_t role = this->GetCFGRole();
	if ( role != ama::CFG_BRANCH && role != ama::CFG_LOOP ) {
		return 0;
	}
	switch ( this->node_class ) {
		case ama::N_SCOPED_STATEMENT: {
			return nd_child->node_class == ama::N_SCOPE;
		}
		case ama::N_CONDITIONAL: {
			return nd_child != this->c;
		}
		case ama::N_BINOP: {
			if ( this->data == "&&" || this->data == "||" ) {
				return nd_child != this->c;
			}
			break;
		}
	}
	return 0;
}

int32_t ama::Node::ComputeLineNumber() const {
	int32_t ret = 0;
	for (ama::Node* ndi = this->Root(); ndi;) {
		for (char ch: ndi->comments_before) {
			if (ch == '\n') {
				ret += 1;
			}
		}
		if (ndi == this) {break;}
		if (ndi->node_class == ama::N_STRING && !(ndi->flags & ama::LITERAL_PARSED)) {
			for (char ch: ndi->data) {
				if (ch == '\n') {
					ret += 1;
				}
			}
		}
		if (ndi->c) {
			ndi = ndi->c;
		} else {
			for (; ndi; ndi = ndi->p) {
				for (char ch: ndi->comments_after) {
					if (ch == '\n') {
						ret += 1;
					}
				}
				if ( ndi->s ) { ndi = ndi->s;break; }
			}
		}
	}
	return ret;
}
