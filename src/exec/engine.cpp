#include "console.hpp"
#include "../../modules/cpp/json/json.h"
#include "../ast/node.hpp"
#include "../script/jsenv.hpp"
#include "../codegen/gen.hpp"
#include "engine.hpp"
#include <stdio.h>
#include <string>
#include <vector>
#include "../util/jc_array.h"
#include "../util/jc_unique_string.h"
#include <unordered_map>
namespace ama {
	ama::ExecSession CreateSession(ama::Node* nd_entry, std::span<ama::Node*> interests) {
		//remove the default entry / interest? that's just `new ExecSession!`
		//the sole query to support is "lookup"
		ama::ExecSession ret{};
		for ( ama::Node* const &nd_interest: interests ) {
			ret.AddInterest(nd_interest);
		}
		ama::ExecNode* ed = ret.CreateNodeGraph(nd_entry).entry;
		ret.entries.push_back(ed);
		return std::move(ret);
	}
};
std::vector<ama::ExecNode*> ama::ExecSession::ComputeReachableSet(std::span<ama::ExecNode*> entries, int32_t dir) {
	std::vector<ama::ExecNode*> Q{};
	std::unordered_map<ama::ExecNode*, intptr_t> inQ{};
	for ( ama::ExecNode * & ed: entries ) {
		Q.push_back(ed);
		inQ--->set(ed, 1);
	}
	for (int qi = 0; qi < Q.size(); qi += 1) {
		if ( dir & ama::REACH_FORWARD ) {
			for (ama::ExecNodeExtraLink* link = Q[qi]->next.more; link; link = link->x) {
				ama::ExecNode* edi = link->target;
				if ( !inQ--->get(edi) ) {
					Q--->push(edi);
					inQ--->set(edi, 1);
				}
			}
		}
		if ( dir & ama::REACH_BACKWARD ) {
			for (ama::ExecNodeExtraLink* link = Q[qi]->prev.more; link; link = link->x) {
				ama::ExecNode* edi = link->target;
				if ( !inQ--->get(edi) ) {
					Q--->push(edi);
					inQ--->set(edi, 1);
				}
			}
		}
	}
	return std::move(Q);
}
intptr_t ama::ExecSession::AddInterest(ama::Node* nd_interest) {
	uint8_t kind = ama::N_REF;
	JC::unique_string name = nd_interest->GetName();
	if ( nd_interest->node_class == ama::N_FUNCTION ) {
		kind = ama::N_FUNCTION;
	} else {
		assert(nd_interest->node_class == ama::N_REF);
		if ( nd_interest->Owner()->node_class == ama::N_CLASS ) {
			kind = ama::N_DOT;
		}
	}
	//Owning(ama::N_SCOPE)
	ama::Node* nd_owner = nd_interest->Owner();
	if ( !nd_owner ) {
		nd_owner = nd_interest->Root();
	}
	//QueryEffects depends on same_name_next being strictly backward
	this->interests--->push(ama::ExecInterest{.kind = kind, .nd_interest = nd_interest, .nd_owner = nd_owner, .same_name_next = this->name_to_interest--->get(name) - intptr_t(1L)});
	this->name_to_interest--->set(name, this->interests.size());
	////////////
	////do we want to keep the temp flags for too long?
	//int changed=0
	//for nd_owner in this.owners
	//	for(ndi=nd_owner;ndi;ndi=ndi.PreorderNext(nd_owner))
	//		if ndi!=nd_owner&&ndi.GetCFGRole()==CFG_DECL:
	//			ndi=ndi.PreorderLastInside()
	//			continue
	//	changed|=this.CheckInterest(ndi)
	//if !changed:return
	////////////
	for ( ama::ExecNode* const &ed: this->ComputeReachableSet(this->entries, ama::REACH_FORWARD) ) {
		if ( ed->flags & ama::ENODEF_FOLDED_CFG ) {
			ama::ExecRange rg_new = this->CreateNodeGraph(ed->nd);
			if ( !(rg_new.entry->flags & ama::ENODEF_FOLDED_CFG) ) {
				this->ReplaceNode(ed, rg_new);
			}
		}
	}
	return this->interests.size() - 1;
}
static void CollectLabels(std::unordered_map<JC::unique_string, ama::Node*>& labels, ama::Node* nd_entry) {
	for (ama::Node* ndi = nd_entry; ndi; ndi = ndi->PreorderNext(nd_entry)) {
		if ( ndi != nd_entry && ndi->GetCFGRole() == ama::CFG_DECL ) {
			ndi = ndi->PreorderLastInside();
			continue;
		}
		if ( ndi->node_class == ama::N_LABELED && ndi->c->node_class == ama::N_REF && !(ndi->p->node_class == ama::N_RAW && (ndi->p->flags & 0xffff)) ) {
			//jump to the label - it gets an ExecNode created, the "target" ndi.c.s doesn't
			labels--->set(ndi->data, ndi);
		}
	}
}
static void CollectComputedLabels(std::vector<ama::Node*>& addressed_labels, std::unordered_map<JC::unique_string, ama::Node*>& labels, ama::Node* nd_entry) {
	for (ama::Node* ndi = nd_entry; ndi; ndi = ndi->PreorderNext(nd_entry)) {
		if ( ndi != nd_entry && ndi->GetCFGRole() == ama::CFG_DECL ) {
			ndi = ndi->PreorderLastInside();
			continue;
		}
		if ( ndi->node_class == ama::N_PREFIX && ndi->data == "&&" && ndi->c->node_class == ama::N_REF ) {
			ama::Node* nd_target = labels--->get(ndi->c->data);
			if ( nd_target ) {
				addressed_labels.push_back(nd_target);
			}
		}
	}
	addressed_labels--->sortby([] (auto nd) -> intptr_t { return intptr_t(nd); });
	addressed_labels--->unique();
}

