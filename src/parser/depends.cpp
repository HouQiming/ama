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
	void ParseCInclude(ama::Node* nd_root) {
		std::string dir_base = (nd_root->data.empty() ? ".":path::dirname(nd_root->data));
		for ( ama::Node* nd_raw: nd_root->FindAllWithin(0, ama::N_RAW) ) {
			if ( (nd_raw->flags & 0xffff) != 0 ) { continue; }
			if ( !(nd_raw->c && nd_raw->c->node_class == ama::N_REF && nd_raw->c->data == "#include") ) { continue; }
			uint32_t flags = ama::DEP_C_INCLUDE;
			if ( !nd_raw->c->s || nd_raw->c->s->s || nd_raw->c->s->node_class != ama::N_STRING ) {
				//#include <foo/bar.baz>, join all children and make them string
				flags |= ama::DEPF_C_INCLUDE_NONSTR;
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
				for ( ama::Node* const &ndi: toDrop ) {
					ndi->s = nullptr;
					ndi->p = nullptr;
					ndi->FreeASTStorage();
				}
				nd_real_include->p = nd_raw;
				nd_raw->c->Insert(ama::POS_AFTER, nd_real_include);
			}
			ama::Node* nd_included = nd_raw->c->BreakSibling();
			assert(nd_included->node_class == ama::N_STRING);
			ama::gcstring fn_included = nd_included->GetStringValue();
			nd_raw->ReplaceWith(ama::nDependency(nd_included)->setFlags(flags));
			nd_raw->FreeASTStorage();
		}
	}
	void ParseJSRequire(ama::Node* nd_root) {
		std::vector<ama::gcstring> js_require_paths{};
		JSValue js_js_require_paths = JS_GetPropertyStr(ama::jsctx, JS_GetGlobalObject(ama::jsctx), "js_require_paths");
		if ( !JS_IsUndefined(js_js_require_paths) ) {
			int32_t lg = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, js_js_require_paths, "length"), 0);
			for (int i = 0; i < lg; i += 1) {
				js_require_paths.push_back(ama::UnwrapString(
					JS_GetPropertyUint32(ama::jsctx, js_js_require_paths, i)
				));
			}
		}
		std::string dir_base = (nd_root->data.empty() ? ".":path::dirname(nd_root->data));
		for ( ama::Node* nd_require: nd_root->FindAllWithin(0, ama::N_CALL, "require") ) {
			if ( !nd_require->c->s || nd_require->c->s->node_class != ama::N_STRING || nd_require->c->s->s ) { continue; }
			ama::gcstring fn_required = nd_require->c->s->GetStringValue();
			nd_require->ReplaceWith(ama::nDependency(nd_require->c->BreakSibling())->setFlags(ama::DEP_JS_REQUIRE));
			nd_require->FreeASTStorage();
		}
		//re-check all nDependency and fill .data
		for ( ama::Node* nd_dep: nd_root->FindAllWithin(0, ama::N_DEPENDENCY) ) {
			if ( (nd_dep->flags & ama::DEP_TYPE_MASK) == ama::DEP_JS_REQUIRE && nd_dep->data.empty() ) {
				ama::gcstring fn_required = nd_dep->GetName();
				if ( fn_required.empty() ) { continue; }
				ama::gcstring fn_commonjs{};
				for ( ama::gcstring const& dir: js_require_paths ) {
					fn_commonjs = ama::FindCommonJSModuleByPath(ama::gcstring(path::normalize(JC::string_concat(dir, path::sep, fn_required))));
					if ( !fn_commonjs.empty() ) { break; }
				}
				if ( fn_commonjs.empty() ) {
					fn_commonjs = ama::FindCommonJSModule(fn_required, ama::gcstring(dir_base));
				}
				if ( !fn_commonjs.empty() ) {
					fn_commonjs = ama::gcstring(path::CPathResolver{}.add(fn_commonjs)->done());
					nd_dep->data = fn_commonjs;
				}
			}
		}
	}
	ama::Node* ParseDependency(ama::Node* nd_root, JSValueConst options) {
		int32_t enable_c_include = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "enable_c_include"), 1);
		if ( enable_c_include ) {
			ParseCInclude(nd_root);
		}
		/////////////////
		int32_t enable_js_require = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "enable_js_require"), 1);
		if ( enable_js_require ) {
			ParseJSRequire(nd_root);
		}
		return nd_root;
	}
};
