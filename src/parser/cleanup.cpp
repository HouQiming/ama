#include "../../modules/cpp/json/json.h"
#include "../ast/node.hpp"
#include "cleanup.hpp"
#include <vector>
#include "../util/jc_array.h"
namespace ama {
	ama::Node* CleanupDummyRaws(ama::Node* nd_root) {
		//need cleanup inside nodeofs
		for ( ama::Node * nd_raw: nd_root->FindAllWithin(0, ama::N_RAW) ) {
			if ( (nd_raw->flags & 0xffff) != 0 || nd_raw == nd_root ) { continue; }
			if ( !nd_raw->c && nd_raw->p && nd_raw->p->node_class == ama::N_RAW ) {
				if (nd_raw->p->node_class == ama::N_BINOP || nd_raw->p->node_class == ama::N_ASSIGNMENT) {
					nd_raw->ReplaceWith(ama::nAir());
				} else {
					nd_raw->Unlink();
				}
			} else if ( nd_raw->c && !nd_raw->c->s ) {
				//nd_raw.ReplaceWith(nd_raw.c);
				ama::UnparseRaw(nd_raw);
			}
		}
		return nd_root;
	}
	ama::Node* SanitizeCommentPlacement(ama::Node* nd_root) {
		//COULDDO: make \n a trailing comment for statements: what about the first line in a scope?
		//pull comments_before out from nd.c for "headless" nodes
		for (ama::Node* nd = nd_root->PostorderFirst(); nd; nd = nd->PostorderNext(nd_root)) {
			if ( nd->p && nd->p->node_class == ama::N_SCOPE && nd->Prev() && nd->comments_before.size() > 0 && nd->comments_before[0] != '\n' ) {
				ama::Node* ndi_last = nd->Prev();
				//associate the first "line" of multi-line comments with the previous statement
				intptr_t p_newline = nd->comments_before--->indexOf('\n');
				if ( p_newline > intptr_t(0L) ) {
					ndi_last->comments_after = (ama::gcscat(ndi_last->comments_after, nd->comments_before--->subarray(0, p_newline)));
					nd->comments_before = ama::gcstring(nd->comments_before--->subarray(p_newline));
				}
			}
			if ( nd->c && 
			(nd->node_class == ama::N_CALL || nd->node_class == ama::N_CALL_TEMPLATE || nd->node_class == ama::N_CALL_CUDA_KERNEL ||
			nd->node_class == ama::N_TYPED_OBJECT || nd->node_class == ama::N_COMMA ||
			nd->node_class == ama::N_BINOP || nd->node_class == ama::N_POSTFIX || 
			nd->node_class == ama::N_DOT || nd->node_class == ama::N_ITEM || nd->node_class == ama::N_FUNCTION || 
			nd->node_class == ama::N_ASSIGNMENT || nd->node_class == ama::N_CONDITIONAL || 
			nd->node_class == ama::N_LABELED || nd->node_class == ama::N_DELIMITED ||
			nd->node_class == ama::N_PARAMETER_LIST && (nd->flags & ama::PARAMLIST_UNWRAPPED) || 
			(nd->node_class == ama::N_RAW && (nd->flags & 0xffff) == 0)) ) {
				if ( nd->c->comments_before.size() ) {
					nd->comments_before = (nd->comments_before + nd->c->comments_before);
					int32_t delta = nd->c->indent_level;
					nd->AdjustIndentLevel(nd->c->indent_level);
					nd->c->comments_before = "";
					for (ama::Node* ndi = nd->c; ndi; ndi = ndi->s) {
						ndi->AdjustIndentLevel(-delta);
					}
				}
			}
			if ( nd->c && 
			(nd->node_class == ama::N_BINOP || nd->node_class == ama::N_PREFIX || 
			nd->node_class == ama::N_TYPED_OBJECT || nd->node_class == ama::N_COMMA ||
			nd->node_class == ama::N_FUNCTION || nd->node_class == ama::N_CLASS || 
			nd->node_class == ama::N_ASSIGNMENT || nd->node_class == ama::N_CONDITIONAL || 
			nd->node_class == ama::N_LABELED || nd->node_class == ama::N_SCOPED_STATEMENT || nd->node_class == ama::N_EXTENSION_CLAUSE || 
			(nd->node_class == ama::N_RAW && (nd->flags & 0xffff) == 0)) ) {
				ama::Node* nd_last = nd->LastChild();
				if ( nd_last->comments_after.size() ) {
					nd->comments_after = (nd->comments_after + nd_last->comments_after);
					nd_last->comments_after = "";
				}
			}
		}
		return nd_root;
	}
	//ama::Node*+! StripBinaryOperatorSpaces(ama::Node*+! nd_root) {
	//	for(ama::Node*! nd : nd_root.FindAllWithin(0,ama::N_BINOP, NULL)) {
	//		ama::Node*+! nd_a = nd.c;
	//		if( nd_a.comments_after.endsWith(' ') ) {
	//			nd_a.comments_after = new char[|]!(nd_a.comments_after.subarray(0, nd_a.comments_after.length - 1));
	//		}
	//		ama::Node*+! nd_b = nd.c.s;
	//		if( nd_b.comments_before.startsWith(' ') ) {
	//			nd_b.comments_before = new char[|]!(nd_b.comments_before.subarray(1));
	//		}
	//	}
	//	return nd_root;
	//}
	static void TrimComment(ama::gcstring& comment) {
		intptr_t p0 = 0L;
		intptr_t p1 = comment.size();
		for (; p0 < p1 && uint8_t(comment[p0]) <= uint8_t(' ');) {
			p0++;
		}
		while (p0 < p1 && uint8_t(comment[p1 - 1]) <= uint8_t(' ')) {
			p1--;
		}
		if (p0 > 0 || p1 < intptr_t(comment.size())) {
			comment = comment--->subarray(p0, p1 - p0);
		}
	}
	ama::Node* AutoFormat(ama::Node* nd_root) {
		for (ama::Node* ndi = nd_root->PostorderFirst(); ndi; ndi = ndi->PostorderNext(nd_root)) {
			ndi->indent_level = 0;
			//if (!(ndi->p && ndi->p->node_class == ama::N_RAW)) {
			//	TrimComment(ndi->comments_before);
			//	TrimComment(ndi->comments_after);
			//}
		}
		for (ama::Node* ndi = nd_root->PostorderFirst(); ndi; ndi = ndi->PostorderNext(nd_root)) {
			ama::Node* nd = ndi->p;
			if ( ndi != nd_root ) {
				if ( (nd->node_class == ama::N_SCOPE || nd->node_class == ama::N_FILE) && ndi->node_class != ama::N_SYMBOL ) {
					if ( nd->node_class == ama::N_SCOPE ) {
						ndi->AdjustIndentLevel(4);
					}
					if ( !ndi->comments_before.size() ) {
						ndi->comments_before = "\n";
					}
					if ( !ndi->s && !ndi->comments_after.size() ) {
						ndi->comments_after = "\n";
					}
				}
				if ( nd->node_class == ama::N_CLASS && ndi == nd->c->s && ndi->comments_before == "" ) {
					ndi->comments_before = " ";
				}
			}
			//if ( ndi->node_class == ama::N_SCOPE && ndi->c ) {
			//	if ( !ndi->LastChild()->comments_after.size() ) { ndi->LastChild()->comments_after = "\n"; }
			//}
		}
		if ( nd_root->p ) {
			//sane new line before / after
			int is_one_liner = 1;
			int has_other_child = 0;
			for (ama::Node* ndi = nd_root->p->c; ndi; ndi = ndi->s) {
				if ( ndi != nd_root ) {
					if ( !has_other_child ) {
						nd_root->AdjustIndentLevel(ndi->indent_level);
					}
					has_other_child = 1;
					if ( ndi->comments_before--->endsWith('\n') || ndi->comments_after--->endsWith('\n') ) {
						is_one_liner = 0;
						break;
					}
				}
			}
			if ( !(is_one_liner && has_other_child) ) {
				//we need newlines
				if ( nd_root->comments_before == "" ) {
					ama::Node* nd_prev = nd_root->Prev();
					if ( !(nd_prev && nd_prev->comments_after--->endsWith('\n')) ) {
						nd_root->comments_before = "\n";
					}
				}
				if ( !nd_root->s && nd_root->comments_after == "" ) {
					nd_root->comments_after = "\n";
				}
			}
		}
		return nd_root;
	}
	//also finalizes N_OBJECT vs N_SCOPE ambiguity
	ama::Node* NodifySemicolonAndParenthesis(ama::Node* nd_root) {
		for (ama::Node* ndi = nd_root; ndi; ndi = ndi->PreorderNext(nullptr)) {
			std::span<char> symbol = "";
			if (ndi == nd_root || ndi->node_class == ama::N_SCOPE) {
				symbol = ";";
			} else if (ndi->node_class == ama::N_ARRAY || ndi->node_class == ama::N_OBJECT) {
				symbol = ",";
			}
			if (symbol.size()) {
				for (ama::Node* ndj = ndi->c; ndj; ndj = ndj->s) {
					while ( ndj->s && ndj->s->isSymbol(symbol) ) {
						ama::Node* nd_semicolon = ndj->s;
						ndj->MergeCommentsAfter(nd_semicolon);
						ndj->Unlink();
						nd_semicolon->flags = symbol == "," ? ama::SEMICOLON_COMMA : 0;
						nd_semicolon->node_class = ama::N_DELIMITED;
						nd_semicolon->data = "";
						nd_semicolon->indent_level = ndj->indent_level;
						nd_semicolon->Insert(ama::POS_FRONT, ndj);
						std::swap(nd_semicolon->comments_before, ndj->comments_before);
						ndj->indent_level = 0;
						ndj = nd_semicolon;
					}
				}
			}
			/////////////////
			if ( ndi->isRawNode('(', ')') && ndi->c && !ndi->c->s ) {
				ndi->node_class = ama::N_PAREN;
				ndi->flags = 0;
			} else if ( ndi->isRawNode(0, 0) && ndi->c && (ndi->LastChild()->isSymbol(";") || ndi->LastChild()->isSymbol(",")) ) {
				ama::Node* nd_sym = ndi->LastChild();
				nd_sym->Unlink();
				ama::Node* nd_body = ama::toSingleNode(ndi->BreakChild());
				nd_body->MergeCommentsAfter(nd_sym);
				ndi = ndi->ReplaceWith(ama::CreateNode(ama::N_DELIMITED, nd_body)->setFlags(nd_sym->data == "," ? ama::DELIMITED_COMMA : 0)->setCommentsAfter(nd_sym->comments_after));
			}
			/////////////////
			if (ndi->node_class == ama::N_SCOPE && !(ndi->flags & ama::SCOPE_FROM_INDENT) && (!ndi->c || ndi->c->node_class != ama::N_DELIMITED && !ndi->c->s) && ndi->p) {
				//could be N_OBJECT
				if (ndi->p->node_class == ama::N_CALL || ndi->p->node_class == ama::N_CALL_TEMPLATE ||
				ndi->p->node_class == ama::N_CALL_CUDA_KERNEL || ndi->p->node_class == ama::N_ITEM ||
				ndi->p->node_class == ama::N_DOT || ndi->p->node_class == ama::N_ARRAY ||
				ndi->p->node_class == ama::N_OBJECT || ndi->p->node_class == ama::N_MOV ||
				ndi->p->isStatement("return")) {
					ndi->node_class = ama::N_OBJECT;
				}
			}
		}
		return nd_root;
	}
};