static const intptr_t EXEC_BLOCK_SIZE = intptr_t(65536L) - intptr_t(64L);
ama::ExecNode * ama::ExecSession::CreateExecNode(uint8_t kind, ama::Node* nd_exec) {
	ama::ExecNode* ret = (ama::ExecNode*)(ama::poolAlloc(&this->pool, sizeof(ama::ExecNode), EXEC_BLOCK_SIZE));
	ret->kind = kind;
	ret->nd = nd_exec;
	return ret;
}
void ama::ExecSession::AddLinkTo(ama::ExecNodeLinks* plinks, ama::ExecNode* ed_target) {
	//if !plinks.fast[0]:
	//	plinks.fast[0]=ed_target
	//	return
	//if !plinks.fast[1]:
	//	plinks.fast[1]=plinks.fast[0]
	//	plinks.fast[0]=ed_target
	//	return
	//ExecNodeExtraLink*+ new_link=poolAlloc(&this.pool, sizeof(ExecNodeExtraLink), EXEC_BLOCK_SIZE);
	//new_link.target=plinks.fast[1]
	//new_link.x=plinks.more
	//plinks.more=new_link
	//plinks.fast[1]=plinks.fast[0]
	//plinks.fast[0]=ed_target
	ama::ExecNodeExtraLink* new_link = (ama::ExecNodeExtraLink*)(ama::poolAlloc(&this->pool, sizeof(ama::ExecNodeExtraLink), EXEC_BLOCK_SIZE));
	new_link->target = ed_target;
	new_link->x = plinks->more;
	plinks->more = new_link;
}
void ama::ExecSession::AddEdge(ama::ExecNode* ed0, ama::ExecNode* ed1) {
	//COULDDO: check for existing edge
	//the caching mechanism shouldn't generate duplicate edges...
	this->AddLinkTo(&ed0->next, ed1);
	this->AddLinkTo(&ed1->prev, ed0);
}
void ama::ExecSession::ReplaceNode(ama::ExecNode* ed_old, ama::ExecRange rg_new) {
	for (ama::ExecNodeExtraLink* link0 = ed_old->prev.more; link0; link0 = link0->x) {
		for (ama::ExecNodeExtraLink* link = link0->target->next.more; link; link = link->x) {
			if ( link->target == ed_old ) {
				link->target = rg_new.entry;
			}
		}
	}
	for (ama::ExecNodeExtraLink* link0 = ed_old->next.more; link0; link0 = link0->x) {
		for (ama::ExecNodeExtraLink* link = link0->target->prev.more; link; link = link->x) {
			if ( link->target == ed_old ) {
				link->target = rg_new.entry;
			}
		}
	}
}

