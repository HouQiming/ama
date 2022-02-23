#include <string>
#include <vector>
#include "../../modules/cpp/json/json.h"
#include "../ast/node.hpp"
#include "../util/jc_array.h"
#include "postfix.hpp"
namespace ama {
	static inline int isCPPLambda(ama::Node* ndi) {
		return ndi->node_class == ama::N_CALL && ndi->c->isRawNode('[', ']');
	}
	ama::Node* TranslatePostfixCall(ama::Node* ndi) {
		ama::Node* nd_arglist = ndi->s;
		uint8_t call_class = ama::N_CALL;
		if ( nd_arglist->isRawNode('<', '>') ) {
			call_class = ama::N_CALL_TEMPLATE;
		} else if ( nd_arglist->node_class == ama::N_ARRAY || nd_arglist->isRawNode('[', ']') ) {
			call_class = ama::N_ITEM;
		}
		ndi->MergeCommentsAfter(nd_arglist);
		//re-delimit everything completely
		std::vector<ama::Node*> new_children{};
		new_children.push_back(ndi);
		std::string pending_comments_before{};
		if ( nd_arglist->c ) {
			//console.error(nd_arglist.toSource());
			std::vector<ama::Node*> comma_group{};
			for (ama::Node* ndi = nd_arglist->c; ndi; ndi = ndi->s) {
				//strip trailing ','s
				if ( ndi->isRawNode(0, 0) ) {
					if ( comma_group.size() ) {
						comma_group[comma_group.size() - 1]->comments_after = (comma_group[comma_group.size() - 1]->comments_after + ndi->comments_before);
					} else {
						pending_comments_before--->push(ndi->comments_before);
					}
					for (ama::Node* ndk = ndi->c; ndk; ndk = ndk->s) {
						if ( ndk->isSymbol(",") ) {
							ama::Node* nd_arg = ama::toSingleNode(ama::InsertMany(comma_group));
							nd_arg->comments_before = (ama::gcscat(pending_comments_before, nd_arg->comments_before));
							nd_arg->comments_after = (nd_arg->comments_after + ndk->comments_before);
							new_children.push_back(nd_arg);
							pending_comments_before.clear();
							pending_comments_before--->push(ndk->comments_after);
							comma_group.clear();
						} else {
							ndk->AdjustIndentLevel(ndi->indent_level);
							comma_group.push_back(ndk);
						}
					}
					if ( comma_group.size() ) {
						comma_group[comma_group.size() - 1]->comments_after = (comma_group[comma_group.size() - 1]->comments_after + ndi->comments_after);
					} else {
						pending_comments_before--->push(ndi->comments_after);
					}
				} else if ( ndi->isSymbol(",") ) {
					ama::Node* nd_arg = ama::toSingleNode(ama::InsertMany(comma_group));
					nd_arg->comments_before = (ama::gcscat(pending_comments_before, nd_arg->comments_before));
					nd_arg->comments_after = (nd_arg->comments_after + ndi->comments_before);
					new_children.push_back(nd_arg);
					pending_comments_before.clear();
					pending_comments_before--->push(ndi->comments_after);
					comma_group.clear();
				} else {
					comma_group.push_back(ndi);
				}
			}
			//assert(comma_group.size() > 0);
			//we could get comma_group.size() == 0 for a(foo,)
			if ( comma_group.size() > 0 || new_children.size() > 0 ) {
				ama::Node* nd_arg = ama::toSingleNode(ama::InsertMany(comma_group));
				nd_arg->comments_before = (ama::gcscat(pending_comments_before, nd_arg->comments_before));
				new_children.push_back(nd_arg);
				pending_comments_before.clear();
			}
		}
		//for(int! i = 2; i < new_children.length; i += 1) {
		//	if( new_children[i].comments_before.startsWith(' ') ) {
		//		new_children[i].comments_before = new char[|]!(new_children[i].comments_before.subarray(1));
		//	}
		//}
		ama::Node* nd_tmp = ama::GetPlaceHolder();
		ndi->ReplaceUpto(nd_arglist, nd_tmp);
		//.setCommentsAfter(nd_arglist.comments_after)
		ama::Node* nd_call = nd_tmp->ReplaceWith(ama::CreateNodeFromChildren(call_class, new_children));
		if ( pending_comments_before.size() ) {
			nd_call->comments_after = (ama::gcscat(pending_comments_before, nd_call->comments_after));
			pending_comments_before.clear();
		}
		//console.log(JSON.stringify(ndi.toSource()), JSON.stringify(ndi.comments_after));
		//ndi.indent_level = ndi_old.indent_level;
		//ndi_old.indent_level=0;
		//console.log('>>>', intptr_t(ndi_next_next), intptr_t(ndi_next_next.s));
		//nd_raw.Insert(ama::POS_BACK, ama::cons(ndi, ndi_next_next));
		//nd_arglist is also N_RAW so it can't be freed right now: but we shouldn't process it any more
		ndi = nd_call;
		nd_arglist->c = nullptr;
		return ndi;
	}
	ama::Node* ParsePostfix(ama::Node* nd_root, JSValue options) {
		//note: if we do it forward, the free-ed N_RAW() / N_RAW[] will get enumerated later and cause all kinds of issues
		std::unordered_map<ama::gcstring, int> postfix_ops = ama::GetPrioritizedList(options, "postfix_operators");
		std::unordered_map<ama::gcstring, int> keywords_class = ama::GetPrioritizedList(options, "keywords_class");
		std::unordered_map<ama::gcstring, int> keywords_statement = ama::GetPrioritizedList(options, "keywords_statement");
		std::unordered_map<ama::gcstring, int> keywords_scoped_statement = ama::GetPrioritizedList(options, "keywords_scoped_statement");
		std::unordered_map<ama::gcstring, int> keywords_extension_clause = ama::GetPrioritizedList(options, "keywords_extension_clause");
		std::unordered_map<ama::gcstring, int> keywords_operator_escape = ama::GetPrioritizedList(options, "keywords_operator_escape");
		for (auto iter: keywords_class) {
			keywords_statement--->set(iter.first, 1);
		}
		for (auto iter: keywords_scoped_statement) {
			keywords_statement--->set(iter.first, 1);
		}
		for (auto iter: keywords_extension_clause) {
			keywords_statement--->set(iter.first, 1);
		}
		for (auto iter: keywords_operator_escape) {
			keywords_statement--->set(iter.first, 1);
		}
		int32_t parse_air_object = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_air_object"), 1);
		int32_t parse_typed_object = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_typed_object"), 1);
		std::vector<ama::Node*> a = (std::vector<ama::Node*>{nd_root})--->concat(nd_root->FindAllWithin(0, ama::N_SCOPE), nd_root->FindAllWithin(0, ama::N_RAW));
		for (intptr_t i = intptr_t(a.size()) - 1; i >= 0; --i) {
			ama::Node* ndi = a[i]->c;
			int32_t after_class = 0;
			while ( ndi ) {
				ama::Node* ndi_next = ndi->s;
				//for struct foo bar{}
				if (ndi->node_class == ama::N_REF) {after_class = 0;}
				if (ndi->node_class == ama::N_REF && keywords_statement--->get(ndi->data, 0)) {
					//it's a statement keyword, do nothing: it cannot dot or call
					//the C++ 'operator' keyword also ends here
					if (keywords_class--->get(ndi->data)) {
						after_class = 1;
					}
				} else if ( parse_air_object && 
				ndi->node_class == ama::N_SYMBOL && (ndi->data == "." || ndi->data == "::") && 
				ndi->s && ndi->s->node_class == ama::N_REF ) {
					//dot
					int dot_flags = 0;
					if ( ndi->data == "->" ) {
						dot_flags = ama::DOT_PTR;
					} else if ( ndi->data == "::" ) {
						dot_flags = ama::DOT_CLASS;
					}
					ama::gcstring name = ndi_next->data;
					//.setCommentsBefore(ndi.comments_before).setCommentsAfter(ndi_next.comments_after)
					ama::Node* nd_dot = ndi->ReplaceUpto(
						ndi_next,
						ama::nAir()->setCommentsAfter(ndi->comments_after + ndi_next->comments_before)->dot(name)->setFlags(dot_flags)
					);
					ndi_next->p = nullptr; ndi_next->FreeASTStorage();
					ndi->p = nullptr; ndi->s = nullptr; ndi->FreeASTStorage();
					ndi = nd_dot;
					continue;
				} else if ( (ndi->isSymbol(".") || ndi->isSymbol("@")) && ndi_next && (ndi_next->isRawNode('(', ')') || 
				ndi_next->isRawNode('{', '}') || ndi_next->node_class == ama::N_SCOPE || ndi_next->node_class == ama::N_OBJECT) ) {
					//nodeof
					ndi_next->Unlink();
					ndi = ndi->ReplaceWith(ama::nNodeof(ndi_next)->setIndent(ndi->indent_level));
					continue;
				} else if (ndi->node_class == ama::N_SYMBOL) {
					//do nothing
					after_class = 0;
				} else if (ndi_next && ndi_next->node_class == ama::N_SYMBOL && postfix_ops--->get(ndi_next->data, 0)) {
					//postfix operator
					ndi->MergeCommentsAfter(ndi_next);
					ama::Node* nd_tmp = ama::GetPlaceHolder();
					ndi->ReplaceUpto(ndi_next, nd_tmp);
					ndi->BreakSibling();
					ndi = nd_tmp->ReplaceWith(ama::nPostfix(ndi, ndi_next->data)->setCommentsAfter(ndi_next->comments_after));
					ndi_next->p = nullptr; ndi_next->FreeASTStorage();
					continue;
				} else if (ndi_next && ndi_next->node_class == ama::N_SYMBOL && 
				(ndi_next->data == "." || (!isCPPLambda(ndi) && ndi_next->data == "->") || ndi_next->data == "::") && 
				ndi_next->s && ndi_next->s->node_class == ama::N_REF) {
					//dot
					int dot_flags = 0;
					if ( ndi_next->data == "->" ) {
						dot_flags = ama::DOT_PTR;
					} else if ( ndi_next->data == "::" ) {
						dot_flags = ama::DOT_CLASS;
					}
					ama::Node* nd_name = ndi_next->s;
					nd_name->MergeCommentsBefore(ndi_next);
					ama::gcstring name = nd_name->data;
					ama::Node* nd_tmp = ama::GetPlaceHolder();
					ndi->ReplaceUpto(nd_name, nd_tmp);
					ndi->BreakSibling();
					ndi = nd_tmp->ReplaceWith(
						ndi->MergeCommentsAndIndentAfter(ndi_next)->MergeCommentsAfter(nd_name)->dot(name)->setFlags(dot_flags)->setCommentsAfter(nd_name->comments_after)
					);
					nd_name->p = nullptr; nd_name->FreeASTStorage();
					ndi_next->p = nullptr; ndi_next->FreeASTStorage();
					continue;
				} else if (ndi_next && (ndi_next->isRawNode('(', ')') || ndi_next->isRawNode('<', '>') ||
				ndi_next->node_class == ama::N_ARRAY || 
				ndi_next->isRawNode('[', ']')) && !ndi_next->FindAllWithin(ama::BOUNDARY_ONE_LEVEL, ama::N_SYMBOL, ";").size()) {
					//call
					ndi = TranslatePostfixCall(ndi);
					continue;
				} else if (parse_typed_object && !after_class && ndi_next && 
				ndi->node_class != ama::N_CALL && ndi->node_class != ama::N_STRING && ndi->node_class != ama::N_ARRAY && !ndi->isRawNode('(', ')') && (
					ndi_next->node_class == ama::N_SCOPE && (!ndi_next->c || !ndi_next->c->s) || ndi_next->node_class == ama::N_OBJECT
				)) {
					//0-1 children scope / object: N_TYPED_OBJECT
					//filter out N_STRING caller for `${foo}` strings 
					if (ndi_next->node_class == ama::N_SCOPE) {
						ndi_next->node_class = ama::N_OBJECT;
					}
					ama::Node* nd_tmp = ama::GetPlaceHolder();
					ndi->ReplaceUpto(ndi_next, nd_tmp);
					ndi->BreakSibling();
					ndi = nd_tmp->ReplaceWith(ama::nTypedObject(ndi, ndi_next));
					continue;
				} else if (ndi_next && ndi_next->isSymbol("<<<")) {
					ama::Node* ndj = nullptr;
					for (ndj = ndi_next->s; ndj; ndj = ndj->s) {
						if (ndj->isSymbol(">>>")) {break;}
					}
					if ((ndj && ndj != ndi_next->s)) {
						ama::Node* nd_tmp = ama::GetPlaceHolder();
						ndi_next->ReplaceUpto(ndj, nd_tmp);
						ndj->Unlink();
						ndi_next = nd_tmp->ReplaceWith(ama::toSingleNode(ndi_next->BreakSibling()));
						ndi = TranslatePostfixCall(ndi);
						ndi->node_class = ama::N_CALL_CUDA_KERNEL;
						continue;
					}
				}
				ndi = ndi_next;
			}
		}
		return nd_root;
	}
	ama::Node* UnparseCall(ama::Node* nd) {
		assert(nd->node_class == ama::N_CALL || nd->node_class == ama::N_CALL_TEMPLATE);
		std::vector<ama::Node*> args{};
		for (ama::Node* ndi = nd->c->BreakSibling(); ndi; ndi = ndi->s) {
			args.push_back(ndi);
			if ( ndi->s ) {
				args.push_back(ama::nSymbol(","));
			}
		}
		uint32_t flags = uint32_t('(') | uint32_t(')') << 8;
		if ( nd->node_class == ama::N_CALL_TEMPLATE ) {
			flags = uint32_t('<') | uint32_t('>') << 8;
		}
		ama::Node* ret = nd->ReplaceWith(ama::cons(nd->c->BreakSelf(), ama::CreateNodeFromChildren(ama::N_RAW, args)->setFlags(flags)->setIndent(nd->indent_level)));
		//nd.c = NULL;
		nd->FreeASTStorage();
		return ret;
	}
};
