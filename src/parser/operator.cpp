#include <vector>
#include <unordered_map>
#include "../util/jc_array.h"
#include "../ast/node.hpp"
#include "../script/jsenv.hpp"
#include "operator.hpp"
namespace ama {
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
		//if( nd_next ) { nd_next.BreakSelf(); }
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
		//if( nd_next ) { nd_ret.Insert(ama::POS_AFTER, nd_next); }
		cstk--->pop();
		if ( !cstk.size() ) {
			cstk--->push(ColonStackItem{.nd_head = nd_ret, .nd_qmark = nullptr, .nd_colon = nullptr});
		}
		return nd_next;
	}
	ama::Node* ParseColons(ama::Node* nd_root, int has_c_conditional) {
		for ( ama::Node* const &nd_raw: nd_root->FindAllWithin(0, ama::N_RAW) ) {
			if ( !nd_raw->c ) { continue; }
			//console.error('--------');
			//console.error(nd_raw.toSource());
			std::vector<ColonStackItem> cstk{};
			cstk--->push(ColonStackItem{.nd_head = nd_raw->c, .nd_qmark = nullptr, .nd_colon = nullptr});
			for (ama::Node* ndi = nd_raw->c; ndi; ndi = ndi->s) {
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
	ama::Node* ParseAssignment(ama::Node* nd_root, JSValueConst options) {
		std::unordered_map<ama::gcstring, int> binop_priority = ama::GetPrioritizedList(options, "binary_operators");
		std::vector<ama::Node*> Q = nd_root->FindAllWithin(0, ama::N_RAW);
		for (intptr_t i = 0; i < Q.size(); i++) {
			ama::Node* nd_raw = Q[i];
			ama::Node* ndi = nd_raw->c;
			while ( ndi ) {
				ama::Node* ndi_next = ndi->s;
				if ( ndi_next && ndi_next->s && 
				ndi_next->isSymbol("=") ) {
					ama::Node* ndi_next_next = ndi_next->BreakSibling();
					//move the comments
					ndi->MergeCommentsAfter(ndi_next);
					ndi_next->Unlink();
					ama::Node* nd_value = ama::toSingleNode(ndi_next_next);
					nd_value->comments_before = ndi_next_next->comments_before;
					ndi_next_next->comments_before = "";
					nd_value->MergeCommentsBefore(ndi_next);
					ama::Node* nd_tmp = ama::GetPlaceHolder();
					nd_raw->ReplaceWith(nd_tmp);
					//ndi.s = NULL;
					ama::Node* nd_asgn = nd_tmp->ReplaceWith(ama::nAssignment(nd_raw, nd_value));
					nd_raw->AdjustIndentLevel(-nd_asgn->indent_level);
					//nd_value doesn't need adjusting: it was a child, while nd_raw was the parent
					//always allow :=
					if ( ndi->node_class == ama::N_SYMBOL && (binop_priority--->get(ndi->data) || ndi->data == ":") && ndi != nd_raw->c ) {
						//updating assignment
						ndi->Unlink();
						nd_asgn->data = ndi->DestroyForSymbol();
					}
					if ( ndi_next_next->s ) {
						//detect nested assignment
						Q.push_back(nd_value);
					}
					ndi_next->FreeASTStorage();
					if ( nd_raw->c && !nd_raw->c->s && nd_raw->isRawNode(0, 0) ) {
						ama::UnparseRaw(nd_raw);
					}
					break;
				}
				ndi = ndi_next;
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
	ama::Node* ParseOperators(ama::Node* nd_root, JSValueConst options) {
		std::unordered_map<ama::gcstring, int> binop_priority = ama::GetPrioritizedList(options, "binary_operators");
		std::unordered_map<ama::gcstring, int> prefix_ops = ama::GetPrioritizedList(options, "prefix_operators");
		std::unordered_map<ama::gcstring, int> postfix_ops = ama::GetPrioritizedList(options, "postfix_operators");
		std::unordered_map<ama::gcstring, int> named_ops = ama::GetPrioritizedList(options, "named_operators");
		std::unordered_map<ama::gcstring, int> c_type_prefix_operators = ama::GetPrioritizedList(options, "c_type_prefix_operators");
		std::unordered_map<ama::gcstring, int> is_type_suffix = ama::GetPrioritizedList(options, "ambiguous_type_suffix");
		std::unordered_map<ama::gcstring, int> cv_qualifiers = ama::GetPrioritizedList(options, "cv_qualifiers");
		//named operators: convert refs to symbols
		for ( ama::Node* const &nd_ref: nd_root->FindAllWithin(0, ama::N_REF) ) {
			if ( named_ops--->get(nd_ref->data) || (c_type_prefix_operators--->get(nd_ref->data) && nd_ref->s && nd_ref->s->node_class == ama::N_REF && nd_ref->p && nd_ref->p->node_class == ama::N_RAW) ) {
				nd_ref->node_class = ama::N_SYMBOL;
			}
		}
		//sentenial operand
		//we need to leave uninterpreted structures alone
		//reverse() to fold smaller raws first
		{
			std::vector<ama::Node*> a = nd_root->FindAllWithin(0, ama::N_RAW);
			for (intptr_t i = intptr_t(a.size()) - 1; i >= 0; --i) {
				{
					//binary and unary in one pass
					std::vector<ama::Node*> stack{};
					//ama::Node*+! nd_raw = Q[i];
					ama::Node* ndi = a[i]->c;
					int changed = 0;
					while ( ndi ) {
						ama::Node* ndi_next = ndi->s;
						if ( ndi->node_class == ama::N_SYMBOL ) {
							if ( ndi->data != "=" ) {
								//fold ambiguous_type_suffix as postfix if we got another symbol that isn't =
								if ( stack.size() >= 2 && stack.back()->node_class == ama::N_SYMBOL && is_type_suffix--->get(stack.back()->data) && 
								stack[stack.size() - 2]->node_class != ama::N_SYMBOL && (!prefix_ops--->get(ndi->data) || cv_qualifiers--->get(ndi->data)) ) {
									ama::Node* nd_operator = stack--->pop();
									ama::Node* nd_operand = stack--->pop();
									nd_operand = FoldPostfix(nd_operand, nd_operator);
									stack.push_back(nd_operand);
									changed = 1;
								}
							}
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
							while ( stack.size() >= 1 && stack.back()->node_class == ama::N_SYMBOL && prefix_ops--->get(stack.back()->data) ) {
								if ( stack.size() >= 2 && stack[stack.size() - 2]->node_class != ama::N_SYMBOL && binop_priority--->get(stack.back()->data) ) {
									//prefix-binary ambiguity: assume binary
									break;
								}
								ama::Node* nd_prefix_operator = stack--->pop();
								ndi = ama::nPrefix(nd_prefix_operator->data, ndi);
								nd_prefix_operator->p = nullptr;
								nd_prefix_operator->FreeASTStorage();
								changed = 1;
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
						if ( stack.size() == 1 && !(a[i]->flags & 0xffff) ) {
							assert(!stack[0]->s);
							a[i]->c = nullptr;
							a[i]->ReplaceWith(stack[0]);
							a[i]->FreeASTStorage();
						} else {
							//we still have uninterpreted things, replace children
							assert(stack.size() > 0);
							a[i]->c = nullptr;
							a[i]->Insert(ama::POS_FRONT, ama::InsertMany(stack));
						}
					}
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
		for ( ama::Node* const &nd_ref: nd_root->FindAllWithin(0, ama::N_SYMBOL) ) {
			if ( named_ops--->get(nd_ref->data) || c_type_prefix_operators--->get(nd_ref->data) ) {
				nd_ref->node_class = ama::N_REF;
			}
		}
		return nd_root;
	}
	ama::Node* ParsePointedBrackets(ama::Node* nd_root) {
		for ( ama::Node* const &nd_raw: nd_root->FindAllWithin(0, ama::N_RAW) ) {
			//actually split it
			std::vector<ama::Node*> stack{};
			int was_ref = 0;
			for (ama::Node* ndi = nd_raw->c; ndi; ndi = ndi->s) {
				if ( ndi->node_class == ama::N_SYMBOL ) {
					if ( was_ref && ndi->data == "<" ) {
						stack.push_back(ndi);
					} else if ( (ndi->data == ">" || ndi->data == ">>" || ndi->data == ">>>") && stack.size() >= ndi->data.size() ) {
						intptr_t n_levels = ndi->data.size();
						for (int j = 0; j < n_levels; j += 1) {
							ama::Node* ndi0 = stack--->pop();
							ama::Node* nd_tmp = ama::GetPlaceHolder();
							ama::Node* ndi0_next = ndi0->s;
							if ( ndi0_next == ndi ) {
								ndi0_next = nullptr;
							}
							ndi0->ReplaceUpto(ndi, nd_tmp);
							if ( j == 0 ) { ndi->BreakSelf(); }
							ama::Node* nd_child = ama::CreateNode(ama::N_RAW, ndi0_next);
							nd_child->flags = uint32_t('<') | uint32_t('>') << 8;
							nd_tmp->ReplaceWith(nd_child);
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
			nd_parent->indent_level = nd_binop->indent_level;
		}
		ama::Node* nd_a = nd_binop->BreakChild();
		ama::Node* nd_b = nd_a->s;
		ama::Node* ret = nd_binop->ReplaceWith(ama::cons(nd_a, ama::cons(ama::nSymbol(nd_binop->data), nd_b)));
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
		if ( !nd_parent || nd_parent->node_class != ama::N_RAW ) {
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
	ama::Node* UnparseLabel(ama::Node* nd_label) {
		assert(nd_label->node_class == ama::N_LABELED);
		nd_label->node_class = ama::N_BINOP;
		nd_label->data = ":";
		return UnparseBinop(nd_label);
	}
};