static const uint16_t TMPF_CONTAINS_INTEREST = 1u;
static const uint16_t TMPF_CFG_MATTERS_FOR_INTEREST = 2u;
static const int32_t JSCOPE_PARENT_DECL = 1;
static const int32_t JSCOPE_PARENT_SWITCH = 2;
static const int32_t JSCOPE_PARENT_LOOP = 4;
//for internal use only
struct NodeGraphContext {
	ama::ExecSession* sess{};
	ama::Node* nd_entry{};
	int labels_collected = 0;
	std::unordered_map<JC::unique_string, ama::Node*> labels{};
	std::vector<ama::Node*> addressed_labels{};
	std::unordered_map<ama::Node*, ama::Node*> jump_targets{};
	std::vector<ama::ExecNode*> jump_jobs{};
	ama::Node* QueryJumpTarget(ama::Node* ndi);
	ama::ExecRange dfsCreateNodeGraph(ama::Node* nd);
};
ama::Node * NodeGraphContext::QueryJumpTarget(ama::Node* ndi) {
	assert(ndi->node_class == ama::N_KEYWORD_STATEMENT);
	ama::Node* nd_target = this->jump_targets--->get(ndi);
	if ( nd_target ) { return nd_target; }
	if ( ndi->data == "goto" || ((ndi->data == "break" || ndi->data == "continue") && ndi->c && ndi->c->node_class == ama::N_REF) ) {
		//collect labels lazily if we see goto
		if ( !this->labels_collected ) {
			this->labels_collected = 1;
			CollectLabels(this->labels, this->nd_entry->Owner());
		}
		if ( ndi->c && ndi->c->node_class == ama::N_REF ) {
			nd_target = this->labels--->get(ndi->c->data);
			if ( ndi->data != "goto" && nd_target ) {
				//were labeled break / continue to fail, we try the normal logic
				nd_target = nd_target->Find(ama::N_SCOPE, nullptr);
			}
		} else {
			//computed goto
			//this.uncertainty |= ama::UNCERTAIN_JUMP_TARGET;
			if ( this->labels_collected < 2 ) {
				this->labels_collected = 2;
				CollectComputedLabels(this->addressed_labels, this->labels, this->nd_entry->Owner());
			}
		}
	}
	if ( !nd_target && ndi->data != "goto" ) {
		//break / continue / return: there is no target node
		//we only need common ancestor and target execution node... point to the loop / function scope
		//all jumps can use the _AFTER positioning
		int32_t parent_mode = JSCOPE_PARENT_DECL;
		if ( ndi->data == "return" || ndi->data == "throw" ) {
			//do nothing
		} else {
			parent_mode |= JSCOPE_PARENT_LOOP;
			if ( ndi->data == "break" ) {
				parent_mode |= JSCOPE_PARENT_SWITCH;
			} else {
				assert(ndi->data == "continue");
			}
		}
		for (ama::Node* ndj = ndi; ndj; ndj = ndj->p) {
			if ( ndj->node_class != ama::N_SCOPE ) { continue; }
			//take the outer-most scope if we can't find a correct one
			nd_target = ndj;
			ama::Node* nd_parent = ndj->p;
			if ( nd_parent ) { break; }
			if ( ndj == this->nd_entry ) { break; }
			uint8_t parent_role = nd_parent->GetCFGRole();
			if ( (parent_mode & JSCOPE_PARENT_DECL) && parent_role == ama::CFG_DECL ) { break; }
			if ( (parent_mode & JSCOPE_PARENT_LOOP) && parent_role == ama::CFG_LOOP ) {
				if ( ndi->data == "break" ) {
					nd_target = nd_parent;
				}
				break;
			}
			if ( (parent_mode & JSCOPE_PARENT_SWITCH) && nd_parent->node_class == ama::N_SCOPED_STATEMENT && nd_parent->data == "switch" ) {
				break;
			}
		}
	}
	if ( !nd_target ) {
		//assume nop-jump
		nd_target = ndi;
	}
	this->jump_targets--->set(ndi, nd_target);
	return nd_target;
}
///dfsCreateNodeGraph relies on CreateNodeGraph for temp flag setting
ama::ExecRange NodeGraphContext::dfsCreateNodeGraph(ama::Node* nd) {
	ama::ExecRange rg_canon = this->sess->canonicals[nd].rg_canon;
	if ( rg_canon.entry ) { return rg_canon; }
	uint16_t flags = 0;
	uint8_t role = nd->GetCFGRole();
	//nd_entry could be anything: we may end up calling this expanding some other node
	switch ( role ) {
		default: {
			assert(0);
			break;
		}
		case ama::CFG_DECL: {
			if ( nd == this->nd_entry ) {
				//get into nd_entry no matter what
				ama::Node* nd_scope = nd->Find(ama::N_SCOPE, nullptr);
				if ( nd_scope ) {
					ama::ExecNode* ed_enter = this->sess->CreateExecNode(ama::ENODE_DECL_ENTRY, nd);
					ama::ExecNode* ed_after{};
					ama::ExecNodeCanonicals* p_canon = &this->sess->canonicals[nd];
					ed_after = p_canon->ed_after;
					if ( !ed_after ) {
						ed_after = this->sess->CreateExecNode(ama::ENODE_AFTER, nd);
						p_canon->ed_after = ed_after;
					}
					ama::ExecRange rg_scope = this->dfsCreateNodeGraph(nd_scope);
					this->sess->AddEdge(ed_enter, rg_scope.entry);
					this->sess->AddEdge(rg_scope.exit, ed_after);
					rg_scope.entry = ed_enter;
					rg_scope.exit = ed_after;
					p_canon->rg_canon = rg_scope;
					return rg_scope;
				}
			}
			//otherwise, simply ignore: create a dumb node
			break;
		}
		case ama::CFG_BASIC: {
			if ( nd->node_class == ama::N_SCOPE ) {
				ama::ExecNode* ed_last{};
				ama::ExecNode* ed_first{};
				for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
					ama::ExecRange rgi = this->dfsCreateNodeGraph(ndi);
					if ( ed_last ) {
						if ( ed_last->kind != ama::ENODE_JUMP ) {
							this->sess->AddEdge(ed_last, rgi.entry);
						}
					} else {
						ed_first = rgi.entry;
					}
					ed_last = rgi.exit;
				}
				//create the end-of-scope node
				ama::ExecNode* ed_after{};
				ama::ExecNodeCanonicals* p_canon = &this->sess->canonicals[nd];
				ed_after = p_canon->ed_after;
				if ( !ed_after ) {
					ed_after = this->sess->CreateExecNode(ama::ENODE_AFTER, nd);
					p_canon->ed_after = ed_after;
				}
				if ( ed_last ) {
					if ( ed_last->kind != ama::ENODE_JUMP ) {
						this->sess->AddEdge(ed_last, ed_after);
					}
				} else {
					ed_first = ed_after;
				}
				p_canon->rg_canon = ama::ExecRange{.entry = ed_first, .exit = ed_after};
				return ama::ExecRange{.entry = ed_first, .exit = ed_after};
			} else if ( nd->node_class == ama::N_SEMICOLON ) {
				return this->dfsCreateNodeGraph(nd->c);
			} else if ( nd->tmp_flags & TMPF_CFG_MATTERS_FOR_INTEREST ) {
				//a basic node could contain a branch inside: a=(foo&&bar?baz():0)
				//thus the effects of a node should exclude its contained branches
				std::vector<ama::Node*> non_basic_children{};
				for (ama::Node* ndi = nd; ndi; ndi = ndi->PreorderNext(nd)) {
					uint8_t role_i = ndi->GetCFGRole();
					if ( role_i != ama::CFG_BASIC ) {
						non_basic_children.push_back(ndi);
						ndi = ndi->PreorderLastInside();
						continue;
					}
				}
				if ( non_basic_children.size() > 1 ) {
					//use fork / join to represent non-deterministic order
					ama::ExecNode* ed_fork = this->sess->CreateExecNode(ama::ENODE_FORK, nd);
					ama::ExecNode* ed_join = this->sess->CreateExecNode(ama::ENODE_JOIN, nd);
					for ( ama::Node* const &ndi: non_basic_children ) {
						ama::ExecRange rgi = this->dfsCreateNodeGraph(ndi);
						this->sess->AddEdge(ed_fork, rgi.entry);
						if ( rgi.exit->kind != ama::ENODE_JUMP ) {
							this->sess->AddEdge(rgi.exit, ed_join);
						}
					}
					ama::ExecNode* ed_self = this->sess->CreateExecNode(ama::ENODE_NORMAL, nd);
					this->sess->AddEdge(ed_join, ed_self);
					this->sess->canonicals[nd].rg_canon = ama::ExecRange{.entry = ed_fork, .exit = ed_self};
					return ama::ExecRange{.entry = ed_fork, .exit = ed_self};
				} else if ( non_basic_children.size() == 1 ) {
					//sess one is easy: just put non_basic_children[0] before nd
					ama::ExecRange rgi = this->dfsCreateNodeGraph(non_basic_children[0]);
					ama::ExecNode* ed_self = this->sess->CreateExecNode(ama::ENODE_NORMAL, nd);
					if ( rgi.exit->kind != ama::ENODE_JUMP ) {
						this->sess->AddEdge(rgi.exit, ed_self);
					}
					rgi.exit = ed_self;
					this->sess->canonicals[nd].rg_canon = rgi;
					return rgi;
				}
			}
			//otherwise, create a dumb node
			break;
		}
		case ama::CFG_JUMP: {
			ama::ExecNode* ed = this->sess->CreateExecNode(ama::ENODE_JUMP, nd);
			this->jump_jobs--->push(ed);
			this->sess->canonicals[nd].rg_canon = ama::ExecRange{.entry = ed, .exit = ed};
			return ama::ExecRange{.entry = ed, .exit = ed};
		}
		case ama::CFG_BRANCH: {
			if ( !(nd->tmp_flags & TMPF_CFG_MATTERS_FOR_INTEREST) ) {
				flags = ama::ENODEF_FOLDED_CFG;
				break;
			}
			ama::ExecNode* ed = this->sess->CreateExecNode(ama::ENODE_BRANCH, nd);
			ama::ExecNode* ed_phi = this->sess->CreateExecNode(ama::ENODE_PHI, nd);
			if ( nd->node_class == ama::N_SCOPED_STATEMENT ) {
				int has_else = 0;
				if ( nd->data == "if" ) {
					ama::ExecRange rg_yes = this->dfsCreateNodeGraph(nd->c->s);
					this->sess->AddEdge(ed, rg_yes.entry);
					this->sess->AddEdge(rg_yes.exit, ed_phi);
					for (ama::Node* nd_maybe_else = nd->c->s->s; nd_maybe_else; nd_maybe_else = nd_maybe_else->s) {
						if ( nd_maybe_else && nd_maybe_else->node_class == ama::N_EXTENSION_CLAUSE && (nd_maybe_else->data == "else" || nd_maybe_else->data == "elif") ) {
							ama::Node* nd_scope = nd_maybe_else->Find(ama::N_SCOPE, nullptr);
							if ( nd_scope ) {
								has_else = 1;
								ama::ExecRange rg_else = this->dfsCreateNodeGraph(nd_scope);
								this->sess->AddEdge(ed, rg_else.entry);
								this->sess->AddEdge(rg_else.exit, ed_phi);
							}
						}
					}
				} else if ( nd->data == "switch" ) {
					//populate the cache first
					ama::Node* nd_scope = nd->c->s;
					ama::ExecRange rg_inside = this->dfsCreateNodeGraph(nd_scope);
					for (ama::Node* ndi = nd_scope; ndi; ndi = ndi->PreorderNext(nd_scope)) {
						//search for cases / default: they could be inside arbitrary language constructs
						if ( ndi->GetCFGRole() == ama::CFG_DECL || (ndi->node_class == ama::N_SCOPED_STATEMENT && ndi->data == "switch") ) {
							//except another switch or a declaration
							ndi = ndi->PreorderLastInside();
							continue;
						}
						if ( ndi->node_class == ama::N_LABELED && ((ndi->c->node_class == ama::N_RAW && ndi->c->c && ndi->c->c->isRef("case")) || ndi->c->isRef("default")) ) {
							//should be cached
							ama::ExecRange rg_case = this->dfsCreateNodeGraph(ndi);
							this->sess->AddEdge(ed, rg_case.entry);
							this->sess->AddEdge(rg_case.exit, ed_phi);
							if ( ndi->c->isRef("default") ) {
								has_else = 1;
							}
						}
					}
				} else {
					fprintf(stderr, "panic: unrecognized branch statement:\n");
					{
						auto const& object0 = nd->toSource();
						fprintf(stderr, "%.*s\n", int(object0.size()), object0.data());
					}
					abort();
				}
				if ( !has_else ) {
					//we could end up not-executing any of the included scopes
					this->sess->AddEdge(ed, ed_phi);
				}
			} else if ( nd->node_class == ama::N_CONDITIONAL ) {
				ama::ExecRange rg_yes = this->dfsCreateNodeGraph(nd->c->s);
				ama::ExecRange rg_no = this->dfsCreateNodeGraph(nd->c->s->s);
				this->sess->AddEdge(ed, rg_yes.entry);
				this->sess->AddEdge(rg_yes.exit, ed_phi);
				this->sess->AddEdge(ed, rg_no.entry);
				this->sess->AddEdge(rg_no.exit, ed_phi);
			} else if ( nd->node_class == ama::N_BINOP ) {
				//&& ||
				ama::ExecRange rg_yes = this->dfsCreateNodeGraph(nd->c->s);
				this->sess->AddEdge(ed, rg_yes.entry);
				this->sess->AddEdge(rg_yes.exit, ed_phi);
				this->sess->AddEdge(ed, ed_phi);
			} else {
				fprintf(stderr, "panic: unrecognized branch statement class:\n");
				{
					auto const& object0 = nd->toSource();
					fprintf(stderr, "%.*s\n", int(object0.size()), object0.data());
				}
				abort();
			}
			this->sess->canonicals[nd].rg_canon = ama::ExecRange{.entry = ed, .exit = ed_phi};
			return ama::ExecRange{.entry = ed, .exit = ed_phi};
		}
		case ama::CFG_LOOP: {
			if ( !(nd->tmp_flags & TMPF_CFG_MATTERS_FOR_INTEREST) ) {
				flags = ama::ENODEF_FOLDED_CFG;
				break;
			}
			assert(nd->node_class == ama::N_SCOPED_STATEMENT);
			ama::ExecNode* ed = this->sess->CreateExecNode(ama::ENODE_LOOP, nd);
			ama::ExecNode* ed_after{};
			ama::ExecNodeCanonicals* p_canon = &this->sess->canonicals[nd];
			ed_after = p_canon->ed_after;
			if ( !ed_after ) {
				//we HAVE data divergence here, so the kind is PHI instead of AFTER
				//we are also a goto target, so set ed_after
				ed_after = this->sess->CreateExecNode(ama::ENODE_PHI, nd);
				p_canon->ed_after = ed_after;
			}
			/////////////
			ama::Node* nd_scope = nd->c->s;
			ama::ExecRange rg_scope = this->dfsCreateNodeGraph(nd_scope);
			this->sess->AddEdge(ed, rg_scope.entry);
			if ( nd->data != "do" ) {
				//could skip
				this->sess->AddEdge(ed, ed_after);
			}
			//rg_scope.exit should be the N_SCOPE's ed_after
			this->sess->AddEdge(rg_scope.exit, rg_scope.entry);
			this->sess->AddEdge(rg_scope.exit, ed_after);
			p_canon->rg_canon = ama::ExecRange{.entry = ed, .exit = ed_after};
			return ama::ExecRange{.entry = ed, .exit = ed_after};
		}
	}
	//fall back path: create a normal node
	ama::ExecNode* ed = this->sess->CreateExecNode(ama::ENODE_NORMAL, nd);
	ed->flags = flags;
	this->sess->canonicals[nd].rg_canon = ama::ExecRange{.entry = ed, .exit = ed};
	return ama::ExecRange{.entry = ed, .exit = ed};
}
int ama::ExecSession::MatchInterest(ama::Node* ndi, intptr_t i) const {
	if ( !(ndi->node_class == ama::N_REF || ndi->node_class == ama::N_DOT) ) { return 0; }
	int couldbe_interest = 0;
	if ( this->interests[i].kind == ama::N_REF && ndi->node_class == ama::N_REF && this->interests[i].nd_owner->isAncestorOf(ndi) ) {
		couldbe_interest = 1;
	} else if ( this->interests[i].kind == ama::N_DOT && ndi->node_class == ama::N_DOT ) {
		couldbe_interest = 1;
	} else if ( this->interests[i].kind == ama::N_FUNCTION && ndi->p && ndi == ndi->p->c && ndi->p->node_class == ama::N_CALL ) {
		//COULDDO: improve this
		couldbe_interest = 1;
	}
	return couldbe_interest;
}
int ama::ExecSession::CheckInterest(ama::Node* ndi, ama::Node* nd_entry) {
	int ret = 0;
	if ( ndi->node_class == ama::N_REF || ndi->node_class == ama::N_DOT ) {
		intptr_t first = this->name_to_interest--->get(ndi->data) - intptr_t(1L);
		for (intptr_t i = first; i >= intptr_t(0L); i = this->interests[i].same_name_next) {
			//COULDDO: more accurate name resolution for dot / function
			//we can always rule out an effect later as impossible
			if ( this->MatchInterest(ndi, i) ) {
				//we got an interest, mark parents as to-expand
				uint16_t new_flags = TMPF_CONTAINS_INTEREST;
				ama::Node* ndj_last{};
				for (ama::Node* ndj = ndi; ndj; ndj = ndj->p) {
					if ( ndj_last && ndj->isChildCFGDependent(ndj_last) ) {
						new_flags |= TMPF_CFG_MATTERS_FOR_INTEREST;
					}
					if ( (ndj->tmp_flags & new_flags) == new_flags ) { break; }
					ndj->tmp_flags |= new_flags;
					ret = 1;
					if ( ndj == nd_entry ) { break; }
					ndj_last = ndj;
				}
			}
		}
	}
	return ret;
}
ama::ExecRange ama::ExecSession::CreateNodeGraph(ama::Node* nd_entry) {
	//put the owner in session
	ama::Node* nd_owner = nd_entry->Owner();
	if ( !this->canonicals[nd_owner].in_session ) {
		this->canonicals[nd_owner].in_session = 1;
		this->owners.push_back(nd_owner);
	}
	NodeGraphContext ngctx{.sess = this, .nd_entry = nd_entry, .labels_collected = 0};
	for (ama::Node* ndi = nd_entry; ndi; ndi = ndi->PreorderNext(nd_entry)) {
		if ( ndi != nd_entry && ndi->GetCFGRole() == ama::CFG_DECL ) {
			ndi = ndi->PreorderLastInside();
			continue;
		}
		//preorder means this initialization happens before we flag a parent as to-expand
		ndi->tmp_flags &= ~(TMPF_CONTAINS_INTEREST | TMPF_CFG_MATTERS_FOR_INTEREST);
		//if we see an interest, we need to expand parents
		this->CheckInterest(ndi, nd_entry);
		if ( ndi->GetCFGRole() == ama::CFG_JUMP ) {
			//resolve jump target
			//which may not always work, but we can mark unknown jumps as always interesting
			ngctx.QueryJumpTarget(ndi);
		}
	}
	//propagate TMPF_CFG_MATTERS_FOR_INTEREST to JUMPs crossing them: common ancestor test
	for (; ;) {
		int jump_mattered = 0;
		for ( auto&& pair_nd_source_nd_target: ngctx.jump_targets ) {
			ama::Node* nd_source = pair_nd_source_nd_target.first;
			ama::Node * & nd_target = pair_nd_source_nd_target.second;
			ama::Node const* nd_jump_range = nd_source->CommonAncestor(nd_target);
			if ( nd_entry->isAncestorOf(nd_jump_range) && (nd_jump_range->tmp_flags & TMPF_CFG_MATTERS_FOR_INTEREST) ) {
				//we got a cross-interest jump, mark both side's parents as to-expand
				uint16_t new_flags = TMPF_CONTAINS_INTEREST | TMPF_CFG_MATTERS_FOR_INTEREST;
				ama::Node* ndj_last{};
				for (ama::Node* ndj = nd_source; ndj; ndj = ndj->p) {
					if ( (ndj->tmp_flags & new_flags) == new_flags ) { break; }
					ndj->tmp_flags |= new_flags;
					jump_mattered = 1;
					if ( ndj == nd_entry ) { break; }
					ndj_last = ndj;
				}
				for (ama::Node* ndj = nd_target; ndj; ndj = ndj->p) {
					if ( (ndj->tmp_flags & new_flags) == new_flags ) { break; }
					ndj->tmp_flags |= new_flags;
					jump_mattered = 1;
					if ( ndj == nd_entry ) { break; }
					ndj_last = ndj;
				}
			}
		}
		if ( !jump_mattered ) { break; }
	}
	//create graph for the expanded nodes
	//we only expand CFG nodes if they have TMPF_CFG_MATTERS_FOR_INTEREST: merely using an interest in the condition is no different than a basic block
	ama::ExecRange rg = ngctx.dfsCreateNodeGraph(nd_entry);
	//jump edges have to be filled after we populate the cache
	for ( ama::ExecNode* const &ed: ngctx.jump_jobs ) {
		ama::Node* nd_jump = ed->nd;
		ama::Node* nd_target = ngctx.QueryJumpTarget(nd_jump);
		if ( nd_jump->data == "goto" && nd_jump->c && nd_jump->c->node_class != ama::N_REF ) {
			//the whole addressed_labels for computed goto
			for ( ama::Node* const &nd_addressed_label: ngctx.addressed_labels ) {
				ama::ExecRange rgi = this->canonicals[nd_addressed_label].rg_canon;
				if ( rgi.entry ) {
					this->AddEdge(ed, rgi.entry);
				}
			}
		} else if ( nd_target ) {
			if ( nd_jump->data == "goto" ) {
				ama::ExecRange rgi = this->canonicals[nd_target].rg_canon;
				if ( rgi.entry ) {
					this->AddEdge(ed, rgi.entry);
				}
			} else {
				ama::ExecNode* ed_after = this->canonicals[nd_target].ed_after;
				if ( ed_after ) {
					this->AddEdge(ed, ed_after);
				}
			}
		}
	}
	return rg;
}
ama::ExecRange ama::ExecSession::LocateNode(ama::Node* nd) {
	ama::ExecRange rgi{};
	for (ama::Node* ndi = nd; ndi; ndi = ndi->p) {
		rgi = this->canonicals[ndi].rg_canon;
		if ( rgi.entry ) { break; }
	}
	return std::move(rgi);
}
ama::ExecEffect * ama::ExecSession::QueryEffects(ama::ExecNode* ed) {
	ama::ExecNodeCanonicals* p_canon = &this->canonicals[ed->nd];
	ama::ExecEffectList* pef = ed->kind == ama::ENODE_PHI || ed->kind == ama::ENODE_AFTER ? &p_canon->effects_after : &p_canon->effects;
	if ( pef->last_interest_checked < this->interests.size() ) {
		ama::Node* nd_activation = ed->nd;
		switch ( ed->kind ) {
			case ama::ENODE_NORMAL: case ama::ENODE_FORK: case ama::ENODE_BRANCH: case ama::ENODE_LOOP: default: {
				//check newly-added interests
				for (ama::Node* ndi = nd_activation; ndi; ndi = ndi->PreorderNext(nd_activation)) {
					if ( ndi != nd_activation && (ndi->GetCFGRole() == ama::CFG_DECL || this->canonicals[ndi].rg_canon.entry) ) {
						ndi = ndi->PreorderLastInside();
						continue;
					}
					if ( ndi->node_class == ama::N_REF || ndi->node_class == ama::N_DOT ) {
						intptr_t first = this->name_to_interest--->get(ndi->data) - intptr_t(1L);
						for (intptr_t i = first; i >= pef->last_interest_checked; i = this->interests[i].same_name_next) {
							if ( this->MatchInterest(ndi, i) ) {
								//new effect
								ama::ExecEffect* new_effect = (ama::ExecEffect*)(ama::poolAlloc(&this->pool, sizeof(ama::ExecEffect), EXEC_BLOCK_SIZE));
								new_effect->interest_id = i;
								new_effect->nd_alleged_ref = ndi;
								new_effect->x = pef->first;
								pef->first = new_effect;
							}
						}
					}
				}
				break;
			}
			case ama::ENODE_PHI: {
				//this is nd-cached! we must avoid ed ops! scan the statement instead
				//auto eds_stmt=this.ComputeStatementRangeBackward(ed)
				//uint8_t[+]! affected = new uint8_t[+]!(this.interests.length - pef.last_interest_checked);
				for (ama::Node* ndi = nd_activation; ndi; ndi = ndi->PreorderNext(nd_activation)) {
					//we CANNOT skip already-cached, non-decl children here: they're exactly what we phi!
					if ( ndi != nd_activation && ndi->GetCFGRole() == ama::CFG_DECL ) {
						ndi = ndi->PreorderLastInside();
						continue;
					}
					//COULDDO: recurse
					//if ndi!=nd_activation&&this.canonicals[ndi].rg_canon.entry:
					if ( ndi->node_class == ama::N_REF || ndi->node_class == ama::N_DOT ) {
						//console.log('lol',ndi.data)
						intptr_t first = this->name_to_interest--->get(ndi->data) - intptr_t(1L);
						for (intptr_t i = first; i >= pef->last_interest_checked; i = this->interests[i].same_name_next) {
							if ( this->MatchInterest(ndi, i) ) {
								//affected[i - pef.last_interest_checked] = 1;
								//include all possible effects to avoid interpretation
								ama::ExecEffect* new_effect = (ama::ExecEffect*)(ama::poolAlloc(&this->pool, sizeof(ama::ExecEffect), EXEC_BLOCK_SIZE));
								new_effect->interest_id = pef->last_interest_checked + i;
								new_effect->nd_alleged_ref = ndi;
								new_effect->x = pef->first;
								pef->first = new_effect;
							}
						}
					}
				}
				//include each interest at most once
				//for(int! i = 0; i < affected.length; i += 1) {
				//	if( affected[i] ) {
				//	}
				//}
				break;
			}
			case ama::ENODE_AFTER: {
				//destruction effects: declaration search
				for (ama::Node* ndi = nd_activation; ndi; ndi = ndi->PreorderNext(nd_activation)) {
					if ( ndi != nd_activation && (ndi->GetCFGRole() == ama::CFG_DECL || this->canonicals[ndi].rg_canon.entry) ) {
						ndi = ndi->PreorderLastInside();
						continue;
					}
					if ( ndi->node_class == ama::N_REF && (ndi->flags & ama::REF_WRITTEN) ) {
						intptr_t first = this->name_to_interest--->get(ndi->data) - intptr_t(1L);
						for (intptr_t i = first; i >= pef->last_interest_checked; i = this->interests[i].same_name_next) {
							if ( this->MatchInterest(ndi, i) ) {
								//new effect
								ama::ExecEffect* new_effect = (ama::ExecEffect*)(ama::poolAlloc(&this->pool, sizeof(ama::ExecEffect), EXEC_BLOCK_SIZE));
								new_effect->interest_id = i;
								new_effect->nd_alleged_ref = ndi;
								new_effect->x = pef->first;
								pef->first = new_effect;
							}
						}
					}
				}
				break;
			}
			case ama::ENODE_DECL_ENTRY: case ama::ENODE_JUMP: case ama::ENODE_JOIN: {
				//nothing to do
				break;
			}
		}
		pef->last_interest_checked = this->interests.size();
	}
	return pef->first;
}
ama::ExecNode * ama::ExecSession::CloneExecNode(ama::ExecNode* edi) {
	ama::ExecNode* ret = this->CreateExecNode(edi->kind, edi->nd);
	ret->flags = edi->flags;
	return ret;
}
ama::ExecNode * ama::ExecSession::CreateValueGraphBackward(
ama::ExecNode* ed_exit, intptr_t interest_id,
FValueFilter filter) {
	//when filter fails, we need to if-converge or propagate the whole set backward
	//let the filter check convergence even when it doesn't have an interesting effect?
	//convergence could be arbitrary... and there isn't a well-defined order for set backward
	//compute edges separately: per-starting-node reachability set
	std::vector<ama::ExecNode*> subgraph{};
	std::vector<ama::ExecNode*> Q{};
	std::unordered_map<ama::ExecNode*, ama::ExecNode*> to_cloned{};
	ama::ExecNode* ed_cloned0 = this->CloneExecNode(ed_exit);
	Q.push_back(ed_exit);
	to_cloned--->set(ed_exit, ed_cloned0);
	subgraph.push_back(ed_exit);
	for (int qi = 0; qi < Q.size(); qi += 1) {
		for (ama::ExecNodeExtraLink* link = Q[qi]->prev.more; link; link = link->x) {
			ama::ExecNode* edi = link->target;
			ama::ExecNode* edi_cloned = to_cloned--->get(edi);
			if ( !edi_cloned ) {
				int filter_passed = 0;
				if ( edi->kind == ama::ENODE_DECL_ENTRY ) {
					if ( !filter || filter(this, edi, nullptr) ) {
						filter_passed = 1;
					}
				}
				if ( !filter_passed ) {
					for (ama::ExecEffect* eff = this->QueryEffects(edi); eff; eff = eff->x) {
						if ( interest_id < 0 || eff->interest_id == interest_id ) {
							if ( !filter || filter(this, edi, eff) ) {
								filter_passed = 1;
								break;
							}
						}
					}
				}
				if ( filter_passed ) {
					edi_cloned = this->CloneExecNode(edi);
					subgraph.push_back(edi);
				} else {
					edi_cloned = edi;
				}
				Q--->push(edi);
				to_cloned--->set(edi, edi_cloned);
			}
		}
	}
	std::unordered_map<ama::ExecNode*, intptr_t> inQ{};
	for ( ama::ExecNode * & ed: subgraph ) {
		Q.clear();
		inQ.clear();
		Q.push_back(ed);
		inQ--->set(ed, 1);
		ama::ExecNode* ed_cloned = to_cloned--->get(ed);
		for (int qi = 0; qi < Q.size(); qi += 1) {
			if ( qi > 0 ) {
				ama::ExecNode* edi_cloned = to_cloned--->get(Q[qi]);
				if ( edi_cloned != Q[qi] ) {
					//don't expand, but connect
					this->AddEdge(edi_cloned, ed_cloned);
					continue;
				}
			}
			for (ama::ExecNodeExtraLink* link = Q[qi]->prev.more; link; link = link->x) {
				ama::ExecNode* edi = link->target;
				if ( !inQ--->get(edi) ) {
					Q--->push(edi);
					inQ--->set(edi, 1);
				}
			}
		}
	}
	return ed_cloned0;
}

