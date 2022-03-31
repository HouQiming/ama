#include <vector>
#include <unordered_map>
#include "../util/jc_array.h"
#include "../ast/node.hpp"
#include "../script/jsenv.hpp"
#include "postfix.hpp"
#include "operator.hpp"
namespace ama {
	ama::Node* ParseCommaExpr(ama::Node* nd_root) {
		//things inside [] or {} don't count
		for ( ama::Node* nd_raw: nd_root->FindAllWithin(0, ama::N_RAW) ) {
			if (!(nd_raw->isRawNode(0, 0) || nd_raw->isRawNode('(', ')'))) {continue;}
			std::vector<ama::Node*> comma_children{};
			for (ama::Node* ndi = nd_raw->c; ndi; ndi = ndi->s) {
				again:
				if (ndi->isSymbol(",")) {
					if (!comma_children.size() && !ndi->s) {
						//trailing ',', ignore
						continue;
					}
					ama::Node* nd_next = ndi->BreakSibling();
					if (!nd_next) {nd_next = ama::nAir();}
					ndi->Unlink();
					ama::Node* nd_last = nd_raw->BreakChild();
					nd_raw->Insert(ama::POS_FRONT, nd_next);
					comma_children.push_back(nd_last->toSingleNode());
					ndi = nd_next;
					goto again;
				}
			}
			if (comma_children.size()) {
				comma_children.push_back(nd_raw->BreakChild()->toSingleNode());
				ama::Node* nd_comma = ama::CreateNodeFromChildren(ama::N_COMMA, comma_children);
				if (nd_raw->isRawNode('(', ')')) {
					nd_raw->Insert(ama::POS_FRONT, nd_comma);
				} else {
					nd_raw->ReplaceWith(nd_comma);
				}
			}
		}
		return nd_root;
	}
	struct ColonStackItem {
		ama::Node* nd_head{};
		ama::Node* nd_qmark{};
		ama::Node* nd_colon{};
	};
	static ama::Node* FoldColonStack(std::vector<ColonStackItem>& cstk, ama::Node* nd_next) {
		assert(cstk.back().nd_colon);
		ama::Node* nd_head = cstk.back().nd_head;
		//console.log(cstk.length, phead);
		//COULDDO: range mode
		ama::Node* nd_tmp = ama::GetPlaceHolder();
		nd_head->ReplaceUpto(nd_next ? nd_next->Prev() : nd_head->p->LastChild(), nd_tmp);
		ama::Node* nd_colon = cstk.back().nd_colon->BreakSelf();
		ama::Node* nd_ret{};
		if ( nd_head == nd_colon ) { nd_head = ama::nAir(); }
		if ( cstk.back().nd_qmark ) {
			//N_CONDITIONAL
			ama::Node* nd_qmark = cstk.back().nd_qmark->BreakSelf();
			ama::Node* nd_cond = ama::toSingleNode(nd_head);
			ama::Node* nd_true = ama::toSingleNode(nd_qmark->BreakSibling());
			ama::Node* nd_false = ama::toSingleNode(nd_colon->BreakSibling());
			nd_cond->MergeCommentsAfter(nd_qmark);
			nd_true->MergeCommentsBefore(nd_qmark);
			nd_true->MergeCommentsAfter(nd_colon);
			nd_false->MergeCommentsBefore(nd_colon);
			//if (nd_cond->node_class == ama::N_MOV) {
			//	//priority reversal
			//	ama::Node* nd_asgn = nd_cond;
			//	nd_cond = nd_asgn->c->BreakSibling();
			//	nd_asgn->Insert(ama::POS_BACK, ama::nConditional(nd_cond, nd_true, nd_false));
			//	nd_ret = nd_asgn;
			//} else {
			//	nd_ret = ama::nConditional(nd_cond, nd_true, nd_false);
			//}
			nd_ret = ama::nConditional(nd_cond, nd_true, nd_false);
			nd_qmark->p = nullptr; nd_qmark->FreeASTStorage();
			nd_colon->p = nullptr; nd_colon->FreeASTStorage();
		} else {
			//N_LABELED
			if ( nd_head->node_class == ama::N_AIR && !nd_colon->s ) {
				//standalone ':', do nothing
				nd_ret = nd_colon;
			} else {
				ama::Node* nd_label = ama::toSingleNode(nd_head);
				ama::Node* nd_value = ama::toSingleNode(nd_colon->BreakSibling());
				nd_label->MergeCommentsAfter(nd_colon);
				nd_value->MergeCommentsBefore(nd_colon);
				nd_ret = ama::nLabeled(nd_label, nd_value);
				nd_colon->p = nullptr; nd_colon->FreeASTStorage();
			}
		}
		nd_tmp->ReplaceWith(nd_ret);
		for (ama::Node* ndi = nd_ret->c; ndi; ndi = ndi->s) {
			ndi->AdjustIndentLevel(-nd_ret->indent_level);
		}
		if (nd_ret->node_class == ama::N_LABELED && nd_ret->p && nd_ret->p->p && nd_ret->p->p->node_class == ama::N_MOV && 
		nd_ret->p->p->c == nd_ret->p && !nd_ret->Prev()) {
			//do FixPriorityReversal here aggressively
			ama::Node* nd_raw = nd_ret->p;
			ama::Node* nd_asgn = nd_raw->p;
			ama::Node* nd_var = nd_ret->c->s;
			if (nd_var->node_class == ama::N_AIR && nd_ret->s) {
				//case / default
				if (nd_raw->comments_before.size()) {
					nd_ret->comments_before = nd_raw->comments_before + nd_ret->comments_before;
					nd_raw->comments_before = "";
				}
				if (nd_asgn->comments_before.size()) {
					nd_ret->comments_before = nd_asgn->comments_before + nd_ret->comments_before;
					nd_asgn->comments_before = "";
				}
				int8_t new_indent_level = ama::ClampIndentLevel(int32_t(nd_ret->indent_level) + nd_raw->indent_level + nd_asgn->indent_level);
				nd_ret->Unlink();
				nd_asgn->Insert(ama::POS_BEFORE, nd_ret);
				nd_ret->indent_level = new_indent_level;
				//need to re-sanitize the new assignment variable's comment placement
				if (!nd_raw->c) {
					nd_raw->Insert(ama::POS_FRONT, ama::nAir());
				}
				if ( nd_raw->c->comments_before.size() ) {
					nd_raw->comments_before = (nd_raw->comments_before + nd_raw->c->comments_before);
					nd_raw->c->comments_before = "";
				}
				nd_raw->AdjustIndentLevel(nd_raw->c->indent_level);
				nd_raw->c->indent_level = 0;
			} else if (nd_ret->s) {
				//not a case but we have a sibling, can't do anything
			} else {
				ama::UnparseRaw(nd_raw);
				assert(nd_ret->p == nd_asgn && nd_asgn->c == nd_ret);
				nd_var = nd_var->Unlink();
				int32_t indent_asgn = nd_asgn->indent_level;
				int32_t indent_var = nd_var->indent_level;
				int32_t indent_label = nd_ret->indent_level;
				nd_ret->ReplaceWith(nd_var);
				nd_asgn->ReplaceWith(nd_ret);
				nd_ret->Insert(ama::POS_BACK, nd_asgn);
				nd_ret->indent_level = indent_asgn;
				nd_asgn->indent_level = indent_var;
				nd_var->indent_level = 0;
				//the shenanigans above ignored the assignment value
				nd_asgn->c->s->AdjustIndentLevel(indent_asgn - indent_var);
			}
		}
		cstk--->pop();
		if ( !cstk.size() ) {
			cstk--->push(ColonStackItem{.nd_head = nd_ret, .nd_qmark = nullptr, .nd_colon = nullptr});
		}
		return nd_next;
	}
	ama::Node* ParseColons(ama::Node* nd_root, JSValueConst options) {
		int has_c_conditional = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_c_conditional"), 1);
		//int has_labels = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_labels"), 1);
		for ( ama::Node* const & nd_raw: nd_root->FindAllWithin(0, ama::N_RAW) ) {
			if ( !nd_raw->c ) { continue; }
			//console.error('--------');
			//console.error(nd_raw.toSource());
			std::vector<ColonStackItem> cstk{};
			cstk--->push(ColonStackItem{.nd_head = nd_raw->c, .nd_qmark = nullptr, .nd_colon = nullptr});
			for (ama::Node* ndi = nd_raw->c; ndi; ndi = ndi->s) {
				again:
				if ( has_c_conditional && ndi->isSymbol("?") ) {
					assert(cstk.size() > 0);
					if ( cstk.back().nd_colon ) {
						// foo?bar:baz? /*we are here*/
						// bar:baz? /*we are here*/
						// 1?1:0?2:3 == 1?1:(0?2:3) - here we need to create a new conditional
						ama::Node* nd_colon = cstk.back().nd_colon;
						assert(nd_colon);
						if ( nd_colon->s != ndi ) {
							cstk--->push(ColonStackItem{.nd_head = nd_colon->s, .nd_qmark = ndi, .nd_colon = nullptr});
						}
					} else if ( cstk.back().nd_qmark ) {
						//again, create a new conditional
						ama::Node* nd_qmark = cstk.back().nd_qmark;
						assert(nd_qmark);
						if ( nd_qmark->s != ndi ) {
							cstk--->push(ColonStackItem{.nd_head = nd_qmark->s, .nd_qmark = ndi, .nd_colon = nullptr});
						}
					} else if ( cstk.back().nd_head != ndi ) {
						cstk.back().nd_qmark = ndi;
					}
				} else if ( ndi->isSymbol(":") ) {
					assert(cstk.size() > 0);
					while ( cstk.back().nd_colon ) {
						assert(cstk.size() > 0);
						ndi = FoldColonStack(cstk, ndi);
					}
					//conditional -- foo?bar:
					//label -- bar:
					//both only involves setting pcolon
					if ( !cstk.back().nd_qmark || cstk.back().nd_head != ndi ) {
						assert(cstk.back().nd_qmark != ndi);
						cstk.back().nd_colon = ndi;
						if ( !cstk.back().nd_qmark && cstk.back().nd_head && (cstk.back().nd_head->isRef("case") || cstk.back().nd_head->isRef("default")) ) {
							//`case` / `default` case: flush EVERYTHING immediately
							ama::Node* nd_raw_current = ndi->p;
							ama::Node* nd_next = ndi->s;
							while ( cstk.size() ) {
								if ( cstk.back().nd_colon ) {
									FoldColonStack(cstk, nd_next);
								} else {
									cstk--->pop();
								}
							}
							cstk--->push(ColonStackItem{.nd_head = nd_next, .nd_qmark = nullptr, .nd_colon = nullptr});
							if (nd_raw_current && nd_next && nd_next->Prev()) {
								ama::Node* nd_air_label = nd_next->Prev();
								if (nd_air_label->node_class == ama::N_LABELED && !nd_air_label->Prev()) {
									nd_air_label->Unlink();
									nd_air_label->comments_before = nd_raw_current->comments_before + nd_air_label->comments_before;
									nd_raw_current->comments_before = "";
									nd_air_label->AdjustIndentLevel(nd_raw_current->indent_level);
									nd_raw_current->Insert(ama::POS_BEFORE, nd_air_label);
								}
							}
							ndi = nd_next;
							if (!ndi) {break;}
							goto again;
						}
					}
				} else if ( ndi->isSymbol(",") ) {
					//fold ?:
					while ( cstk.size() > 0 && cstk.back().nd_colon ) {
						ndi = FoldColonStack(cstk, ndi);
					}
				}
			}
			while ( cstk.size() ) {
				if ( cstk.back().nd_colon ) {
					FoldColonStack(cstk, nullptr);
				} else {
					cstk--->pop();
				}
			}
		}
		return nd_root;
	}
	bool isUnderParameter(ama::Node* nd_asgn) {
		ama::Node* ndi = nd_asgn;
		for (; ;) {
			ama::Node* nd_last = ndi;
			ndi = ndi->p;
			if (!ndi) {return false;}
			if (ndi->node_class == ama::N_MOV && ndi->c == nd_last) {continue;}
			if (ndi->isRawNode(0, 0) && ndi->c == nd_last) {continue;}
			if (ndi->node_class == ama::N_PARAMETER_LIST) {return true;}
			return false;
		}
	}
	ama::Node* ParseAssignment(ama::Node* nd_root, JSValueConst options) {
		std::unordered_map<ama::gcstring, int> binop_priority = ama::GetPrioritizedList(options, "binary_operators");
		std::unordered_map<ama::gcstring, int> lower_than_assignment_operators = ama::GetPrioritizedList(options, "lower_than_assignment_operators");
		std::vector<ama::Node*> Q = nd_root->FindAllWithin(0, ama::N_RAW);
		for (intptr_t i = 0; i < intptr_t(Q.size()); i++) {
			ama::Node* nd_raw = Q[i];
			ama::Node* ndi = nd_raw->c;
			ama::Node* nd_before_lhs = nullptr;
			while ( ndi ) {
				ama::Node* ndi_next = ndi->s;
				if ( ndi_next && ndi_next->s && ndi_next->isSymbol("=") ) {
					ama::Node* ndi_next_next = ndi_next->BreakSibling();
					//move the comments
					ndi->MergeCommentsAfter(ndi_next);
					ndi_next->Unlink();
					ama::Node* nd_after = nullptr;
					for (ama::Node* ndj = ndi_next_next->s; ndj; ndj = ndj->s) {
						if (ndj->node_class == ama::N_SYMBOL && lower_than_assignment_operators--->get(ndj->data)) {
							if (ndj->data == "?") {
								//skip next :
								intptr_t c_cond_level = 1;
								ndj = ndj->s;
								while (ndj) {
									if (ndj->isSymbol(":")) {
										c_cond_level -= 1;
										if (c_cond_level <= 0) {break;}
									} else if (ndj->isSymbol("?")) {
										c_cond_level += 1;
									}
									ndj = ndj->s;
								}
								if (!ndj) {break;}
								continue;
							}
							//must be non-NULL: ndj starts at ndi_next_next->s
							nd_after = ndj->Prev()->BreakSibling();
							break;
						}
					}
					ama::Node* nd_value = ama::toSingleNode(ndi_next_next);
					if (nd_value != ndi_next_next) {
						nd_value->comments_before = nd_value->comments_before + ndi_next_next->comments_before;
						ndi_next_next->comments_before = "";
					}
					nd_value->MergeCommentsBefore(ndi_next);
					ama::Node* nd_tmp = ama::GetPlaceHolder();
					ama::Node* nd_lhs = nullptr;
					if (nd_before_lhs) {
						nd_lhs = nd_before_lhs->BreakSibling()->toSingleNode();
						nd_before_lhs->Insert(ama::POS_AFTER, nd_tmp);
					} else if (nd_after) {
						nd_lhs = nd_raw->BreakChild()->toSingleNode();
						nd_raw->Insert(ama::POS_FRONT, nd_tmp);
					} else {
						nd_raw->ReplaceWith(nd_tmp);
						nd_lhs = nd_raw;
					}
					//ndi.s = NULL;
					ama::Node* nd_asgn = nd_tmp->ReplaceWith(ama::nAssignment(nd_lhs, nd_value));
					//nd_raw->AdjustIndentLevel(-nd_asgn->indent_level);
					//nd_value doesn't need adjusting: it was a child, while nd_raw was the parent
					//always allow :=
					if ( ndi->node_class == ama::N_SYMBOL && (binop_priority--->get(ndi->data) || ndi->data == ":") && ndi != nd_raw->c && ndi != nd_lhs ) {
						//updating assignment
						ndi->Unlink();
						nd_asgn->data = ndi->DestroyForSymbol();
					}
					if ( ndi_next_next->s ) {
						//detect nested assignment
						Q.push_back(nd_value);
					}
					ndi_next->FreeASTStorage();
					if ( nd_raw == nd_lhs && nd_raw->c && !nd_raw->c->s && nd_raw->isRawNode(0, 0) ) {
						nd_lhs = nd_raw->c;
						ama::UnparseRaw(nd_raw);
					}
					if (nd_after) {
						assert(nd_asgn->p == nd_raw);
						nd_asgn->Insert(ama::POS_AFTER, nd_after);
						ndi = nd_asgn;
						ndi_next = nd_asgn->s;
						//gotta continue looping
					} else {
						break;
					}
				} else if (ndi->node_class == ama::N_SYMBOL && lower_than_assignment_operators--->get(ndi->data)) {
					//mark the lhs range
					//?: immediately before = isn't considered as a separate operator
					nd_before_lhs = ndi;
				}
				ndi = ndi_next;
			}
		}
		//fix assignment - function associativity
		for (ama::Node * nd_func: nd_root->FindAllWithin(0, ama::N_FUNCTION)) {
			if (nd_func->c->node_class == ama::N_MOV) {
				ama::Node* nd_asgn = nd_func->c;
				ama::Node* nd_name = nd_asgn->c;
				ama::Node* nd_value = nd_asgn->c->s;
				ama::Node* nd_tmp = ama::GetPlaceHolder();
				nd_value->ReplaceWith(nd_tmp);
				nd_asgn->ReplaceWith(nd_value);
				nd_func->ReplaceWith(nd_asgn);
				nd_tmp->ReplaceWith(nd_func);
			}
		}
		//fix parameter faux assignment once we get a real one
		for (ama::Node * nd_paramlist: nd_root->FindAllWithin(0, ama::N_PARAMETER_LIST)) {
			for (ama::Node* ndi = nd_paramlist->c; ndi; ndi = ndi->s) {
				if (ndi->node_class == ama::N_MOV && ndi->c->node_class == ama::N_MOV && ndi->c->s->node_class == ama::N_AIR) {
					ndi = ndi->ReplaceWith(ndi->c->Unlink());
				}
			}
		}
		return nd_root;
	}
	//COULDDO: lazy parsing with OwningUnparsedExpr  
	static int FoldBinop(std::unordered_map<ama::gcstring, int> const& binop_priority, std::vector<ama::Node*>& stack, int pr) {
		int changed = 0;
		while ( stack.size() >= 3 && stack.back()->node_class != ama::N_SYMBOL && 
		stack[stack.size() - 2]->node_class == ama::N_SYMBOL && stack[stack.size() - 3]->node_class != ama::N_SYMBOL && 
		binop_priority--->get(stack[stack.size() - 2]->data) >= pr ) {
			//fold high priority binop
			ama::Node* nd_last_operator = stack[stack.size() - 2];
			ama::Node* nd_folded = ama::nBinop(
				stack[stack.size() - 3]->MergeCommentsAfter(nd_last_operator),
				nd_last_operator->data,
				stack[stack.size() - 1]->MergeCommentsBefore(nd_last_operator)
			);
			nd_last_operator->p = nullptr;
			nd_last_operator->FreeASTStorage();
			stack--->pop();
			stack--->pop();
			stack[stack.size() - 1] = nd_folded;
			changed = 1;
		}
		return changed;
	}
	static ama::Node* FoldPostfix(ama::Node* nd_operand, ama::Node* nd_operator) {
		ama::Node* nd_prefix_core = nd_operand;
		//the s-reset is mandatory
		nd_operand->s = nullptr;
		while ( nd_prefix_core->node_class == ama::N_PREFIX ) {
			nd_prefix_core = nd_prefix_core->c;
		}
		ama::Node* nd_tmp = ama::GetPlaceHolder();
		if ( nd_prefix_core != nd_operand ) {
			nd_prefix_core->ReplaceWith(nd_tmp);
		}
		ama::Node* nd_unary = ama::nPostfix(nd_prefix_core->MergeCommentsAfter(nd_operator), nd_operator->data)->setCommentsAfter(nd_operator->comments_after);
		//nd_operator is in the stack and its p / c / s could be BS
		//nd_operator.Unlink();
		nd_operator->p = nullptr;
		nd_operator->FreeASTStorage();
		if ( nd_prefix_core != nd_operand ) {
			nd_tmp->ReplaceWith(nd_unary);
		} else {
			nd_operand = nd_unary;
		}
		return nd_operand;
	}
	static bool CanBeCStyleCastType(ama::Node* nd_type) {
		return nd_type->isRawNode('(', ')') && nd_type->c && !nd_type->c->s || nd_type->node_class == ama::N_CALL && CanBeCStyleCastType(nd_type->c);
	}
	ama::Node* ParseOperators(ama::Node* nd_root, JSValueConst options) {
		std::unordered_map<ama::gcstring, int> binop_priority = ama::GetPrioritizedList(options, "binary_operators");
		std::unordered_map<ama::gcstring, int> prefix_ops = ama::GetPrioritizedList(options, "prefix_operators");
		std::unordered_map<ama::gcstring, int> postfix_ops = ama::GetPrioritizedList(options, "postfix_operators");
		std::unordered_map<ama::gcstring, int> named_ops = ama::GetPrioritizedList(options, "named_operators");
		std::unordered_map<ama::gcstring, int> c_type_prefix_operators = ama::GetPrioritizedList(options, "c_type_prefix_operators");
		std::unordered_map<ama::gcstring, int> is_type_suffix = ama::GetPrioritizedList(options, "ambiguous_type_suffix");
		std::unordered_map<ama::gcstring, int> cv_qualifiers = ama::GetPrioritizedList(options, "cv_qualifiers");
		std::unordered_map<ama::gcstring, int> keywords_statement = ama::GetPrioritizedList(options, "keywords_statement");
		std::unordered_map<ama::gcstring, int> keywords_scoped_statement = ama::GetPrioritizedList(options, "keywords_scoped_statement");
		std::unordered_map<ama::gcstring, int> keywords_operator_escape = ama::GetPrioritizedList(options, "keywords_operator_escape");
		int32_t parse_c_style_cast = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_c_style_cast"), 1);
		//treat statement / escape keywords as named ops: they cannot be operands
		for (auto iter: keywords_statement) {
			named_ops--->set(iter.first, 1);
		}
		for (auto iter: keywords_scoped_statement) {
			named_ops--->set(iter.first, 1);
		}
		for (auto iter: keywords_operator_escape) {
			named_ops--->set(iter.first, 1);
		}
		//named operators: convert refs to symbols
		for ( ama::Node * nd_ref: nd_root->FindAllWithin(0, ama::N_REF) ) {
			if ( named_ops--->get(nd_ref->data) || (c_type_prefix_operators--->get(nd_ref->data) && nd_ref->s && nd_ref->s->node_class == ama::N_REF && nd_ref->p && nd_ref->p->node_class == ama::N_RAW) ) {
				nd_ref->node_class = ama::N_SYMBOL;
			}
		}
		//sentenial operand
		//we need to leave uninterpreted structures alone
		//reverse() to fold smaller raws first
		//ignore paren-target C-style cast like (foo)(bar) - there's no way we can tell them apart from a call!
		std::vector<ama::Node*> raws = nd_root->FindAllWithin(0, ama::N_RAW);
		for (intptr_t i = intptr_t(raws.size()) - 1; i >= 0; --i) {
			//binary and unary in one pass
			std::vector<ama::Node*> stack{};
			ama::Node* nd_raw = raws[i];
			ama::Node* ndi = nd_raw->c;
			int changed = 0;
			while ( ndi ) {
				ama::Node* ndi_next = ndi->s;
				if ( ndi->node_class == ama::N_SYMBOL ) {
					if ( ndi->data != "=" ) {
						//fold ambiguous_type_suffix as postfix if we got another symbol that isn't =
						//when && follows a symbol, it can't be the computed-goto prefix
						if ( stack.size() >= 2 && stack.back()->node_class == ama::N_SYMBOL && is_type_suffix--->get(stack.back()->data) && 
						stack[stack.size() - 2]->node_class != ama::N_SYMBOL && (!prefix_ops--->get(ndi->data) || cv_qualifiers--->get(ndi->data) || ndi->data == "&&") ) {
							ama::Node* nd_operator = stack--->pop();
							ama::Node* nd_operand = stack--->pop();
							nd_operand = FoldPostfix(nd_operand, nd_operator);
							stack.push_back(nd_operand);
							changed = 1;
						}
					}
					//COULDDO: N_ARRAY after type suffix: need an option
					int pr = 0x7fffffff;
					if ( postfix_ops--->get(ndi->data) && stack.size() > 0 && stack.back()->node_class != ama::N_SYMBOL ) {
						//precedence problem - need to replace the prefix-core instead
						ama::Node* nd_operand = stack--->pop();
						nd_operand = FoldPostfix(nd_operand, ndi);
						stack.push_back(nd_operand);
						ndi = ndi_next;
						changed = 1;
						continue;
					} else {
						pr = binop_priority--->get(ndi->data);
						if ( pr ) {
							//flush >=pr
							changed |= FoldBinop(binop_priority, stack, pr);
						} else {
							//separator: ; or , or ...
							changed |= FoldBinop(binop_priority, stack, 1);
						}
					}
				} else {
					//operand, fold prefix
					//C-style cast `(foo)bar` counts as prefix
					if (ndi->node_class != ama::N_SCOPE) {
						while ( stack.size() >= 1 && stack.back()->node_class == ama::N_SYMBOL && prefix_ops--->get(stack.back()->data) ||
						parse_c_style_cast && stack.size() >= 1 && CanBeCStyleCastType(stack.back())) {
							if ( stack.size() >= 2 && stack[stack.size() - 2]->node_class != ama::N_SYMBOL && 
							stack.back()->node_class == ama::N_SYMBOL && binop_priority--->get(stack.back()->data) ) {
								//prefix-binary ambiguity: assume binary
								break;
							}
							if (parse_c_style_cast && stack.back()->node_class == ama::N_SYMBOL && CanBeCStyleCastType(ndi) && ndi_next && ndi_next->node_class != ama::N_SYMBOL) {
								//defer prefixop before type cast
								break;
							}
							ama::Node* nd_prefix_operator = stack--->pop();
							if (nd_prefix_operator->node_class == ama::N_SYMBOL) {
								ndi = ama::nPrefix(nd_prefix_operator->data, ndi->MergeCommentsBefore(nd_prefix_operator))->setCommentsBefore(nd_prefix_operator->comments_before);
								nd_prefix_operator->p = nullptr;
								nd_prefix_operator->FreeASTStorage();
							} else {
								//C-style cast
								assert(parse_c_style_cast);
								//COULDDO: a dedicated node class
								//but we aren't too confident here: the syntax is too generic to unambiguously conclude that it's a cast
								ndi = ama::CreateNode(ama::N_RAW, ama::cons(nd_prefix_operator, ama::cons(ndi, nullptr)));
								while (nd_prefix_operator->node_class == ama::N_CALL) {
									nd_prefix_operator = ama::UnparseCall(nd_prefix_operator);
								}
							}
							changed = 1;
						}
					}
					if ( stack.size() >= 1 && stack.back()->node_class != ama::N_SYMBOL ) {
						//pushing an operand while we have another operand, fold binop
						changed |= FoldBinop(binop_priority, stack, 1);
					}
				}
				stack.push_back(ndi);
				ndi = ndi_next;
			}
			//fold any leftover ambiguous_type_suffix as postfix at the end
			if ( stack.size() >= 2 && stack.back()->node_class == ama::N_SYMBOL && is_type_suffix--->get(stack.back()->data) && stack[stack.size() - 2]->node_class != ama::N_SYMBOL ) {
				ama::Node* nd_operator = stack--->pop();
				ama::Node* nd_operand = stack--->pop();
				nd_operand = FoldPostfix(nd_operand, nd_operator);
				stack.push_back(nd_operand);
				changed = 1;
			}
			//fold remaining binops
			changed |= FoldBinop(binop_priority, stack, 1);
			if ( changed ) {
				if ( stack.size() == 1 && !(nd_raw->flags & 0xffff) ) {
					assert(!stack[0]->s);
					nd_raw->c = nullptr;
					nd_raw->ReplaceWith(stack[0]);
					nd_raw->FreeASTStorage();
				} else {
					//we still have uninterpreted things, replace children
					assert(stack.size() > 0);
					nd_raw->c = nullptr;
					nd_raw->Insert(ama::POS_FRONT, ama::InsertMany(stack));
				}
			}
		}
		//unify named unary ops to call: `sizeof(T)` could have been mistakenly parsed as N_CALL
		//for(ama::Node*&+! nd_unary : nd_root.FindAllWithin(0,ama::N_PREFIX, NULL)) {
		//	if( named_ops[nd_unary.data] ) {
		//		//if( nd_unary.c && nd_unary.c.comments_before == " " ) {
		//		//	nd_unary.c.comments_before = '';
		//		//}
		//		nd_unary.ReplaceWith(ama::CreateNode(ama::N_CALL, ama::cons(ama::nRef(nd_unary.data), nd_unary.c)).setFlags(ama::CALL_IS_UNARY_OPERATOR));
		//		nd_unary.c = NULL;
		//		nd_unary.FreeASTStorage();
		//	}
		//}
		//convert failed-to-fold named ops back
		for ( ama::Node* const & nd_ref: nd_root->FindAllWithin(0, ama::N_SYMBOL) ) {
			if ( named_ops--->get(nd_ref->data) || c_type_prefix_operators--->get(nd_ref->data) ) {
				nd_ref->node_class = ama::N_REF;
			}
		}
		return nd_root;
	}
	ama::Node* ParsePointedBrackets(ama::Node* nd_root) {
		for ( ama::Node * nd_raw: nd_root->FindAllWithin(0, ama::N_RAW) ) {
			//actually split it
			std::vector<ama::Node*> stack{};
			int was_ref = 0;
			bool in_rvalue = false;
			for (ama::Node* ndi = nd_raw->c; ndi; ndi = ndi->s) {
				if ( ndi->node_class == ama::N_SYMBOL ) {
					if (stack.size() >= 1 && ndi->data == ">=") {
						ndi->Insert(ama::POS_BEFORE, ama::nSymbol(">"));
						ndi->data = "=";
						ndi = ndi->Prev();
					} else if (stack.size() >= 2 && ndi->data == ">>=") {
						ndi->Insert(ama::POS_BEFORE, ama::nSymbol(">>"));
						ndi->data = "=";
						ndi = ndi->Prev();
					}
					if ( was_ref && ndi->data == "<" ) {
						stack.push_back(ndi);
					} else if ( (ndi->data == ">" || ndi->data == ">>" || ndi->data == ">>>") && stack.size() >= ndi->data.size() ) {
						if (ndi->s) {
							//check for likely false positives: foo=bar<...>baz; bar<...>0
							if (in_rvalue && ndi->s->node_class == ama::N_REF) {continue;}
							if (ndi->s->node_class == ama::N_NUMBER || ndi->s->node_class == ama::N_STRING) {continue;}
						}
						intptr_t n_levels = ndi->data.size();
						for (int j = 0; j < n_levels; j += 1) {
							ama::Node* ndi0 = stack--->pop();
							ama::Node* nd_tmp = ama::GetPlaceHolder();
							ama::Node* ndi0_next = ndi0->s;
							if ( ndi0_next == ndi ) {
								ndi0_next = nullptr;
							}
							ndi->Prev()->MergeCommentsAfter(ndi); 
							ndi0->ReplaceUpto(ndi, nd_tmp);
							if ( j == 0 ) {ndi->BreakSelf();}
							ama::Node* nd_child = ama::CreateNode(ama::N_RAW, ndi0_next);
							nd_child->flags = uint32_t('<') | uint32_t('>') << 8;
							nd_tmp->ReplaceWith(nd_child);
							for (ama::Node* ndj = nd_child->c; ndj; ndj = ndj->s) {
								ndj->AdjustIndentLevel(-nd_child->indent_level);
							}
							//////////
							ndi0->p = nullptr; ndi0->s = nullptr; ndi0->FreeASTStorage();
							if ( j == 0 ) {
								ndi->p = nullptr; ndi->s = nullptr; ndi->FreeASTStorage();
							}
							ndi = nd_child;
						}
						was_ref = 0;
						continue;
					} else if ( ndi->data == "||" || ndi->data == "&&" ) {
						stack.clear();
					} else if (!stack.size() && ndi->data == "=") {
						stack.clear();
						in_rvalue = true;
					} else if (ndi->data == ";") {
						stack.clear();
						in_rvalue = false;
					}
				}
				was_ref = ndi->node_class == ama::N_REF;
			}
		}
		return nd_root;
	}
	ama::Node* UnparseBinop(ama::Node* nd_binop) {
		ama::Node* nd_parent = nd_binop->p;
		if ( !nd_parent || nd_parent->node_class != ama::N_RAW ) {
			nd_parent = nd_binop->ReplaceWith(ama::CreateNode(ama::N_RAW, nullptr));
			nd_parent->Insert(ama::POS_FRONT, nd_binop);
			//nd_parent->indent_level = nd_binop->indent_level;
		}
		ama::Node* nd_a = nd_binop->BreakChild();
		ama::Node* nd_b = nd_a->BreakSibling();
		int8_t indent_a = ama::ClampIndentLevel(int32_t(nd_a->indent_level) + nd_binop->indent_level);
		int8_t indent_b = ama::ClampIndentLevel(int32_t(nd_b->indent_level) + nd_binop->indent_level);
		ama::Node* ret = nd_binop->ReplaceWith(ama::cons(nd_a, ama::cons(ama::nSymbol(nd_binop->data), nd_b)));
		nd_a->indent_level = indent_a;
		nd_b->indent_level = indent_b;
		if ( nd_a->isRawNode(0, 0) ) {
			ama::UnparseRaw(nd_a);
		}
		if ( nd_b->isRawNode(0, 0) ) {
			ama::UnparseRaw(nd_b);
		}
		nd_binop->FreeASTStorage();
		return ret;
	}
	ama::Node* UnparsePrefix(ama::Node* nd_unary) {
		ama::Node* nd_parent = nd_unary->p;
		if ( nd_parent && nd_parent->node_class != ama::N_RAW ) {
			nd_parent = nd_unary->ReplaceWith(ama::CreateNode(ama::N_RAW, nullptr));
			nd_parent->Insert(ama::POS_FRONT, nd_unary);
			nd_parent->indent_level = nd_unary->indent_level;
		}
		ama::Node* nd_opr = nd_unary->BreakChild();
		ama::Node* ret = nd_unary->ReplaceWith(ama::cons(ama::nSymbol(nd_unary->data), nd_opr));
		if ( nd_opr->isRawNode(0, 0) ) {
			ama::UnparseRaw(nd_opr);
		}
		nd_unary->FreeASTStorage();
		return ret;
	}
	ama::Node* UnparsePostfix(ama::Node* nd_unary) {
		ama::Node* nd_parent = nd_unary->p;
		if ( nd_parent && nd_parent->node_class != ama::N_RAW ) {
			nd_parent = nd_unary->ReplaceWith(ama::CreateNode(ama::N_RAW, nullptr));
			nd_parent->Insert(ama::POS_FRONT, nd_unary);
			nd_parent->indent_level = nd_unary->indent_level;
		}
		ama::Node* nd_opr = nd_unary->BreakChild();
		ama::Node* ret = nd_unary->ReplaceWith(ama::cons(nd_opr, ama::nSymbol(nd_unary->data)));
		if ( nd_opr->isRawNode(0, 0) ) {
			ama::UnparseRaw(nd_opr);
		}
		nd_unary->FreeASTStorage();
		return ret;
	}
	ama::Node* UnparseLabel(ama::Node* nd_label) {
		assert(nd_label->node_class == ama::N_LABELED);
		nd_label->node_class = ama::N_BINOP;
		nd_label->data = ":";
		return UnparseBinop(nd_label);
	}
};
