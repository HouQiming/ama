#ifndef _NODE_JCH_HPP
#define _NODE_JCH_HPP
#include <string>
#include <vector>
#include "../util/jc_array.h"
#include "../util/gcstring.h"
/*#pragma add("jc_files", "./node.jc");*/
/*#pragma add("cflags", "-fno-delete-null-pointer-checks");*/
namespace ama {
	struct Node;
};
namespace ama {
	static const uint8_t N_NONE = 0;
	/////////////////
	//The node classes are designed for .FindAll ergonomy: if we process two
	//types of nodes more often then not, they're the same class, and vice
	//versa. For example, `#include <foo>` and `#include "foo"` and 
	//`require('foo')` are all N_DEPENDENCY, only differentiated by flags.
	//While the relatively minor CUDA `foo<<<grid,block>>>` has its own node
	//class: because it requires different treatment than both template
	//substitution and function call.
	/////////////////
	//raw nodes
	static const uint8_t N_RAW = 1;
	static const uint8_t N_SYMBOL = 2;
	static const uint8_t N_REF = 3;
	static const uint8_t N_NUMBER = 4;
	static const uint8_t N_STRING = 5;
	/////////////////
	//"parsed" nodes
	//N_NODEOF is our own extension to switch between source code and ama code.
	//The syntax is `.(foo)`. Example uses:
	//    nd.Match(.(JSON.parse<.(Node.MatchAny('foo'))>)))
	//    nd_root.Insert(POS_FRONT, .(#include <stdio.h>))
	static const uint8_t N_NODEOF = 6;
	static const uint8_t N_SCOPE = 7;
	static const uint8_t N_FUNCTION = 8;
	static const uint8_t N_CLASS = 9;
	static const uint8_t N_POSTFIX = 10;
	static const uint8_t N_DOT = 11;
	static const uint8_t N_ITEM = 12;
	static const uint8_t N_CALL = 13;
	static const uint8_t N_CALL_TEMPLATE = 14;
	static const uint8_t N_CALL_CUDA_KERNEL = 15;
	static const uint8_t N_DEPENDENCY = 16;
	static const uint8_t N_BINOP = 17;
	static const uint8_t N_PREFIX = 18;
	static const uint8_t N_ASSIGNMENT = 19;
	static const uint8_t N_SCOPED_STATEMENT = 20;
	static const uint8_t N_EXTENSION_CLAUSE = 21;
	static const uint8_t N_PARAMETER_LIST = 22;
	static const uint8_t N_CONDITIONAL = 23;
	static const uint8_t N_LABELED = 24;
	//N_AIR is a node that stands for something but generates no code, like
	//the namespace in `::foo` and the undefined initial value of `int bar;`
	static const uint8_t N_AIR = 25;
	static const uint8_t N_FILE = 26;
	static const uint8_t N_SEMICOLON = 27;
	static const uint8_t N_PAREN = 28;
	static const uint8_t N_KEYWORD_STATEMENT = 29;
	static const uint8_t N_JS_REGEXP = 30;
	/////////////////
	//don't start any other constant with "N_", jsgen.cpp depends on this
	/////////////////
	//for Node::tmp_flags
	//all valid nodes should have TMPF_IS_NODE set
	static const uint16_t TMPF_IS_NODE = 32768u;
	//you should never set TMPF_GC_MARKED or see it set
	static const uint16_t TMPF_GC_MARKED = 16384u;
	/////////////////
	//for Node::flags
	static const uint32_t FILE_SPACE_INDENT = 1u;
	static const uint32_t REF_WRITTEN = 1u;
	//REF_RW is only set for REF_WRITTEN nodes: non-written nodes are trivially read 
	static const uint32_t REF_RW = 2u;
	//function / class names are declared but not written
	static const uint32_t REF_DECLARED = 4u;
	//const uint32_t! CALL_IS_UNARY_OPERATOR = 1u;
	static const uint32_t LITERAL_PARSED = 1u;
	static const uint32_t STRING_SINGLE_QUOTED = 2u;
	static const uint32_t DOT_PTR = 1u;
	static const uint32_t DOT_CLASS = 2u;
	static const uint32_t DEP_C_INCLUDE = 0u;
	static const uint32_t DEP_JS_REQUIRE = 1u;
	static const uint32_t DEP_TYPE_MASK = 31u;
	static const uint32_t DEPF_C_INCLUDE_NONSTR = 32u;
	static const uint32_t PARAMLIST_TEMPLATE = 1u;
	//for Node::indent_level, reserve 1 bit for future use
	static const intptr_t MAX_INDENT = 63;
	/////////////////
	//for Node::Insert
	static const int POS_BEFORE = 0;
	static const int POS_AFTER = 1;
	static const int POS_FRONT = 2;
	static const int POS_BACK = 3;
	static const int POS_REPLACE = 4;
	/////////////////
	//for Node::FindAllWithin 
	static const int32_t BOUNDARY_FUNCTION = 1;
	static const int32_t BOUNDARY_CLASS = 2;
	static const int32_t BOUNDARY_NODEOF = 4;
	static const int32_t BOUNDARY_SCOPE = 8;
	static const int32_t BOUNDARY_MATCH = 16;
	static const int32_t BOUNDARY_ONE_LEVEL = 32;
	static const int32_t BOUNDARY_ANY = 0x7fffffff;
	static const int32_t BOUNDARY_DEFAULT = BOUNDARY_NODEOF;
	/////////////////
	//return values of GetCFGRole()
	static const uint8_t CFG_BASIC = 0;
	static const uint8_t CFG_BRANCH = 1;
	static const uint8_t CFG_LOOP = 2;
	static const uint8_t CFG_JUMP = 3;
	static const uint8_t CFG_DECL = 4;
	/////////////////
	//for FormatFancyMessage
	static const int MSG_COLORED = 1;
	static const int MSG_WARNING = 2;
	/////////////////
	struct TCloneResult {
		Node* nd{};
		std::unordered_map<Node const*, Node*> mapping{};
	};
	//The AST node, we don't track original code locations, if you want to
	//report something, try putting them into the generated comments using 
	//`#error` and stuff.
	//Try to fit each Node into a 64-byte cacheline
	struct Node {
		uint8_t node_class{};
		/*
		indent_level stores delta-indent w.r.t. parent
		For example, in:
			int main(){
				return 0;
			}
		The `return 0;` node has .indent_level=4 (default tab width) while
		all other nodes have .indent_level=0
		*/
		int8_t indent_level{};
		//tmp_flags are caller-saved temporaries
		//They must not affect code generation
		uint16_t tmp_flags{};
		//flags are persistent flags affecting code generation
		//The flags of N_RAW is `opening_char|closing_char<<8`, for example:
		//    .([]) is nRaw().setFlags(0x5d5b)
		uint32_t flags{};
		//The associated string for leaf nodes
		ama::gcstring data{};
		/////////////
		ama::gcstring comments_before{};
		ama::gcstring comments_after{};
		/////////////
		//child
		Node* c{};
		//next sibling
		Node* s{};
		//parent
		Node* p{};
		//previous sibling, or PackTailPointer(tail sibling) if this==this->p->c
		Node* v{};
		/////////////
		//LastChildSP is for internal use only in simppair.cpp (SP stands for simppair)
		ama::Node* LastChildSP()const;
		/////////////
		//These setters are also available in JS
		inline ama::Node* setData(ama::gcstring data) {
			this->data = data;
			return this;
		}
		inline ama::Node* setFlags(int flags) {
			this->flags = flags;
			return this;
		}
		inline ama::Node* setCommentsBefore(ama::gcstring comments) {
			//assert(comments != nullptr);
			this->comments_before = comments;
			return this;
		}
		inline ama::Node* setCommentsAfter(ama::gcstring comments) {
			//assert(comments != nullptr);
			this->comments_after = comments;
			return this;
		}
		inline ama::Node* setIndent(int indent_level) {
			this->indent_level = indent_level;
			return this;
		}
		/////////////
		//Nodes are garbage-collected by ama::gc(), but you can also
		//free them manually with Node::FreeASTStorage();
		//Debug build checks for double-frees, but no guarantees.
		//Note that FreeASTStorage() frees the entire tree under `this`.
		void FreeASTStorage();
		ama::TCloneResult CloneEx()const;
		ama::Node* Clone()const;
		//ReplaceWith has a quirk -- you can't directly replace a node with
		//something that parents it. For example, this is invalid:
		//    nd.ReplaceWith(nParen(nd)); //will fail!
		//Instead, use GetPlaceHolder():
		//    Node* nd_tmp=GetPlaceHolder();
		//    nd.ReplaceWith(nd_tmp);
		//    nd_tmp.ReplaceWith(nParen(nd));
		ama::Node* ReplaceWith(ama::Node* nd_new);
		ama::Node* Unlink();
		ama::Node* Insert(int pos, ama::Node* nd_new);
		ama::Node* Root()const;
		ama::Node* RootStatement()const;
		ama::Node* LastChild()const;
		int isAncestorOf(ama::Node const* nd_maybe_child)const;
		//Find an ancestor node with node_class nc
		//Returns NULL if not found
		ama::Node* Owning(int nc)const;
		//Find the owning N_CLASS or N_FILE
		//Returns this->Root() if neither is found
		ama::Node* Owner()const;
		ama::Node* CommonAncestor(ama::Node const* b)const;
		//Returns the string content for N_STRING
		//The default parser keeps quotes around source code strings, so
		//parsing "hello world" gives a node with .data="\"hello world\""
		//You need GetStringValue() to get the content "hello world".
		//Once you call GetStringValue() though, the original textual form
		//is no longer preserved. For example, "hello \u0077orld" becomes
		//"hello world". That's why this function is not `const`. 
		ama::gcstring GetStringValue();
		//Create N_DOT with this node as the object
		ama::Node* dot(ama::gcstring name);
		ama::Node* Find(int node_class, ama::gcstring data)const;
		std::vector<ama::Node*> FindAll(int node_class, ama::gcstring data = ama::gcstring())const;
		std::vector<ama::Node*> FindAllWithin(int32_t boundary, int node_class, ama::gcstring data = ama::gcstring())const;
		std::vector<ama::Node*> FindAllBefore(ama::Node const* nd_before, int32_t boundary, int node_class, ama::gcstring data = ama::gcstring())const;
		int isRawNode(char ch_open, char ch_close)const;
		//Best effort "name" getter
		//For example, .(this->FindAll(N_CALL,'test')).data is "",
		//but .GetName() on the same node returns "FindAll".
		ama::gcstring GetName()const;
		/////////////
		//toSource() returns the full source code of a node
		//The results are production-ready
		//Implemented in src/codegen/gen.jc
		std::string toSource()const;
		//dump() returns an abbreviated string dump useful in error messages
		//The result may not be useful as code
		//Implemented in src/codegen/gen.jc
		std::string dump()const;
		int isMethodCall(ama::gcstring name)const;
		int isSymbol(std::span<char> name)const;
		int isRef(std::span<char> name)const;
		ama::Node* InsertDependency(uint32_t flags, ama::gcstring name);
		ama::Node* InsertCommentBefore(std::span<char> s);
		ama::Node* MergeCommentsBefore(ama::Node* nd_before);
		ama::Node* MergeCommentsAfter(ama::Node* nd_after);
		ama::Node* MergeCommentsAndIndentAfter(ama::Node* nd_after);
		ama::gcstring DestroyForSymbol();
		//Validate() is purely for debugging: it validates the tree and when
		//there's any error, it prints some messages and aborts 
		void Validate();
		//ValidateEx returns the errors count instead of aborting if quiet==1
		int ValidateEx(intptr_t max_depth, int quiet);
		int ValidateChildCount(int n_children);
		ama::Node* ParentStatement();
		ama::Node* Prev();
		//Each BreakFoo function unlinks the foo node and returns it
		//Unlike Unlink(), BreakSibling() keeps the .s pointer of the
		//broken-off sibling
		ama::Node* BreakSibling();
		ama::Node* BreakChild();
		ama::Node* BreakSelf();
		//Replace all nodes between this and nd_upto with nd_new
		//nd_upto is inclusive, use nd_new==NULL to delete the nodes instead
		ama::Node* ReplaceUpto(ama::Node* nd_upto, ama::Node* nd_new);
		void AdjustIndentLevel(intptr_t delta);
		/*
		For a recursion-free preorder traversal of nd:
			for(Node* ndi=nd;ndi;ndi=ndi->PreorderNext(nd)){
				if we want to skip ndi's children {
					ndi=ndi->PreorderSkip();
					continue;
				}
			}
		*/
		ama::Node* PreorderNext(ama::Node* nd_root);
		ama::Node* PreorderSkip();
		ama::Node* PostorderFirst();
		ama::Node* PostorderNext(ama::Node* nd_root);
		//toSingleNode() converts a node with possible sibling chain into
		//a single node.
		//With -fno-delete-null-pointer-checks, NULL->toSingleNode()
		//returns nAir()
		ama::Node* toSingleNode();
		//Unparse turns a node back to a less-parsed state (usually N_RAW)
		//It's mainly used to correct over-eager mistakes in an earlier pass
		ama::Node* Unparse();
		uint8_t GetCFGRole()const;
		int isChildCFGDependent(ama::Node const* nd_child)const;
		//Format a clang-style message, referencing `this`
		//We intentionally limit our messages to warnings and notes
		std::string FormatFancyMessage(std::span<char> msg, int flags)const;
		//ComputeLineNumber currently traverses the entire AST up to `this`
		int32_t ComputeLineNumber() const;
	};
	extern ama::Node* g_placeholder;
	ama::Node* AllocNode();
	//All nodes are allocated from a dedicated pool, as a side effect, we can
	//check whether an arbitrary pointer is a valid Node in O(1).
	int isValidNodePointer(ama::Node const* nd_tentative);
	//The garbage collector also utilizes this pool to get all possible node
	//pointers.
	std::vector<ama::Node*> GetAllPossibleNodeRanges();
	ama::Node* CreateNode(uint8_t node_class, ama::Node* child);
	ama::Node* FixParents(ama::Node* nd_parent, ama::Node* nd);
	static inline ama::Node* cons(ama::Node* a, ama::Node* b) {
		assert(a != b);
		a->s = b;
		if ( b ) { b->v = a; }
		return a;
	}
	static inline ama::Node* InsertMany(std::span<ama::Node*> children) {
		ama::Node* nd{};
		for (intptr_t I = children.size() - intptr_t(1L); I >= intptr_t(0L); I -= intptr_t(1L)) {
			nd = cons(children[I], nd);
		}
		return nd;
	}
	static inline ama::Node* nString(ama::gcstring s) {
		return ama::CreateNode(N_STRING, nullptr)->setFlags(ama::LITERAL_PARSED)->setData(s);
	}
	static inline ama::Node* nRef(ama::gcstring s) {
		return ama::CreateNode(N_REF, nullptr)->setData(s);
	}
	static inline ama::Node* nSymbol(ama::gcstring s) {
		return ama::CreateNode(N_SYMBOL, nullptr)->setData(s);
	}
	static inline ama::Node* nNumber(ama::gcstring s) {
		return ama::CreateNode(N_NUMBER, nullptr)->setData(s);
	}
	static inline ama::Node* nNodeof(Node* nd) {
		nd->s = nullptr;
		return ama::CreateNode(N_NODEOF, nd);
	}
	static inline ama::Node* nPrefix(ama::gcstring s, Node* nd) {
		nd->s = nullptr;
		return ama::CreateNode(N_PREFIX, nd)->setData(s);
	}
	static inline ama::Node* nPostfix(Node* nd, ama::gcstring s) {
		nd->s = nullptr;
		return ama::CreateNode(N_POSTFIX, nd)->setData(s);
	}
	static inline ama::Node* nAssignment(Node* nd_def, Node* nd_value) {
		return ama::CreateNode(N_ASSIGNMENT, cons(nd_def, cons(nd_value, nullptr)))->setData("");
	}
	static inline ama::Node* nUpdate(Node* nd_def, ama::gcstring op, Node* nd_value) {
		return ama::CreateNode(N_ASSIGNMENT, cons(nd_def, cons(nd_value, nullptr)))->setData(op);
	}
	static inline ama::Node* nBinop(Node* nd_a, ama::gcstring op, Node* nd_b) {
		return ama::CreateNode(N_BINOP, cons(nd_a, cons(nd_b, nullptr)))->setData(op);
	}
	static inline ama::Node* nExtensionClause(ama::gcstring keyword, Node* nd_arg, Node* nd_scope) {
		return ama::CreateNode(N_EXTENSION_CLAUSE, cons(nd_arg, nd_scope))->setData(keyword);
	}
	//the namespace in `::foo`, the initial value in `int bar;`
	static inline ama::Node* nAir() {
		return ama::CreateNode(N_AIR, nullptr);
	}
	static inline ama::Node* nConditional(Node* nd_cond, Node* nd_true, Node* nd_false) {
		return ama::CreateNode(N_CONDITIONAL, cons(nd_cond, cons(nd_true, cons(nd_false, nullptr))));
	}
	static inline ama::Node* nClass(ama::gcstring keyword, Node* nd_before, Node* nd_name, Node* nd_after, Node* nd_body) {
		assert(nd_name->node_class == N_REF || nd_name->node_class == N_DOT);
		return ama::CreateNode(N_CLASS, cons(nd_before, cons(nd_name, cons(nd_after, cons(nd_body, nullptr)))))->setData(keyword);
	}
	static inline ama::Node* nFunction(Node* nd_before, Node* nd_proto, Node* nd_after, Node* nd_body) {
		return ama::CreateNode(N_FUNCTION, cons(nd_before, cons(nd_proto, cons(nd_after, cons(nd_body, nullptr)))));
	}
	static inline ama::Node* nLabeled(Node* nd_label, Node* nd_value) {
		return ama::CreateNode(N_LABELED, cons(nd_label, cons(nd_value, nullptr)));
	}
	static inline ama::Node* nDependency(Node* nd) {
		nd->s = nullptr;
		return ama::CreateNode(N_DEPENDENCY, nd);
	}
	///////////////
	static inline ama::Node* GetPlaceHolder() {
		assert(!g_placeholder->p);
		assert(!g_placeholder->s);
		g_placeholder->comments_before = "";
		g_placeholder->comments_after = "";
		g_placeholder->c = nullptr;
		return g_placeholder;
	}
	static inline int isValidPreviousSibling(Node const* v) {
		return v && !(intptr_t(v) & 1);
	}
	static inline Node* PackTailPointer(Node* nd_tail) {
		assert(!(intptr_t(nd_tail) & intptr_t(1L)));
		return (Node*)(intptr_t(nd_tail) | intptr_t(1L));
	}
	static inline Node* UnpackTailPointer(Node* nd_tail) {
		assert(intptr_t(nd_tail) & intptr_t(1L));
		return (Node*)(intptr_t(nd_tail) & ~intptr_t(1L));
	}
	ama::Node* CreateNodeFromChildren(uint8_t node_class, std::span<ama::Node*> children);
	///////////////
	int8_t ClampIndentLevel(intptr_t level);
	//the non-method toSingleNode doesn't need -fno-delete-null-pointer-checks
	ama::Node* toSingleNode(ama::Node* nd_child);
	ama::Node* UnparseRaw(ama::Node* nd_raw);
	extern ama::Node* g_free_nodes;
	int ValidateChildRange(ama::Node* p0, ama::Node* p1);
	void DeleteChildRange(ama::Node* nd0, ama::Node* nd1);
	ama::Node* ReplaceChildRange(ama::Node* nd0, ama::Node* nd1, ama::Node* nd_new);
	//it's in JSAPI
	void DumpASTAsJSON(ama::Node* nd);
};

#endif