////////////////////////////
static std::vector<char const*> g_superscript_digits{"⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹"};
static std::vector<char const*> g_subscript_digits{"\342\202\200", "\342\202\201", "\342\202\202", "\342\202\203", "\342\202\204", "\342\202\205", "\342\202\206", "\342\202\207", "\342\202\210", "\342\202\211"};
struct GraphvizDumpingContext {
	ama::ExecSession* sess{};
	std::unordered_map<void*, intptr_t> node_numbers{};
	intptr_t cur_num{};
	intptr_t cur_ed_num{};
	ama::Node* nd_dumping{};
	std::unordered_map<ama::Node*, intptr_t> node_interest_ids{};
	void DumpNodeName(std::string& code, ama::Node* nd, int superscript);
	void DumpExecNodeName(std::string& code, ama::ExecNode* ed);
};
void GraphvizDumpingContext::DumpNodeName(std::string& code, ama::Node* nd, int superscript) {
	intptr_t num = this->node_numbers--->get(nd);
	if ( !num ) {
		num = ++this->cur_num;
		this->node_numbers--->set(nd, num);
	}
	if ( superscript ) {
		code--->push("\313\231");
		for ( char const& ch: JSON::stringify(num) ) {
			code--->push(g_superscript_digits[ch - '0']);
		}
	} else {
		code--->push(JSON::stringify(num));
	}
}
void GraphvizDumpingContext::DumpExecNodeName(std::string& code, ama::ExecNode* ed) {
	intptr_t num = this->node_numbers--->get(ed);
	if ( !num ) {
		num = ++this->cur_ed_num;
		this->node_numbers--->set(ed, num);
	}
	code--->push(JSON::stringify(num));
}
static int GraphvizGenerateNode(ama::CodeGenerator* gctx, ama::Node* nd) {
	GraphvizDumpingContext* dctx = (GraphvizDumpingContext*)(gctx->opaque);
	if ( nd != dctx->nd_dumping ) {
		if ( dctx->sess->canonicals[nd].rg_canon.entry ) {
			dctx->DumpNodeName(gctx->code, nd, 1);
			return 1;
		}
	}
	if ( nd->node_class == ama::N_SCOPE ) {
		gctx->code--->push("{...}");
	} else {
		gctx->GenerateDefault(nd);
	}
	intptr_t interest_id = dctx->node_interest_ids--->get(nd) - 1;
	if ( interest_id >= intptr_t(0L) ) {
		for ( char const& ch: JSON::stringify(interest_id) ) {
			gctx->code--->push(g_subscript_digits[ch - '0']);
		}
	}
	return 1;
}

