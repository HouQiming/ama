#include <vector>
#include <array>
#include <unordered_map>
#include "../ast/node.hpp"
#include "../script/jsenv.hpp"
#include "postfix.hpp"
#include "decl.hpp"
#include "operator.hpp"
#include "scoping.hpp"
#include "../util/jc_array.h"
namespace ama {
	ama::Node* ConvertToParameterList(ama::Node* nd_raw) {
		uint32_t flags = 0;
		if (nd_raw->node_class == ama::N_PAREN) {
			nd_raw->Unparse();
		}
		if (nd_raw->isRawNode('<', '>')) {
			flags = ama::PARAMLIST_TEMPLATE;
		} else if (!nd_raw->isRawNode('(', ')')) {
			flags = ama::PARAMLIST_UNWRAPPED;
			nd_raw = ama::CreateNode(ama::N_RAW, nd_raw)->setFlags(uint32_t('(') | (uint32_t(')') << 8));
		}
		std::vector<ama::Node*> params{};
		for (ama::Node* ndi = nd_raw->c; ndi; ndi = ndi->s) {
			if ( ndi->isSymbol(",") ) { continue; }
			if ( ndi->isRawNode(0, 0) && ndi->c && ndi->c->s && ndi->LastChild()->isSymbol(",") ) {
				ama::Node* nd_comma = ndi->LastChild();
				nd_comma->Unlink();
				ama::Node* nd_arg_last = ndi->LastChild();
				nd_arg_last->MergeCommentsAndIndentAfter(nd_comma);
				ndi->comments_after = (ndi->comments_after + nd_comma->comments_after);
				nd_comma->FreeASTStorage();
				if ( !ndi->c->s ) {
					ndi = ama::UnparseRaw(ndi);
				}
			}
			params--->push(ndi);
		}
		return ama::CreateNodeFromChildren(ama::N_PARAMETER_LIST, params)->setFlags(flags)->setIndent(nd_raw->indent_level)->setCommentsBefore(nd_raw->comments_before)->setCommentsAfter(nd_raw->comments_after);
	}
	//detects: N_FUNCTION and N_CLASS
	//also parses if / else / ...
	//after ParseAssignment	
	static const int KW_NONE = 0;
	static const int KW_CLASS = 1;
	static const int KW_STMT = 2;
	static const int KW_EXT = 3;
	static const int KW_FUNC = 4;
	static const int KW_NOT_FUNC = 5;
	//void! DumpASTAsJSON(ama::Node*+! nd);
	static ama::Node* TranslateCUnscopedStatement(std::vector<ama::Node*> &Q, ama::Node* nd_keyword, ama::Node* nd_end, int is_elseif) {
		ama::Node* nd_tmp = ama::GetPlaceHolder();
		nd_keyword->ReplaceUpto(nd_end, nd_tmp);
		ama::Node* nd_body = nd_keyword->BreakSibling();
		ama::Node* nd_arg = nullptr;
		if (nd_body) {
			ama::Node* nd_body_first = nd_body;
			while (nd_body_first->node_class == ama::N_CALL || nd_body_first->node_class == ama::N_POSTFIX) {
				nd_body_first = nd_body_first->c;
			}
			if (nd_body_first->isRawNode('(', ')')) {
				while (nd_body->node_class == ama::N_CALL || nd_body->node_class == ama::N_POSTFIX) {
					nd_body = nd_body->Unparse();
				}
			}
		}
		if (!nd_arg && nd_keyword && (nd_keyword->data == "if" || nd_keyword->data == "while") && nd_body && 
		nd_body->node_class != ama::N_LABELED && nd_body->node_class != ama::N_RAW) {
			ama::Node* nd_last_colon = nullptr;
			for (ama::Node* ndi = nd_body; ndi; ndi = ndi->s) {
				if ( ndi->isSymbol(":") ) {
					nd_last_colon = ndi;
				}
			}
			if (!nd_last_colon) {
				//it's saner to treat the `foo` in dangling `if foo` as the condition than the body
				nd_arg = nd_body;
				nd_body = nullptr;
			}
		}
		if (is_elseif) {
			//there is no arg
			nd_arg = nullptr;
		} else if ( nd_keyword->node_class == ama::N_CALL ) {
			nd_keyword = ama::UnparseCall(nd_keyword);
			nd_arg = nd_keyword->BreakSibling();
		} else if ( nd_body && nd_body->isRawNode('(', ')') ) {
			nd_arg = nd_body;
			nd_body = nd_body->BreakSibling();
		} else {
			//Python-like : separation handling
			ama::Node* nd_last_colon = nullptr;
			for (ama::Node* ndi = nd_body; ndi; ndi = ndi->s) {
				if ( ndi->isSymbol(":") ) {
					nd_last_colon = ndi;
				}
			}
			if ( nd_last_colon ) {
				nd_arg = nd_body;
				nd_body = nd_last_colon->BreakSibling();
			}
		}
		ama::Node* nd_stmt = ama::CreateNode(ama::N_SSTMT, nullptr)->setData(nd_keyword->data)->setCommentsBefore(nd_keyword->comments_before)->setIndent(nd_keyword->indent_level);
		nd_body = ama::toSingleNode(nd_body);
		//if (!is_elseif) {
		//	ama::Node* nd_scoped_body = ama::CreateNode(ama::N_SCOPE, nd_body);
		//	if ( nd_body->comments_before--->indexOf('\n') < 0 ) {
		//		std::swap(nd_scoped_body->comments_before, nd_body->comments_before);
		//	}
		//	if ( nd_body->comments_after--->indexOf('\n') < 0 ) {
		//		std::swap(nd_scoped_body->comments_after, nd_body->comments_after);
		//	}
		//	nd_scoped_body->indent_level = nd_keyword->indent_level;
		//	if ( nd_body->comments_after--->indexOf('\n') >= 0 && nd_stmt->comments_after--->indexOf('\n') < 0 ) {
		//		nd_stmt->comments_after = (ama::gcscat(nd_stmt->comments_after, "\n"));
		//	}
		//	if ( nd_body->comments_before--->indexOf('\n') >= 0 && nd_body->comments_after--->indexOf('\n') < 0 ) {
		//		nd_body->comments_after = (ama::gcscat(nd_body->comments_after, "\n"));
		//	}
		//	nd_body->AdjustIndentLevel(-nd_scoped_body->indent_level);
		//	nd_body = nd_scoped_body;
		//}
		nd_stmt->Insert(
			ama::POS_FRONT,
			ama::cons(ama::toSingleNode(nd_arg)->MergeCommentsBefore(nd_keyword), nd_body)
		);
		nd_tmp->ReplaceWith(nd_stmt);
		nd_keyword->s = nullptr;
		nd_keyword->FreeASTStorage();
		if (nd_body->node_class == ama::N_RAW) {
			//things could nest
			Q.push_back(nd_body);
		}
		return nd_stmt;
	}
	static ama::Node* TranslateDoWhileClause(ama::Node* nd_keyword, ama::Node* nd_end) {
		ama::Node* nd_tmp = ama::GetPlaceHolder();
		nd_keyword->ReplaceUpto(nd_end, nd_tmp);
		ama::Node* nd_arg{};
		ama::Node* nd_body = nd_keyword->BreakSibling();
		if ( nd_keyword->node_class == ama::N_CALL ) {
			nd_keyword = ama::UnparseCall(nd_keyword);
			nd_arg = nd_keyword->BreakSibling();
		} else if ( nd_body && nd_body->isRawNode('(', ')') ) {
			nd_arg = nd_body;
			nd_body = nd_body->BreakSibling();
		}
		ama::Node* nd_stmt = ama::CreateNode(
			ama::N_EXTENSION_CLAUSE, ama::cons(ama::toSingleNode(nd_arg)->MergeCommentsBefore(nd_keyword), ama::nAir())
		)->setData(nd_keyword->data)->setCommentsBefore(nd_keyword->comments_before)->setIndent(nd_keyword->indent_level);
		nd_tmp->ReplaceWith(ama::cons(nd_stmt, nd_body));
		nd_keyword->s = nullptr;
		nd_keyword->FreeASTStorage();
		return nd_stmt;
	}
	static ama::Node* TranslateCForwardDeclaration(ama::Node* nd_raw) {
		//N_FUNCTION with air body
		//before, paramlist, after, body
		ama::Node* nd_proto{};
		for (ama::Node* ndi = nd_raw->c; ndi; ndi = ndi->s) {
			if ( ndi->node_class == ama::N_CALL ) {
				nd_proto = ndi;
			}
		}
		if ( !nd_proto ) { return nullptr; }
		ama::Node* nd_fname = ama::UnparseCall(nd_proto);
		nd_proto = nd_fname->BreakSibling();
		ama::Node* nd_after = ama::toSingleNode(nd_proto->BreakSibling());
		ama::Node* nd_before = ama::toSingleNode(nd_raw->BreakChild());
		ama::Node* nd_func = ama::nFunction(nd_before, ConvertToParameterList(nd_proto), nd_after, ama::nAir());
		//nd_func.data = GetFunctionName(0, nd_func);
		nd_raw->ReplaceWith(nd_func);
		//nd_raw->FreeASTStorage();
		return nd_func;
	}
	////pp1 is inclusive, also breaks the pp1 link
	//private ama::Node*+*+! UpdateLink(ama::Node*+! nd0, ama::Node*+! nd1) {
	//	for(ama::Node*+! ndi = nd0; ndi != nd1; ndi = ndi.s) {
	//		if( ndi.s == nd1 ) { return &ndi.s; }
	//	}
	//	assert(0);
	//	return NULL;
	//}
	ama::Node* ParseScopedStatements(ama::Node* nd_root, JSValue options) {
		std::unordered_map<ama::gcstring, int> keywords_class = ama::GetPrioritizedList(options, "keywords_class");
		std::unordered_map<ama::gcstring, int> keywords_scoped_statement = ama::GetPrioritizedList(options, "keywords_scoped_statement");
		std::unordered_map<ama::gcstring, int> keywords_extension_clause = ama::GetPrioritizedList(options, "keywords_extension_clause");
		std::unordered_map<ama::gcstring, int> keywords_function = ama::GetPrioritizedList(options, "keywords_function");
		std::unordered_map<ama::gcstring, int> keywords_after_class_name = ama::GetPrioritizedList(options, "keywords_after_class_name");
		std::unordered_map<ama::gcstring, int> keywords_after_prototype = ama::GetPrioritizedList(options, "keywords_after_prototype");
		std::unordered_map<ama::gcstring, int> keywords_not_a_function = ama::GetPrioritizedList(options, "keywords_not_a_function");
		int32_t parse_c_forward_declarations = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_c_forward_declarations"), 1);
		int32_t parse_cpp11_lambda = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_cpp11_lambda"), 1);
		int32_t struct_can_be_type_prefix = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "struct_can_be_type_prefix"), 1);
		std::vector<ama::Node*> Q = nd_root->FindAllWithin(0, ama::N_RAW);
		for ( intptr_t qi = 0; qi < intptr_t(Q.size()); qi++ ) {
			ama::Node* nd_raw = Q[qi];
			if ( !nd_raw->p ) { continue; }
			if ( nd_raw->p->node_class == ama::N_KSTMT && nd_raw->p->data--->startsWith('#') ) {
				//the `defined()` in #if defined(){}
				continue;
			}
			//int in_paramlist = 0;
			//ama::Node* nd_paramlist = nd_raw->Owning(ama::N_PARAMETER_LIST);
			//if (nd_paramlist) {
			//	//ignore `class foo` in parameter lists
			//	in_paramlist = 1;
			//	for (ama::Node* ndj = nd_raw; ndj != nd_paramlist; ndj = ndj.p) {
			//		if (ndj.node_class == ama::N_SCOPE) {
			//			in_paramlist = 0;
			//			break;
			//		}
			//	}
			//}
			ama::Node* ndi = nd_raw->c;
			ama::Node* nd_prototype_start = ndi;
			ama::Node* nd_last_scoped_stmt{};
			ama::Node* nd_keyword{};
			int kw_mode = KW_NONE;
			int is_elseif = 0;
			int after_qmark = 0;
			int after_colon = 0;
			while ( ndi ) {
				ama::Node* ndi_next = ndi->s;
				if ( kw_mode == KW_EXT && nd_last_scoped_stmt && ndi == nd_keyword->s && 
				(ndi->node_class == ama::N_REF || ndi->node_class == ama::N_CALL) && keywords_scoped_statement--->get(ndi->GetName()) ) {
					//`else if` case: translate if into N_RAW and queue it
					is_elseif = 1;
					break;
				}
				if (ndi->isSymbol("?")) {
					after_qmark += 1;
				}
				if (ndi->isSymbol(":")) {
					if (after_qmark > 0) {
						after_qmark -= 1;
					} else {
						after_colon += 1;
					}
				}
				if ( !nd_keyword && (ndi->node_class == ama::N_REF || ndi->node_class == ama::N_CALL) ) {
					//keywords are not necessarily statement starters: template<>, weird macro, label, etc.
					ama::gcstring name = ndi->GetName();
					if ( !name.empty() ) {
						if ( keywords_class--->get(name) ) {
							nd_keyword = ndi;
							kw_mode = KW_CLASS;
							continue;
						} else if ( keywords_extension_clause--->get(name) ) {
							if (name == "while" && !(nd_last_scoped_stmt && nd_last_scoped_stmt->data == "do") && !(nd_keyword && nd_keyword->data == "do")) {
								//fall through: a new `while` statement
							} else if ( nd_last_scoped_stmt ) {
								nd_keyword = ndi;
								kw_mode = KW_EXT;
								continue;
							} else if ( nd_keyword && (kw_mode == KW_STMT || kw_mode == KW_EXT) ) {
								//extension of unscoped statement
								//ama::Node*+! nd_ext_keyword = ndi.BreakSelf();
								ama::Node* nd_stmt = TranslateCUnscopedStatement(Q, nd_keyword, ndi->Prev(), 0);
								//nd_stmt.Insert(ama::POS_AFTER, nd_ext_keyword);
								nd_last_scoped_stmt = nd_stmt;
								nd_keyword = ndi;
								kw_mode = KW_EXT;
								continue;
							} else {
								//fall through: could be `while`
							}
						}
						if ( keywords_scoped_statement--->get(name) ) {
							nd_keyword = ndi;
							kw_mode = KW_STMT;
							continue;
						} else if ( keywords_function--->get(name) ) {
							nd_keyword = ndi;
							kw_mode = KW_FUNC;
							continue;
						} else if ( keywords_not_a_function--->get(name) ) {
							nd_keyword = ndi;
							kw_mode = KW_NOT_FUNC;
							continue;
						}
					}
				} else if ( ndi->isSymbol(";") || after_colon == 0 && ndi->isSymbol(",") ) {
					//we can't just delimit with "," everywhere: C++ constructor parameter list
					//but we DO need to delimit with "," for mixed C++ / C declarations like `int a(0),b`
					//so test for ':' before that isn't paired with '?'
					if ( nd_keyword && (kw_mode == KW_STMT || kw_mode == KW_EXT) ) {
						//unscoped statement
						//ndi_next = ndi.BreakSibling();
						//drop the ;
						//ama::BreakLink(pndi);
						ama::Node* nd_stmt{};
						if ( kw_mode == KW_EXT && nd_last_scoped_stmt && (nd_keyword->GetName() == "while" || nd_keyword->GetName() == "until") ) {
							nd_stmt = TranslateDoWhileClause(nd_keyword, ndi);
						} else {
							nd_stmt = TranslateCUnscopedStatement(Q, nd_keyword, ndi, 0);
						}
						if ( kw_mode == KW_EXT && nd_last_scoped_stmt ) {
							nd_stmt->node_class = ama::N_EXTENSION_CLAUSE;
							nd_stmt->Unlink();
							nd_last_scoped_stmt->Insert(ama::POS_BACK, nd_stmt);
							ndi = nd_last_scoped_stmt;
							//assert(ndi.s == NULL);
							//ndi.Insert(ama::POS_AFTER, ndi_next);
							ndi = ndi_next;
							nd_prototype_start = ndi;
							kw_mode = KW_NONE;
							nd_keyword = nullptr;
						} else {
							//nd_stmt.Insert(ama::POS_AFTER, ndi_next);
							nd_last_scoped_stmt = nd_stmt;
							nd_prototype_start = ndi_next;
							ndi = ndi_next;
							kw_mode = KW_NONE;
							nd_keyword = nullptr;
						}
						continue;
					}
					nd_prototype_start = ndi_next;
					kw_mode = KW_NONE;
					nd_keyword = nullptr;
				} else if ( (ndi->node_class == ama::N_ARRAY || ndi->isRawNode('[', ']')) && ndi_next && ndi_next->isRawNode('(', ')') ) {
					//C++11 lambda
					nd_prototype_start = ndi;
					kw_mode = KW_NONE;
					nd_keyword = nullptr;
				} else if (ndi->node_class == ama::N_SCOPE && nd_prototype_start && nd_prototype_start != ndi) {
					switch ( kw_mode ) {
						case KW_CLASS:{
							//search for the class name
							ama::Node* nd_class_name{};
							for (ama::Node* ndj = nd_keyword->s; ndj != ndi; ndj = ndj->s) {
								if ( (ndj->node_class == ama::N_SYMBOL || ndj->node_class == ama::N_REF) && keywords_after_class_name--->get(ndj->data) && nd_class_name ) {
									break;
								}
								if ( ndj->node_class == ama::N_REF || ((ndj->node_class == ama::N_CALL || ndj->node_class == ama::N_CALL_TEMPLATE) && !ndj->GetName().empty()) ) {
									if ( struct_can_be_type_prefix && ndj->node_class == ama::N_CALL ) {
										//C struct-derived-type-returning function: struct foo bar(){}
										ama::gcstring keyword = nd_keyword->GetName();
										if ( keyword == "struct" || keyword == "union" ) {
											kw_mode = KW_NONE;
											nd_keyword = nullptr;
											goto its_actually_a_function;
										}
									}
									nd_class_name = ndj;
								}
							}
							if ( !nd_class_name ) {
								//C mode switch: struct foo{union{int bar;float baz;}}
								//it's not a declaration
								kw_mode = KW_NONE;
								nd_keyword = nullptr;
								goto not_declaration;
							}
							if ( nd_keyword->node_class == ama::N_CALL ) {
								nd_keyword = ama::UnparseCall(nd_keyword);
							}
							if ( nd_class_name->node_class == ama::N_CALL || nd_class_name->node_class == ama::N_CALL_TEMPLATE ) {
								nd_class_name = ama::UnparseCall(nd_class_name);
							}
							ama::Node* nd_class = nd_keyword->ReplaceUpto(
								ndi,
								ama::CreateNode(ama::N_CLASS, nullptr)->setData(nd_keyword->data)->setCommentsBefore(nd_keyword->comments_before)->setIndent(nd_keyword->indent_level)
							);
							ndi->BreakSelf();
							ama::Node* nd_after = ama::toSingleNode(nd_class_name->BreakSibling());
							nd_class_name->BreakSelf();
							ama::Node* nd_before = ama::toSingleNode(nd_keyword->s);
							ama::ConvertToScope(ndi);
							nd_class->Insert(
								ama::POS_FRONT,
								ama::cons(nd_before, ama::cons(nd_class_name, ama::cons(nd_after, ndi)))
							);
							ndi = nd_class;
							nd_prototype_start = ndi->s;
							ndi_next = ndi->s;
							nd_keyword->s = nullptr;
							nd_keyword->p = nullptr;
							nd_keyword->FreeASTStorage();
							nd_keyword = nullptr;
							break;
						}
						case KW_STMT:{
							//pull keyword out of raw, separate "condition" (even if it's empty) and scope
							if ( nd_keyword->node_class == ama::N_CALL ) {
								nd_keyword = ama::UnparseCall(nd_keyword);
							}
							ama::Node* nd_stmt = nd_keyword->ReplaceUpto(
								ndi,
								ama::CreateNode(ama::N_SSTMT, nullptr)->setData(nd_keyword->data)->setCommentsBefore(nd_keyword->comments_before)->setIndent(nd_keyword->indent_level)
							);
							ama::Node* nd_arg = nd_keyword->s == ndi ? ama::nAir() : ama::toSingleNode(nd_keyword->s);
							nd_arg->MergeCommentsBefore(nd_keyword);
							ndi->BreakSelf();
							ama::ConvertToScope(ndi);
							nd_stmt->Insert(ama::POS_FRONT, ama::cons(nd_arg, ndi));
							ndi = nd_stmt;
							nd_prototype_start = ndi->s;
							nd_keyword->s = nullptr;
							nd_keyword->FreeASTStorage();
							////////
							//a=b;while(foo){} case - we could have mistaken the statement as an extension clause and didn't delimit at the previous ; / {}
							//delimit it
							ama::Node* nd_prev = ndi->Prev();
							if ( nd_prev && (nd_prev->node_class == ama::N_SCOPE || nd_prev->node_class == ama::N_SSTMT || nd_prev->node_class == ama::N_KSTMT || 
							nd_prev->isSymbol(";") || nd_prev->isSymbol(",")) ) {
								ama::Node* nd_last_stmt_head = nd_raw->c;
								nd_last_stmt_head->ReplaceUpto(nd_prev, nullptr);
								nd_last_stmt_head = nd_last_stmt_head->toSingleNode();
								nd_last_stmt_head->AdjustIndentLevel(nd_raw->indent_level);
								std::swap(nd_last_stmt_head->comments_before, nd_raw->comments_before);
								nd_raw->Insert(ama::POS_BEFORE, nd_last_stmt_head);
								if ( nd_prev->isSymbol(";") || (nd_prev->isSymbol(",") && nd_last_stmt_head != nd_prev) ) {
									nd_prev->Unlink();
									//nd_prev.indent_level = nd_last_stmt_head.indent_level;
									nd_raw->Insert(ama::POS_BEFORE, nd_prev);
								}
							}
							////////
							nd_last_scoped_stmt = nd_stmt;
							////////
							ndi = ndi->s;
							kw_mode = KW_NONE;
							nd_keyword = nullptr;
							continue;
						}
						case KW_EXT:{
							if ( nd_keyword->node_class == ama::N_CALL ) {
								nd_keyword = ama::UnparseCall(nd_keyword);
							}
							ama::DeleteChildRange(nd_keyword, ndi);
							assert(nd_last_scoped_stmt);
							ama::Node* nd_arg = nd_keyword->s == ndi ? ama::nAir() : ama::toSingleNode(nd_keyword->s);
							nd_arg->MergeCommentsBefore(nd_keyword);
							ndi->BreakSelf();
							//insert into the last statement
							ama::ConvertToScope(ndi);
							nd_last_scoped_stmt->Insert(ama::POS_BACK, ama::nExtensionClause(
								nd_keyword->data, nd_arg, ndi
							)->setCommentsBefore(nd_keyword->comments_before)->setIndent(nd_keyword->indent_level));
							nd_keyword->s = nullptr;
							nd_keyword->FreeASTStorage();
							ndi = nd_last_scoped_stmt;
							ndi = ndi->s;
							nd_prototype_start = ndi;
							kw_mode = KW_NONE;
							nd_keyword = nullptr;
							continue;
						}
						case KW_NOT_FUNC:{
							//it's not a function
							break;
						}
						case KW_FUNC: default:{
							its_actually_a_function:
							//it's not necessarily a declaration, detect C / C++11 / Java / etc. keywordless functions
							//what else could it be? C/C++ initializer list, new (which could have (){} for JC)
							//use (){} as indicator: we could drag back those misidentified functions later
							//if we had a keyword, we won't have a C-like return type
							ama::Node* nd_paramlist_start = nd_prototype_start;
							if ( nd_keyword ) {
								nd_paramlist_start = nd_keyword->s;
								if ( nd_keyword->node_class == ama::N_CALL ) {
									//the unparsing always creates a paramlist
									int start_was_paramlist = nd_keyword == nd_prototype_start;
									nd_keyword = ama::UnparseCall(nd_keyword);
									if ( start_was_paramlist ) {
										nd_prototype_start = nd_keyword;
									}
									nd_paramlist_start = nd_keyword->s;
								}
							}
							ama::Node* nd_paramlist = nullptr;
							//int is_unwrapped_parameter_list = 0;
							for (ama::Node* ndj = nd_paramlist_start; ndj != ndi; ndj = ndj->s) {
								if ( ndj->isRawNode('(', ')') || ndj->node_class == ama::N_CALL ) {
									nd_paramlist = ndj;
								} else if ( (ndj->node_class == ama::N_SYMBOL || ndj->node_class == ama::N_REF) && keywords_after_prototype--->get(ndj->data) ) {
									if (nd_paramlist) {
										break;
									}
									if (ndj->node_class == ama::N_SYMBOL && ndj != nd_paramlist_start && ndj->Prev()->node_class == ama::N_REF && 
									!(nd_raw->p && (nd_raw->p->node_class == ama::N_ARRAY || nd_raw->p->isRawNode('[', ']') || nd_raw->p->isRawNode('{', '}') || nd_raw->p->node_class == ama::N_SCOPE)) &&
									!(nd_raw && (nd_raw->node_class == ama::N_ARRAY || nd_raw->isRawNode('[', ']') || nd_raw->isRawNode('{', '}') || nd_raw->node_class == ama::N_SCOPE))) {
										//for function-indicator symbols like the Javascript '=>'
										//this conflicts with destructuring - {a:{b}}
										//don't count as function if it's immediately under {} or []
										nd_paramlist = ndj->Prev();
										break;
									}
								} else if ( parse_cpp11_lambda && (ndj->node_class == ama::N_ARRAY || ndj->isRawNode('[', ']')) && ndj->s->isRawNode('(', ')') ) {
									nd_paramlist = ndj->s;
									break;
								}
							}
							if ( !nd_paramlist ) {
								goto not_declaration;
							}
							if ( nd_paramlist->node_class == ama::N_CALL ) {
								int start_was_paramlist = nd_prototype_start == nd_paramlist;
								nd_paramlist = ama::UnparseCall(nd_paramlist);
								if ( start_was_paramlist ) {
									nd_prototype_start = nd_paramlist;
								}
								nd_paramlist = nd_paramlist->s;
							}
							//keyworded function shouldn't have any junk before... but C++ function has both junk before and junk after
							//before (C++ return type plus junk plus name, or just a name), paramlist, after, body, figure out return type separately
							ama::Node* nd_func = nd_prototype_start->ReplaceUpto(
								ndi,
								ama::CreateNode(ama::N_FUNCTION, nullptr)
							);
							ama::Node* nd_before = nd_prototype_start;
							if ( nd_before == nd_paramlist ) {
								nd_before = ama::nAir();
							}
							ndi->BreakSelf();
							ama::Node* nd_after = nd_paramlist->BreakSibling();
							nd_paramlist->BreakSelf();
							if ( !nd_after ) {
								nd_after = ama::nAir();
							}
							nd_paramlist = ConvertToParameterList(nd_paramlist);
							//if (is_unwrapped_parameter_list) {
							//	nd_paramlist->flags = ama::PARAMLIST_UNWRAPPED;
							//}
							nd_before = ama::toSingleNode(nd_before);
							nd_after = ama::toSingleNode(nd_after);
							ama::ConvertToScope(ndi);
							nd_func->Insert(ama::POS_FRONT, ama::cons(nd_before, ama::cons(nd_paramlist, ama::cons(nd_after, ndi))));
							//nd_func.data = GetFunctionName(kw_mode == KW_FUNC, nd_func);
							ndi = nd_func;
							nd_prototype_start = ndi->s;
							ndi_next = ndi->s;
							break;
						}
					}
					kw_mode = KW_NONE;
					nd_keyword = nullptr;
					not_declaration:;
				}
				if ( !(kw_mode == KW_EXT && nd_keyword) ) {
					nd_last_scoped_stmt = nullptr;
				}
				ndi = ndi_next;
			}
			if ( nd_keyword && (kw_mode == KW_STMT || kw_mode == KW_EXT) ) {
				//C unscoped statement
				if ( nd_raw->s && nd_raw->s->isSymbol(";") ) {
					//pull in the misclassified ";"
					ama::Node* nd_semicolon = nd_raw->s;
					nd_semicolon->Unlink();
					nd_raw->Insert(ama::POS_BACK, nd_semicolon);
				}
				ama::Node* ndi = nd_raw->LastChild();
				ama::Node* nd_stmt{};
				if ( kw_mode == KW_EXT && nd_last_scoped_stmt && (nd_keyword->GetName() == "while" || nd_keyword->GetName() == "until") ) {
					nd_stmt = TranslateDoWhileClause(nd_keyword, ndi);
				} else {
					nd_stmt = TranslateCUnscopedStatement(Q, nd_keyword, ndi, is_elseif);
					//if (is_elseif) {
					//	//re-queue the if part
					//	ama::Node* nd_elseif = nd_stmt->LastChild();
					//	//the counter-example where it's not N_RAW: `#define for if (0) {} else for`
					//	if (nd_elseif->node_class == ama::N_RAW) {
					//		Q.push_back(nd_elseif);
					//	}
					//}
				}
				if ( kw_mode == KW_EXT && nd_last_scoped_stmt ) {
					nd_stmt->node_class = ama::N_EXTENSION_CLAUSE;
					nd_stmt->Unlink();
					nd_last_scoped_stmt->Insert(ama::POS_BACK, nd_stmt);
				}
			} else if ( kw_mode == KW_FUNC && nd_keyword && parse_c_forward_declarations && 
			nd_raw->isRawNode(0, 0) && nd_raw->p && (nd_raw->p->node_class == ama::N_SCOPE || nd_raw->p->node_class == ama::N_FILE) ) {
				//C forward declaration with extern
				TranslateCForwardDeclaration(nd_raw);
				//note that extern is not necessarily function... we may have extern "C"{}
			} else if ( nd_keyword && kw_mode == KW_CLASS && !(nd_raw->isRawNode('<', '>')) && !(nd_raw->p && nd_raw->p->isRawNode('<', '>')) &&
			!(nd_raw->p && nd_raw->p->node_class == ama::N_PARAMETER_LIST)) {
				//class forward declaration, treat as keyword statement
				//but not inside template<>
				nd_keyword->BreakSelf();
				ama::Node* nd_stmt = ama::CreateNode(ama::N_KSTMT, ama::toSingleNode(nd_keyword->BreakSibling()));
				nd_stmt->indent_level = nd_keyword->indent_level;
				nd_stmt->comments_before = nd_keyword->comments_before;
				if ( nd_stmt->c ) {
					nd_stmt->c->MergeCommentsBefore(nd_keyword);
				} else {
					nd_stmt->comments_after = (nd_keyword->comments_after + nd_stmt->comments_after);
				}
				nd_stmt->data = nd_keyword->DestroyForSymbol();
				ndi = nd_raw->Insert(ama::POS_BACK, nd_stmt);
			}
			/////////////////////
			//split / remove nd_raw based on parsed scoped stuff
			if (nd_raw->isRawNode(0, 0) && nd_raw->p && (
			nd_raw->p->node_class == ama::N_SCOPE || nd_raw->p->node_class == ama::N_FILE ||
			nd_raw->p->node_class == ama::N_ARRAY || nd_raw->p->node_class == ama::N_OBJECT ||
			nd_raw->p->node_class == ama::N_RAW)) {
				for (ama::Node* ndi = nd_raw->c; ndi; ndi = ndi->s) {
					again:
					if (ndi->node_class == ama::N_SSTMT || ndi->node_class == ama::N_KSTMT) {
						ama::Node* nd_stmt = ndi->BreakSelf();
						ama::Node* nd_next = ndi->BreakSibling();
						nd_stmt->AdjustIndentLevel(nd_raw->indent_level);
						nd_raw->Insert(ama::POS_AFTER, nd_stmt);
						if (nd_next) {
							nd_next = nd_next->toSingleNode();
							nd_next->AdjustIndentLevel(nd_raw->indent_level);
							nd_stmt->Insert(ama::POS_AFTER, nd_next);
							nd_next->comments_after = nd_next->comments_after + nd_raw->comments_after;
							nd_raw->comments_after = "";
						} else {
							nd_stmt->comments_after = nd_stmt->comments_after + nd_raw->comments_after;
							nd_raw->comments_after = "";
						}
						//eagerly drop dummy raw
						if (!nd_raw->c) {
							nd_stmt->comments_before = nd_raw->comments_before + nd_stmt->comments_before;
							nd_raw->Unlink();
						}
						if (nd_next && nd_next->isRawNode(0, 0)) {
							ndi = nd_next->c;
							goto again;
						} else {
							break;
						}
					}
				}
			}
		}
		//C forward declaration
		if ( parse_c_forward_declarations ) {
			for ( ama::Node * nd_raw: nd_root->FindAllWithin(0, ama::N_RAW) ) {
				//have to rely on context? we can do Owner tests here 
				ama::Node* nd_owner = nd_raw->Owner();
				if ( !(nd_raw->p && (nd_raw->p->node_class == ama::N_SCOPE || nd_raw->p->node_class == ama::N_FILE)) || nd_owner->node_class == ama::N_FUNCTION ) { continue; }
				if ( !nd_raw->isRawNode(0, 0) ) { continue; }
				ama::Node* nd_proto{};
				for (ama::Node* ndi = nd_raw->c; ndi; ndi = ndi->s) {
					if ( (ndi->node_class == ama::N_REF && keywords_not_a_function--->get(ndi->data)) || ndi->isSymbol("=") ) {
						nd_proto = nullptr;
						break;
					}
					if ( ndi->node_class == ama::N_CALL ) {
						nd_proto = ndi;
					}
					if ( ndi->node_class == ama::N_SCOPE ) {
						nd_proto = nullptr;
						break;
					}
				}
				if ( !nd_proto ) { continue; }
				if ( !nd_raw->c->s && nd_proto->c->node_class != ama::N_CALL ) {
					//ignore just-a-call, but allow::
					//FOO_DECL(int,foo)(int bar);
					continue;
				}
				TranslateCForwardDeclaration(nd_raw);
			}
		}
		/////////
		//don't set REF_WRITTEN for function / class names: it's not profitable to treat them as "written" in our current AST formulation
		//turn params into N_MOV
		for ( ama::Node * nd_paramlist: nd_root->FindAllWithin(0, ama::N_PARAMETER_LIST)--->concat(nd_root->FindAllWithin(0, ama::N_CALL_TEMPLATE, "template")) ) {
			for (ama::Node* nd_param = nd_paramlist->node_class == ama::N_CALL_TEMPLATE ? nd_paramlist->c->s : nd_paramlist->c; nd_param; nd_param = nd_param->s) {
				if ( nd_param->node_class == ama::N_MOV ) { continue; }
				//if ( nd_param->node_class == ama::N_SYMBOL || (nd_param->node_class == ama::N_RAW && nd_param->c && nd_param->c->node_class == ama::N_SYMBOL) ) {
				//	//rest args
				//	continue;
				//}
				ama::Node* nd_tmp = ama::GetPlaceHolder();
				nd_param->ReplaceWith(nd_tmp);
				nd_param = nd_tmp->ReplaceWith(ama::nMov(nd_param, ama::nAir()));
			}
		}
		return nd_root;
	}
	ama::Node* ParseKeywordStatements(ama::Node* nd_root, JSValue options) {
		std::unordered_map<ama::gcstring, int> keywords_statement = ama::GetPrioritizedList(options, "keywords_statement");
		for ( ama::Node * nd_keyword: nd_root->FindAllWithin(0, ama::N_REF) ) {
			if ( !keywords_statement--->get(nd_keyword->data) ) { continue; }
			ama::Node* nd_parent = nd_keyword->p;
			if ( !nd_parent ) { continue; }
			if ( nd_parent->node_class == ama::N_SCOPE || nd_parent->node_class == ama::N_FILE ) {
				nd_keyword->node_class = ama::N_KSTMT;
				nd_keyword->Insert(ama::POS_FRONT, ama::nAir());
			} else if ( nd_parent->node_class == ama::N_RAW ) {
				if ( nd_keyword->data == "template" && nd_keyword->s && nd_keyword->s->isRawNode('<', '>') ) {
					//template<foo>bar => nScopedStatement('template',nParameterList(foo),bar)
					//we haven't parsed assignment yet so template alias will be seen as N_RAW
					ama::Node* nd_paramlist = nd_keyword->s;
					ama::Node* nd_last = nd_paramlist;
					while ( nd_last->s && !nd_last->s->isSymbol(";") && !nd_last->s->isSymbol(",") && nd_last->node_class != ama::N_SCOPE ) {
						nd_last = nd_last->s;
					}
					ama::Node* nd_tmp = ama::GetPlaceHolder();
					nd_keyword->ReplaceUpto(nd_last, nd_tmp);
					//////////
					ama::Node* nd_body = ama::toSingleNode(nd_paramlist->BreakSibling());
					ama::Node* nd_stmt = ama::CreateNode(ama::N_SSTMT, ama::cons(ConvertToParameterList(nd_paramlist), nd_body));
					nd_stmt->indent_level = nd_keyword->indent_level;
					nd_stmt->comments_before = nd_keyword->comments_before;
					if ( nd_stmt->c ) {
						nd_stmt->c->MergeCommentsBefore(nd_keyword);
					} else {
						nd_stmt->comments_after = (nd_keyword->comments_after + nd_stmt->comments_after);
					}
					nd_stmt->data = nd_keyword->DestroyForSymbol();
					nd_tmp->ReplaceWith(nd_stmt);
					continue;
				}
				ama::Node* nd_raw = nd_parent;
				ama::Node* nd_tmp = ama::GetPlaceHolder();
				ama::Node* nd_last = nd_keyword;
				while ( nd_last->s && !nd_last->s->isSymbol(";") && !nd_last->s->isSymbol(",") ) {
					nd_last = nd_last->s;
				}
				nd_keyword->ReplaceUpto(nd_last, nd_tmp);
				ama::Node* nd_stmt = ama::CreateNode(ama::N_KSTMT, ama::toSingleNode(nd_keyword->BreakSibling()));
				nd_stmt->indent_level = nd_keyword->indent_level;
				nd_stmt->comments_before = nd_keyword->comments_before;
				if ( nd_stmt->c ) {
					nd_stmt->c->MergeCommentsBefore(nd_keyword);
				} else {
					nd_stmt->comments_after = (nd_keyword->comments_after + nd_stmt->comments_after);
				}
				nd_stmt->data = nd_keyword->DestroyForSymbol();
				nd_tmp->ReplaceWith(nd_stmt);
				if (nd_stmt->data == "#define" ) {
					if (nd_stmt->c->node_class != ama::N_RAW) {
						ama::Node* nd_breakpt = nd_stmt->c;
						for (; ;) {
							if (!nd_breakpt || nd_breakpt->node_class == ama::N_REF || (
								nd_breakpt->node_class == ama::N_CALL &&
								nd_breakpt->c->node_class == ama::N_REF &&
								nd_breakpt->c->comments_after == ""
							)) {
								break;
							}
							ama::Node* nd_breakpt_unparsed = nd_breakpt->Unparse();
							if (nd_breakpt_unparsed == nd_breakpt) {
								//can't unparse anymore
								break;
							}
							nd_breakpt = nd_breakpt_unparsed;
						}
					}
					if (nd_stmt->c->node_class == ama::N_RAW) {
						//#define argument special handling: break the argument part
						ama::Node* nd_define_args = nd_stmt->c;
						ama::Node* nd_breakpt = nd_define_args->c;
						for (; ;) {
							if (!nd_breakpt || nd_breakpt->node_class == ama::N_REF || (
								nd_breakpt->node_class == ama::N_CALL &&
								nd_breakpt->c->node_class == ama::N_REF &&
								nd_breakpt->c->comments_after == ""
							)) {
								break;
							}
							ama::Node* nd_breakpt_unparsed = nd_breakpt->Unparse();
							if (nd_breakpt_unparsed == nd_breakpt) {
								//can't unparse anymore
								break;
							}
							nd_breakpt = nd_breakpt_unparsed;
						}
						if (nd_breakpt && nd_breakpt->node_class == ama::N_REF) {
							nd_breakpt->flags |= ama::REF_DECLARED;
						}
						if (nd_breakpt && nd_breakpt->node_class == ama::N_REF && nd_breakpt->s && nd_breakpt->s->isRawNode('(', ')')) {
							nd_breakpt = nd_breakpt->s;
						}
						if (nd_breakpt) {
							ama::Node* nd_value = nd_breakpt->BreakSibling()->toSingleNode();
							nd_define_args->Insert(ama::POS_BACK, nd_value);
							nd_value->AdjustIndentLevel(-nd_define_args->indent_level);
						}
					}
				}
			}
		}
		return nd_root;
	}
	static void FixTypeSuffixFromInnerRef(std::unordered_map<ama::gcstring, int> const& ambiguous_type_suffix, ama::Node* nd_ref) {
		while ( (nd_ref->p->node_class == ama::N_ITEM || nd_ref->p->node_class == ama::N_CALL) && nd_ref == nd_ref->p->c ) {
			nd_ref = nd_ref->p;
		}
		if ( nd_ref->p->node_class == ama::N_BINOP && nd_ref->p->c->s == nd_ref && ambiguous_type_suffix--->get(nd_ref->p->data) ) {
			ama::UnparseBinop(nd_ref->p);
		}
		while ( nd_ref->Prev() && nd_ref->Prev()->node_class == ama::N_SYMBOL && ambiguous_type_suffix--->get(nd_ref->Prev()->data) && nd_ref->Prev()->Prev() ) {
			ama::Node* nd_opr = nd_ref->Prev();
			ama::Node* ndi = nd_opr->Prev();
			ndi->MergeCommentsAfter(nd_opr);
			nd_opr->Unlink();
			ama::Node* nd_tmp = ama::GetPlaceHolder();
			ndi->ReplaceWith(nd_tmp);
			std::swap(ndi->comments_after, nd_tmp->comments_after);
			ndi = nd_tmp->ReplaceWith(ama::nPostfix(ndi, nd_opr->data)->setCommentsAfter(nd_opr->comments_after));
			nd_opr->FreeASTStorage();
		}
	}
	//detects N_TYPED_VAR
	//also sets REF_WRITTEN and REF_RW
	//after ParseAssignment
	ama::Node* ParseDeclarations(ama::Node* nd_root, JSValue options) {
		//find declaratives and set REF_DECLARED
		//also find writes and set REF_WRITTEN
		std::unordered_map<ama::gcstring, int> ambiguous_type_suffix = ama::GetPrioritizedList(options, "ambiguous_type_suffix");
		std::unordered_map<ama::gcstring, int> keywords_class = ama::GetPrioritizedList(options, "keywords_class");
		std::unordered_map<ama::gcstring, int> keywords_function = ama::GetPrioritizedList(options, "keywords_function");
		std::unordered_map<ama::gcstring, int> keywords_not_variable_name = ama::GetPrioritizedList(options, "keywords_not_variable_name");
		std::unordered_map<ama::gcstring, int> keywords_operator_escape = ama::GetPrioritizedList(options, "keywords_operator_escape");
		int32_t parse_cpp_declaration_initialization = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_cpp_declaration_initialization"), 1);
		//pull types out of comma
		for ( ama::Node * nd_comma: nd_root->FindAllWithin(0, ama::N_COMMA) ) {
			if (nd_comma->c && nd_comma->c->isRawNode(0, 0) && nd_comma->c->c && nd_comma->c->c->s) {
				//could be C-like declaration, rotate the "type" out of the comma
				ama::Node* nd_1st_decl = nd_comma->c;
				ama::Node* nd_var = nd_1st_decl->LastChild();
				nd_var->comments_after = nd_var->comments_after + nd_1st_decl->comments_after;
				nd_1st_decl->comments_after = "";
				nd_var->Unlink();
				int32_t indent_comma = nd_comma->indent_level;
				int32_t indent_var = nd_var->indent_level;
				int32_t indent_raw = nd_1st_decl->indent_level;
				ama::Node* nd_tmp = ama::GetPlaceHolder();
				nd_comma->ReplaceWith(nd_tmp);
				nd_1st_decl->ReplaceWith(nd_var);
				nd_comma->AdjustIndentLevel(-nd_1st_decl->indent_level);
				nd_1st_decl->Insert(ama::POS_BACK, nd_comma);
				nd_tmp->ReplaceWith(nd_1st_decl);
				nd_var->indent_level = 0;
				nd_1st_decl->indent_level = ama::ClampIndentLevel(indent_comma + indent_raw);
				nd_comma->indent_level = indent_var;
			}
		}
		for ( ama::Node * nd_ref: nd_root->FindAllWithin(0, ama::N_REF) ) {
			if ( nd_ref->p->node_class == ama::N_CLASS && nd_ref->p->c->s == nd_ref ) {
				//class, just declared and nothing else
				nd_ref->flags |= ama::REF_DECLARED;
				continue;
			}
			if (nd_ref->p->node_class == ama::N_KSTMT && nd_ref->p->c == nd_ref && keywords_class--->get(nd_ref->p->data, 0)) {
				//forward declaration
				nd_ref->flags |= ama::REF_DECLARED;
				continue;
			}
			ama::Node* nd_stmt = nd_ref->ParentStatement();
			ama::Node* nd_asgn = nd_ref;
			while (nd_asgn && nd_asgn->node_class != ama::N_MOV) {
				if (nd_asgn->node_class == ama::N_DOT || nd_asgn->node_class == ama::N_ITEM || nd_asgn->node_class == ama::N_CALL) {
					nd_asgn = nullptr;
					break;
				}
				nd_asgn = nd_asgn->p;
			}
			ama::Node* nd_owner = nd_ref->Owner();
			if ( nd_stmt->p && nd_stmt->p->node_class == ama::N_SCOPE && nd_stmt->p->p && nd_stmt->p->p->node_class == ama::N_SSTMT && nd_stmt->p->p->data == "enum" && 
			!(nd_stmt->node_class == ama::N_MOV && nd_stmt->c->s->isAncestorOf(nd_ref)) ) {
				nd_ref->flags |= ama::REF_DECLARED;
			}
			if ( nd_asgn && nd_stmt->isAncestorOf(nd_asgn) && nd_asgn->c->isAncestorOf(nd_ref) ) {
				//Go := operator or general assignment
				if ( nd_asgn->data == ":" || nd_asgn->data == "" ) {
					if ( nd_ref == nd_asgn->c ) {
						if ( nd_asgn->p && (
							nd_asgn->p->node_class == ama::N_SCOPE && nd_asgn->p->p && nd_asgn->p->p->node_class == ama::N_SSTMT && nd_asgn->p->p->data == "enum" ||
							nd_asgn->p->node_class == ama::N_PARAMETER_LIST
						)) {
							nd_ref->flags |= ama::REF_WRITTEN | ama::REF_DECLARED;
						} else {
							nd_ref->flags |= ama::REF_WRITTEN;
						}
					}
					if ( nd_asgn->data == ":" ) {
						//Go := operator
						FixTypeSuffixFromInnerRef(ambiguous_type_suffix, nd_ref);
						nd_ref->flags |= ama::REF_WRITTEN | ama::REF_DECLARED;
					} else {
						//destructuring case: `[foo]=...`
						ama::Node* nd_destructuring = nd_ref;
						int destructured = 0;
						while ( nd_destructuring != nd_asgn ) {
							if ( nd_destructuring->node_class == ama::N_SCOPE || nd_destructuring->isRawNode('[', ']') || nd_destructuring->isRawNode('{', '}') || nd_destructuring->isRawNode('(', ')') ) {
								destructured = 1;
								break;
							}
							if ( nd_destructuring->p && nd_destructuring->p->node_class == ama::N_LABELED && nd_destructuring == nd_destructuring->p->c ) {
								//used as a label in JS destructuring, drop it
								destructured = 0;
								break;
							}
							nd_destructuring = nd_destructuring->p;
						}
						if ( destructured ) {
							FixTypeSuffixFromInnerRef(ambiguous_type_suffix, nd_ref);
							nd_ref->flags |= ama::REF_WRITTEN | ama::REF_DECLARED;
						}
					}
					//could be a C initialized declaration, defer for now
				} else {
					//updating assignment
					nd_ref->flags |= ama::REF_WRITTEN | ama::REF_RW;
				}
			} else if ( nd_stmt->p->node_class == ama::N_SCOPE && nd_owner && nd_asgn && nd_asgn->data.empty() && nd_owner->isAncestorOf(nd_asgn) && nd_asgn->c->isAncestorOf(nd_stmt) ) {
				//could be {} nd_destructuring, here nd_stmt is BS
				//`type {...,foo,...}=...; {"bar":foo}=...;`
				ama::Node* nd_destructuring = nd_ref;
				int destructured = 0;
				while ( nd_destructuring != nd_stmt ) {
					if ( nd_destructuring->node_class == ama::N_SCOPE || 
					nd_destructuring->node_class == ama::N_ARRAY || nd_destructuring->isRawNode('[', ']') || 
					nd_destructuring->isRawNode('{', '}') || nd_destructuring->isRawNode('(', ')') ) {
						destructured = 1;
						break;
					}
					if ( nd_destructuring->p && nd_destructuring->p->node_class == ama::N_LABELED && nd_destructuring == nd_destructuring->p->c ) {
						//used as a label in JS destructuring, drop it
						destructured = 0;
						break;
					}
					nd_destructuring = nd_destructuring->p;
				}
				if ( destructured ) {
					nd_ref->flags |= ama::REF_WRITTEN | ama::REF_DECLARED;
				}
			}
			//Rust / JS / Go var / let keyword
			//C/C++ `type foo, *foo[bar], foo=bar, foo(bar), foo{bar,baz};`
			//C++/JS LHS {} destructuring
			ama::Node* nd_cdecl = nd_ref;
			ama::Node* nd_to_fix_core = nullptr;
			int32_t is_array_assignment = 0;
			int32_t got_type = 0;
			while ( nd_cdecl && nd_cdecl != nd_stmt ) {
				if ( nd_cdecl->p->isRawNode(0, 0) ) {
					ama::Node* nd_core = nd_cdecl;
					nd_cdecl = nd_cdecl->p;
					if ( !nd_core->s || nd_core->s->isSymbol(",") || nd_core->s->isSymbol(";")) {
						//it could be a declarative raw
						got_type |= (nd_cdecl->c != nd_core);
					} else {
						//don't go to that raw
						nd_cdecl = nd_core;
					}
					//assume N_ITEM and N_CALL as C array declaration `baz foo[bar]`
					is_array_assignment = 0;
					break;
				}
				if (parse_cpp_declaration_initialization && (nd_cdecl->p->node_class == ama::N_ITEM || nd_cdecl->p->node_class == ama::N_CALL) && nd_cdecl == nd_cdecl->p->c) {
					//could be either an array assignment `foo[bar]=baz;` or a C array declaration `baz foo[bar]`
					//the same applies for `foo(bar)=baz` vs `baz foo(bar)`
					is_array_assignment = 1;
				} else if (parse_cpp_declaration_initialization && (  
				(nd_cdecl->p->node_class == ama::N_BINOP && ambiguous_type_suffix--->get(nd_cdecl->p->data) && nd_cdecl == nd_cdecl->p->c->s) || 
				nd_cdecl->p->node_class == ama::N_PREFIX) ) {
					//foo* bar
					//*bar
					//the mis-parsed part counts as type
					if (!nd_to_fix_core) {
						nd_to_fix_core = nd_cdecl;
					}
					got_type |= 1;
				} else if (parse_cpp_declaration_initialization && nd_cdecl->p->isRawNode('(', ')')) {
					//(*foo)[bar]
				} else if (parse_cpp_declaration_initialization && nd_cdecl->p->node_class == ama::N_TYPED_OBJECT) {
					//foo{bar}
				} else if (nd_cdecl->p->node_class == ama::N_MOV && nd_cdecl->p->c == nd_cdecl) {
					//foo=bar
				} else if (nd_cdecl->p->node_class == ama::N_COMMA && nd_cdecl->p->p && nd_cdecl->p->p->node_class == ama::N_RAW) {
					//the last multi-var comma layer
				} else {
					break;
				}
				nd_cdecl = nd_cdecl->p;
			}
			if ( nd_cdecl != nd_ref && !(nd_ref->s && nd_ref->s->node_class == ama::N_REF) && !keywords_not_variable_name[nd_ref->data]) {
				//we found at least one feasible declaration-ish
				//check parent
				int is_ok = 0;
				if ( nd_cdecl->p && nd_cdecl->p->node_class == ama::N_LABELED && nd_cdecl->p->c == nd_cdecl ) {
					ama::Node* nd_loop = nd_cdecl->Owning(ama::N_SSTMT);
					if ( nd_loop && nd_loop->data--->startsWith("for") && (nd_stmt->isAncestorOf(nd_loop) || nd_loop->c->isAncestorOf(nd_stmt)) && nd_loop->c->isAncestorOf(nd_cdecl) ) {
						//foo in `for(foo:bar)`
						is_ok = 1;
					}
				}
				while ( nd_cdecl->p && nd_cdecl->p->node_class == ama::N_LABELED && nd_cdecl->p->c->s == nd_cdecl ) {
					//public: int a;
					nd_cdecl = nd_cdecl->p;
				}
				if ( nd_cdecl == nd_stmt && got_type) {
					//foo in `type foo;` or `type bar,*foo[8];`
					is_ok = 1;
				} else if ( ama::isUnderParameter(nd_cdecl) && got_type) {
					//foo in `type foo;` or `type bar,*foo[8];`
					is_ok = 1;
				} else if ( nd_cdecl->p && nd_cdecl->p->node_class == ama::N_FUNCTION && nd_cdecl->p->c == nd_cdecl ) {
					//non-dotted function declaration, handle it here, also name the function
					is_ok = 1;
					if ( nd_cdecl->p->data.empty() ) {
						nd_cdecl->p->data = nd_ref->data;
					}
				}
				if ( is_ok ) {
					if (!nd_to_fix_core) {nd_to_fix_core = nd_ref;}
					FixTypeSuffixFromInnerRef(ambiguous_type_suffix, nd_to_fix_core);
					nd_ref->flags |= ama::REF_WRITTEN | ama::REF_DECLARED;
				}
			}
		}
		//detect type before dotted function name
		//non-dotted non-C-macro functions should have been handled above
		for ( ama::Node * nd_func: nd_root->FindAllWithin(0, ama::N_FUNCTION) ) {
			if ( !nd_func->data.empty() ) { continue; }
			//C++ dotted declaration: `type foo::bar(){}`
			//common C macro style: `FOO_DECL(int,foo)(int bar){}`
			nd_func->data = "";
			ama::Node* nd_before = nd_func->c;
			//try to find a name
			int found = 0;
			if (nd_before->node_class == ama::N_RAW) {
				for (ama::Node* ndi = nd_before->c; ndi; ndi = ndi->s) {
					//we could have mistaken it for binop due to type shenanigans
					ama::Node* ndj = ndi;
					while ( ndj->node_class == ama::N_BINOP && ambiguous_type_suffix--->get(ndj->data) ) {
						ndj = ndj->c->s;
					}
					if ( ndj->node_class == ama::N_REF || ndj->node_class == ama::N_DOT ) {
						//the starting keyword doesn't count
						//if it appears after a non-key word, we are likely in the wrong language so take the name
						if ( !found && keywords_function--->get(ndj->data) ) { continue; }
						if (keywords_operator_escape--->get(ndj->data, 0) && ndj->s) {
							//it's `operator`, check for symbol / brackets
							if (ndj->s->node_class == ama::N_SYMBOL && ndj->s->data != "::") {
								nd_func->data = ndj->s->data;
								found = 1;
								continue;
							}
							if (ndj->s->node_class == ama::N_RAW && !ndj->s->c) {
								//operator() / operator[]
								std::array<char, 2> buf{};
								buf[0] = char(ndj->s->flags & 0xff);
								buf[1] = char((ndj->s->flags >> 8) & 0xff);
								nd_func->data = ama::gcstring(buf.data(), 2);
								found = 1;
								continue;
							}
						}
						nd_func->data = ndj->data;
						found = 1;
					}
				}
			} else if (nd_before->node_class == ama::N_REF) {
				if ( !keywords_function--->get(nd_before->data) ) {
					nd_func->data = nd_before->data;
					found = 1;
				}
			} else if (nd_before->node_class == ama::N_BINOP) {
				ama::Node* ndj = nd_before;
				while ( ndj->node_class == ama::N_BINOP && ambiguous_type_suffix--->get(ndj->data) ) {
					ndj = ndj->c->s;
				}
				if ( ndj->node_class == ama::N_REF || ndj->node_class == ama::N_DOT ) {
					FixTypeSuffixFromInnerRef(ambiguous_type_suffix, ndj);
					nd_func->data = ndj->data;
					found = 1;
				}
			}
			if ( !found ) {
				//test for old C macro style `FOO_DECL(int,foo)(int bar){}`
				ama::Node* nd_maybe_c_macro = nd_before->node_class == ama::N_RAW ? nd_before : nd_before->LastChild();
				if ( nd_maybe_c_macro && (nd_maybe_c_macro->node_class == ama::N_CALL || nd_maybe_c_macro->isRawNode('(', ')')) ) {
					ama::Node* nd_maybe_name = nd_maybe_c_macro->LastChild();
					if ( nd_maybe_name && nd_maybe_name->node_class == ama::N_REF ) {
						//C macros won't have dotted name / mistaken-N_BINOP here
						nd_func->data = nd_maybe_name->data;
					}
				}
			}
		}
		//we can't possibly have types-mistaken-as-binop in casts for standard C/C++
		return nd_root;
	}
};
