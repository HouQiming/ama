#include <string>
#include <vector>
#include "../util/jc_array.h"
#include "../util/path.hpp"
#include "../util/fs.hpp"
#include "../ast/node.hpp"
#include "../script/jsenv.hpp"
#include "depends.hpp"
namespace ama {
	//requires DelimitCLikeStatements, preferrably before ParsePostfix
	ama::Node* ParseCInclude(ama::Node* nd_root) {
		std::string dir_base = (nd_root->data.empty() ? "." : path::dirname(nd_root->data));
		for ( ama::Node * nd_raw: nd_root->FindAllWithin(0, ama::N_RAW) ) {
			if ( (nd_raw->flags & 0xffff) != 0 ) { continue; }
			if ( !(nd_raw->c && nd_raw->c->node_class == ama::N_REF && nd_raw->c->data == "#include") ) { continue; }
			//u32 flags = ama::DEP_C_INCLUDE;
			if ( !nd_raw->c->s || nd_raw->c->s->s || nd_raw->c->s->node_class != ama::N_STRING ) {
				//#include <foo/bar.baz>, join all children and make them string
				//flags |= ama::DEPF_C_INCLUDE_NONSTR;
				std::string comments_before{};
				std::string include_expr{};
				std::string comments_after{};
				std::vector<ama::Node*> toDrop{};
				for (ama::Node* ndi = nd_raw->c->BreakSibling(); ndi; ndi = ndi->s) {
					if ( ndi->node_class == ama::N_SCOPE ) {
						//mistakenly-merged scope
						ndi->BreakSelf();
						nd_raw->Insert(ama::POS_AFTER, ndi);
						break;
					}
					comments_before--->push(ndi->comments_before);
					ndi->comments_before = "";
					comments_after--->push(ndi->comments_after);
					ndi->comments_after = "";
					include_expr--->push(ndi->toSource());
					toDrop.push_back(ndi);
				}
				ama::Node* nd_real_include = ama::nString(ama::gcstring(include_expr))->setCommentsBefore(
					ama::gcstring(comments_before)
				)->setCommentsAfter(
					ama::gcstring(comments_after)
				);
				for ( ama::Node* const & ndi: toDrop ) {
					ndi->s = nullptr;
					ndi->p = nullptr;
					ndi->FreeASTStorage();
				}
				nd_real_include->p = nd_raw;
				nd_real_include->flags = ama::LITERAL_PARSED | ama::STRING_C_STD_INCLUDE;
				nd_raw->c->Insert(ama::POS_AFTER, nd_real_include);
			}
			ama::Node* nd_included = nd_raw->c->BreakSibling();
			assert(nd_included->node_class == ama::N_STRING);
			ama::gcstring fn_included = nd_included->GetStringValue();
			nd_raw->ReplaceWith(ama::CreateNode(ama::N_KEYWORD_STATEMENT, nd_included)->setData("#include"));
			nd_raw->FreeASTStorage();
		}
		return nd_root;
	}
	static const int32_t MODE_NONE = 0;
	static const int32_t MODE_FROM = 1;
	static const int32_t MODE_IMPORT = 2;
	ama::Node* ParseImport(ama::Node* nd_root) {
		//here we only clean up just enough to resolve the target file path (or URL)
		//resolve the imported names separately when necessary
		for ( ama::Node * nd_raw: nd_root->FindAllWithin(0, ama::N_RAW) ) {
			if ( (nd_raw->flags & 0xffff) != 0 ) { continue; }
			if ( !(nd_raw->c && nd_raw->c->node_class == ama::N_REF && (nd_raw->c->data == "from" || nd_raw->c->data == "import")) ) { continue; }
			ama::Node* ndi = nd_raw->c;
			ama::Node* nd_from = nullptr;
			ama::Node* nd_import = nullptr;
			int32_t mode = MODE_NONE;
			uint32_t flags = 0;
			while (ndi) {
				again:
				ama::Node* ndi_next = ndi->s;
				if (ndi->isRef("from") && !(flags & ama::IMPORT_HAS_FROM)) {
					flags |= ama::IMPORT_HAS_FROM;
					if (mode == MODE_NONE) {flags |= ama::IMPORT_FROM_FIRST;}
					nd_from = ndi;
					mode = MODE_FROM;
				} else if (ndi->isRef("import") && !(flags & ama::IMPORT_HAS_IMPORT)) {
					flags |= ama::IMPORT_HAS_IMPORT;
					nd_import = ndi;
					mode = MODE_IMPORT;
				}
				if (ndi_next) {
					if (mode == MODE_IMPORT && ndi_next->isSymbol(".*")) {
						ndi->MergeCommentsAfter(ndi_next);
						ndi_next->Unlink();
						ama::Node* nd_tmp = ama::GetPlaceHolder();
						ndi->ReplaceWith(nd_tmp);
						nd_tmp->ReplaceWith(ama::nPostfix(ndi, ".*")->setCommentsAfter(ndi_next->comments_after));
						goto again;
					} else if (mode == MODE_IMPORT && ndi_next->isRef("as") && ndi_next->s) {
						//the `as` operator, make binop no matter what ndi is (it could be '*')
						ama::Node* nd_alias = ndi_next->s;
						ndi->MergeCommentsAfter(ndi_next);
						nd_alias->MergeCommentsBefore(ndi_next);
						ndi_next->Unlink();
						ama::Node* nd_binop = ama::CreateNode(ama::N_BINOP, nullptr)->setData("as");
						ndi->ReplaceUpto(nd_alias, nd_binop);
						//ndi is still connected to nd_alias
						nd_binop->Insert(ama::POS_FRONT, ndi);
						ndi = nd_binop;
						goto again;
					}
				}
				ndi = ndi_next;
			}
			ama::Node* nd_final = ama::CreateNode(ama::N_IMPORT, nullptr)->setFlags(flags);
			ama::Node* nd_from_body = nullptr;
			ama::Node* nd_import_body = nullptr;
			if (nd_from) {
				if (nd_from->Prev()) {
					nd_from->Prev()->MergeCommentsAfter(nd_from);
				} else {
					std::swap(nd_final->comments_before, nd_from->comments_before);
				}
			}
			if (nd_import) {
				if (nd_import->Prev()) {
					nd_import->Prev()->MergeCommentsAfter(nd_import);
				} else {
					std::swap(nd_final->comments_before, nd_import->comments_before);
				}
			}
			//////////////
			if (nd_from) {
				nd_from_body = nd_from->BreakSibling()->toSingleNode();
				nd_from_body->MergeCommentsBefore(nd_from);
				nd_from->Unlink();
			} else {
				nd_from_body = ama::nAir();
			}
			if (nd_import) {
				nd_import_body = nd_import->BreakSibling()->toSingleNode();
				nd_import_body->MergeCommentsBefore(nd_import);
				nd_import->Unlink();
			} else {
				nd_import_body = ama::nAir();
			}
			nd_final->Insert(ama::POS_FRONT, ama::cons(nd_import_body, nd_from_body));
			nd_raw->ReplaceWith(nd_final);
		}
		return nd_root;
	}
};
