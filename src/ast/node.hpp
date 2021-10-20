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
	//raw nodes
	static const uint8_t N_RAW = 1;
	static const uint8_t N_SYMBOL = 2;
	static const uint8_t N_REF = 3;
	static const uint8_t N_NUMBER = 4;
	static const uint8_t N_STRING = 5;
	/////////////////
	//structured nodes
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
	//we don't want to process N_AIR / N_FILE when finding N_RAW
	static const uint8_t N_AIR = 25;
	static const uint8_t N_FILE = 26;
	static const uint8_t N_SEMICOLON = 27;
	static const uint8_t N_PAREN = 28;
	static const uint8_t N_KEYWORD_STATEMENT = 29;
	/////////////////
	static const uint8_t N_JS_REGEXP = 30;
	//don't start any other constant with N_
	/////////////////
	static const uint16_t TMPF_IS_NODE = 32768u;
	static const uint16_t TMPF_GC_MARKED = 16384u;
	/////////////////
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
	//reserve 1 bit
	static const intptr_t MAX_INDENT = 63;
	/////////////////
	struct TCloneResult {
		Node* nd{};
		std::unordered_map<Node const*, Node*> mapping{};
	};
	static const int POS_BEFORE = 0;
	static const int POS_AFTER = 1;
	static const int POS_FRONT = 2;
	static const int POS_BACK = 3;
	static const int POS_REPLACE = 4;
	/////////////////
	static const int32_t BOUNDARY_FUNCTION = 1;
	static const int32_t BOUNDARY_CLASS = 2;
	//const int32_t! BOUNDARY_LOOP = 4;
	//const int32_t! BOUNDARY_SWITCH = 8;
	//we no longer have macro decls
	//const int32_t! BOUNDARY_MACRO_DECL = 16;
	static const int32_t BOUNDARY_NODEOF = 32;
	static const int32_t BOUNDARY_SCOPE = 64;
	static const int32_t BOUNDARY_MATCH = 128;
	static const int32_t BOUNDARY_ONE_LEVEL = 256;
	//const int32_t! BOUNDARY_PROTOTYPE = 512;
	static const int32_t BOUNDARY_ANY = 0x7fffffff;
	static const int32_t BOUNDARY_DEFAULT = BOUNDARY_NODEOF;
	/////////////////
	static const uint8_t CFG_BASIC = 0;
	static const uint8_t CFG_BRANCH = 1;
	static const uint8_t CFG_LOOP = 2;
	static const uint8_t CFG_JUMP = 3;
	//decl is different from basic since we never get into them from the outside
	static const uint8_t CFG_DECL = 4;
	//we don't track original code locations, just report errors into the generated code using #error and stuff
	//try to fit into a 64 byte cacheline
	struct Node {
		uint8_t node_class{};
		//store delta w.r.t. parent N_RAW
		int8_t indent_level{};
		uint16_t tmp_flags{};
		uint32_t flags{};
		ama::gcstring data{};
		/////////////
		ama::gcstring comments_before{};
		ama::gcstring comments_after{};
		/////////////
		//child
		Node* c{};
		//sibling
		Node* s{};
		//parent
		Node* p{};
		//last sibling
		Node* v{};
		/////////////
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
		ama::TCloneResult CloneEx()const;
		ama::Node* Clone()const;
		//auto ReplaceLinkToThis(ama::Node*+! this, ama::Node*+! nd_new);
		ama::Node* ReplaceWith(ama::Node* nd_new);
		ama::Node* Unlink();
		ama::Node* Insert(int pos, ama::Node* nd_new);
		ama::Node* Root()const;
		ama::Node* RootStatement()const;
		int isAncestorOf(ama::Node const* nd)const;
		ama::Node* Owning(int nc)const;
		ama::Node* Owner()const;
		//LastChildSP is for internal use only in simppair.jc (SP stands for simppair)
		ama::Node* LastChildSP()const;
		ama::Node* LastChild()const;
		ama::Node* CommonAncestor(ama::Node const* b)const;
		ama::gcstring GetStringValue()const;
		ama::Node* dot(ama::gcstring name);
		void FreeASTStorage();
		ama::Node* Find(int node_class, ama::gcstring data)const;
		std::vector<ama::Node*> FindAll(int node_class, ama::gcstring data=ama::gcstring())const;
		std::vector<ama::Node*> FindAllWithin(int32_t boundary, int node_class, ama::gcstring data=ama::gcstring())const;
		std::vector<ama::Node*> FindAllBefore(ama::Node const* nd_before, int32_t boundary, int node_class, ama::gcstring data=ama::gcstring())const;
		int isRawNode(char ch_open, char ch_close)const;
		ama::gcstring GetName()const;
		/////////////
		//src/codegen/gen.jc
		std::string toSource()const;
		int isMethodCall(ama::gcstring name)const;
		ama::Node* InsertDependency(uint32_t flags, ama::gcstring name);
		ama::Node* InsertCommentBefore(std::span<char> s);
		//ama::Node*! Save(ama::Node*! this, char[|]! change_ext);
		ama::Node* MergeCommentsBefore(ama::Node* nd_before);
		ama::Node* MergeCommentsAfter(ama::Node* nd_after);
		ama::Node* MergeCommentsAndIndentAfter(ama::Node* nd_after);
		ama::gcstring DestroyForSymbol();
		//expr is ill-defined once detached from a base language
		//NeedTrailingSemicolon is more practical
		//int! isExpr(ama::Node*! this);
		int isSymbol(std::span<char> name)const;
		int isRef(std::span<char> name)const;
		void Validate();
		intptr_t ValidateEx(intptr_t max_depth, int quiet);
		int NeedTrailingSemicolon()const;
		int8_t GetCommentedIndentLevel(int32_t tab_width)const;
		ama::Node* ParentStatement();
		ama::Node* Prev();
		ama::Node* BreakSibling();
		ama::Node* BreakChild();
		ama::Node* BreakSelf();
		///nd_upto is inclusive
		ama::Node* ReplaceUpto(ama::Node* nd_upto, ama::Node* nd_new);
		int ValidateChildCount(int n_children);
		void AdjustIndentLevel(intptr_t delta);
		ama::Node* PreorderNext(ama::Node* nd_root);
		ama::Node* PreorderSkipChildren(ama::Node* nd_root);
		ama::Node* PreorderLastInside();
		ama::Node* PostorderFirst();
		ama::Node* PostorderNext(ama::Node* nd_root);
		ama::Node* toSingleNode();
		ama::Node* Unparse();
		uint8_t GetCFGRole()const;
		int isChildCFGDependent(ama::Node const* nd_child)const;
	};
	ama::Node* AllocNode();
	extern ama::Node* g_placeholder;
	int isValidNodePointer(ama::Node const* nd_tentative);
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
	//inline ama::Node*+! nTypedVar(Node*+! nd_type, Node*+! nd_def) {
	//	return ama::CreateNode(N_TYPED_VAR, cons(nd_type, cons(nd_def, NULL)));
	//}
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
	//inline ama::Node*+! BreakLink(Node*+*+! pndi) {
	//	ama::Node*+! ret = *pndi;
	//	*pndi = NULL;
	//	return ret;
	//}
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
	ama::Node* toSingleNode(ama::Node* nd_child);
	ama::Node* UnparseRaw(ama::Node* nd_raw);
	std::vector<ama::Node*> GetAllPossibleNodeRanges();
	extern ama::Node* g_free_nodes;
	int ValidateChildRange(ama::Node* p0, ama::Node* p1);
	void DeleteChildRange(ama::Node* nd0, ama::Node* nd1);
	ama::Node* ReplaceChildRange(ama::Node* nd0, ama::Node* nd1, ama::Node* nd_new);
	//it's in JSAPI
	void DumpASTAsJSON(ama::Node* nd);
};

#endif