static std::vector<char const*> g_enode_names{"nd", "after", "fork", "join", "br", "phi", "jump", "loop", "entry"};
static std::vector<char const*> g_enode_shapes{"box", "invtrapezium", "triangle", "invtriangle", "diamond", "invhouse", "oval", "parallelogram", "hexagon"};
std::string ama::ExecSession::DumpGraphviz(ama::ExecNode* extra_graph) {
	std::string ret{};{
		
		ret--->push("digraph session {\n"
		);
	}
	///////////////////
	if ( extra_graph ) {
		this->entries.push_back(extra_graph);
	}
	std::vector<ama::ExecNode*> eds = this->ComputeReachableSet(this->entries, ama::REACH_FORWARD | ama::REACH_BACKWARD);
	if ( extra_graph ) {
		this->entries--->pop();
	}
	//////////////
	GraphvizDumpingContext dctx{};
	dctx.sess = this;
	dctx.cur_num = 0;
	dctx.cur_ed_num = 0;
	dctx.nd_dumping = nullptr;
	ama::CodeGenerator gctx{.code = {}, .nd_current = nullptr, .opaque = nullptr, .hook = nullptr, .hook_comment = nullptr, .scope_indent_level = intptr_t(0L), .p_last_indent = intptr_t(0L), .tab_width = 4, .auto_space = 1};
	gctx.opaque = &dctx;
	gctx.hook = GraphvizGenerateNode;
	gctx.tab_indent = 0;
	for ( ama::ExecNode* const &ed: eds ) {
		dctx.nd_dumping = ed->nd;
		dctx.node_interest_ids.clear();
		//dump effects - QueryEffects is kinda a write accesss... which should be fine
		//use node dumper styling
		for (ama::ExecEffect* eff = this->QueryEffects(ed); eff; eff = eff->x) {
			dctx.node_interest_ids--->set(eff->nd_alleged_ref, eff->interest_id + 1);
		}
		gctx.code.clear();
		gctx.code--->push(g_enode_names[ed->kind]);
		dctx.DumpNodeName(gctx.code, ed->nd, 0);
		int has_interest = 0;
		for (ama::ExecEffect* eff = this->QueryEffects(ed); eff; eff = eff->x) {
			if ( !has_interest ) {
				has_interest = 1;
				gctx.code--->push(": [");
			} else {
				gctx.code--->push(", ");
			}
			gctx.code--->push(JSON::stringify(eff->interest_id));
		}
		if ( has_interest ) {
			gctx.code--->push(']');
		}
		gctx.code--->push('\n');
		gctx.Generate(ed->nd);
		ret--->push(g_enode_names[ed->kind]);
		dctx.DumpNodeName(ret, ed->nd, 0);
		ret.push_back('_');
		dctx.DumpExecNodeName(ret, ed);
		ret--->push(" [label=", JSON::stringify(gctx.code), ",shape=", g_enode_shapes[ed->kind]);
		if ( ed->flags & ama::ENODEF_FOLDED_CFG ) {
			ret--->push(",style=dashed");
		}
		if ( ed->flags & ama::ENODEF_DUMP_ACTIVE ) {
			ret--->push(",color=red");
		}
		ret--->push("]\n");
	}
	//the edges
	for ( ama::ExecNode* const &ed: eds ) {
		for (ama::ExecNodeExtraLink* link = ed->next.more; link; link = link->x) {
			ama::ExecNode* edi = link->target;
			ret--->push(g_enode_names[ed->kind]);
			dctx.DumpNodeName(ret, ed->nd, 0);
			ret.push_back('_');
			dctx.DumpExecNodeName(ret, ed);
			ret--->push(" -> ");
			ret--->push(g_enode_names[edi->kind]);
			dctx.DumpNodeName(ret, edi->nd, 0);
			ret.push_back('_');
			dctx.DumpExecNodeName(ret, edi);
			ret--->push('\n');
		}
	}
	ret--->push("}\n");
	return std::move(ret);
}
//TODO: expand call (resolving / detecting target function relevance), expand loop, diversify (expand phi), add entry point
