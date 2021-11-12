#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <array>
#if defined(_WIN32)
	#include <windows.h>
	/*#pragma add("ldflags", "kernel32.lib");*/
#else
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#include <dlfcn.h>
	/*#pragma add("ldflags", "-ldl");*/
#endif
#include "../util/jc_array.h"
#include "../util/gcstring.h"
#include "../../modules/cpp/json/json.h"
#include "../util/path.hpp"
#include "../util/fs.hpp"
#include "../util/env.hpp"
#include "../util/unicode.hpp"
#include "../util/unicode/case.hpp"
#include "./quickjs/src/quickjs.h"
#include "../ast/node.hpp"
#include "../ast/nodegc.hpp"
#include "../codegen/gen.hpp"
#include "../parser/simppair.hpp"
#include "../parser/scoping.hpp"
#include "../parser/depends.hpp"
#include "../parser/postfix.hpp"
#include "../parser/operator.hpp"
#include "../parser/decl.hpp"
#include "../parser/cleanup.hpp"
#include "../parser/findama.hpp"
#include "nodeof_to_ast.hpp"
#include "jsenv.hpp"
#include "jsapi.hpp"
#include "jsgen.hpp"
ama::Node * ama::Node::Unparse() {
	switch ( this->node_class ) {
		default: {
			return this;
		}
		case ama::N_RAW: {
			return ama::UnparseRaw(this);
		}
		case ama::N_BINOP: {
			return ama::UnparseBinop(this);
		}
		case ama::N_CALL: {
			return ama::UnparseCall(this);
		}
		case ama::N_LABELED: {
			return ama::UnparseLabel(this);
		}
		case ama::N_PREFIX: {
			return ama::UnparsePrefix(this);
		}
	}
}
namespace ama {
	void LazyInitScriptEnv();
};
namespace ama {
	static JSValueConst g_require_cache = JS_NULL;
	static std::string GetScriptJSCode(std::span<char> fn) {
		std::string their_extname = unicode::toLowerASCII(path::extname(fn));
		JC::StringOrError s_code = fs::readFileSync(fn);
		if ( !s_code ) {
			//console::error('failed to read', fn);
			return std::string();
		}
		if ( their_extname == ".json" ) {
			return JC::string_concat("(function(exports,module){module.exports=JSON.parse(", JSON::stringify(s_code.some), ")})");
		} else {
			if ( fn--->endsWith(".ama.js") ) {
				ama::Node* nd_root = ParseCode(s_code->c_str(), JS_UNDEFINED);
				if (nd_root) {
					for (ama::Node* nd: nd_root->FindAll(ama::N_BINOP, "!=")) {
						nd->data = "!==";
					}
					for (ama::Node* nd: nd_root->FindAll(ama::N_BINOP, "==")) {
						nd->data = "===";
					}
					ama::NodeofToASTExpression(nd_root);
					s_code.some = nd_root->toSource();
				}
			}
			return JC::string_concat("(function(exports,module,__filename,__dirname){let require=__require.bind(null,__filename);", s_code.some, "\n})\n");
		}
	}
	static std::string ResolveJSRequire(std::span<char> fn_base, std::span<char> fn_required) {
		//ResolveJSRequire's result is used in JS-running functions, so don't return ama::gcstring
		std::string dir_base = path::dirname((path::CPathResolver{}).add(fn_base)->done());
		std::string fn_final = (path::CPathResolver{}).add(dir_base)->add(fn_required)->done();
		ama::gcstring fn_commonjs{};
		if ( !fn_required--->startsWith(".") && !fn_required--->startsWith("/") ) {
			//it's a standard module
			//COULDDO: internal asset strings - faster and simpler than zip
			fn_commonjs = ama::FindCommonJSModuleByPath((path::normalize(JC::string_concat(ama::std_module_dir, path::sep, fn_required))));
			if ( fn_commonjs.empty() ) {
				fn_commonjs = ama::FindCommonJSModuleByPath((path::normalize(JC::string_concat(ama::std_module_dir_global, path::sep, fn_required))));
			}
			if ( fn_commonjs.empty() ) {
				fn_commonjs = ama::FindCommonJSModule((fn_required), (dir_base));
			}
			if ( !fn_commonjs.empty() ) {
				fn_final = JC::array_cast<std::string>(fn_commonjs);
			}
		} else {
			fn_commonjs = ama::FindCommonJSModuleByPath((fn_final));
			if ( !fn_commonjs.empty() ) {
				fn_final = JC::array_cast<std::string>(fn_commonjs);
			}
		}
		return std::move(fn_final);
	}
	static JSValueConst JSResolveJSRequire(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 2 ) {
			return JS_ThrowReferenceError(ctx, "need base name and required name");
		}
		std::span<char> fn_base = ama::UnwrapStringSpan(argv[intptr_t(0L)]);
		std::span<char> fn_required = ama::UnwrapStringSpan(argv[intptr_t(1L)]);
		return ama::WrapString(ResolveJSRequire(fn_base, fn_required));
	}
	static JSValueConst JSRequire(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		//custom CommonJS module loader
		//we MUST NOT hold any ama::gcstring when calling JS - it could get gc-ed
		std::string fn_final{};
		{
			std::span<char> fn_base = ama::UnwrapStringSpan(argv[intptr_t(0L)]);
			std::span<char> fn_required = ama::UnwrapStringSpan(argv[intptr_t(1L)]);
			fn_final = ResolveJSRequire(fn_base, fn_required);
		}
		/////////////
		JSValueConst obj_module = JS_GetPropertyStr(ctx, g_require_cache, fn_final.c_str());
		if ( JS_IsUndefined(obj_module) ) {
			obj_module = JS_NewObject(ctx);
			JSValueConst obj_exports = JS_NewObject(ctx);
			JS_SetPropertyStr(ctx, obj_module, "exports", JS_DupValue(ama::jsctx, obj_exports));
			JS_SetPropertyStr(ctx, g_require_cache, fn_final.c_str(), JS_DupValue(ama::jsctx, obj_module));
			JS_SetPropertyStr(ctx, obj_module, "filename", ama::WrapString(fn_final));
			//////////////
			//char[+]! loader_key = path.extname(fn_final) + ' loader';
			//JSValue! custom_loader = JS_GetPropertyStr(ctx, g_require_cache, loader_key.str());
			std::string ext = path::extname(fn_final);
			std::array<JSValueConst, intptr_t(4L)> module_args{
				obj_exports, //
				obj_module, ama::WrapString(fn_final), ama::WrapString(path::dirname(fn_final))
			};
			if ( ext == ".so" || ext == ".dll" || ext == ".dylib" || ext == ".amaso" ) {
				JSValue js_fn_final = ama::WrapString(fn_final);
				JSValueConst ret = JS_Invoke(ctx, JS_GetGlobalObject(ctx), JS_NewAtom(ctx, "__RequireNativeLibrary"), 4, module_args.data());
				for (intptr_t i = 0; i < 4; i++) {
					JS_FreeValue(ctx, module_args[i]);
				}
				if ( JS_IsException(ret) ) {
					JS_SetPropertyStr(ctx, g_require_cache, fn_final.c_str(), JS_UNDEFINED);
					return ret;
				}
				JS_FreeValue(ctx, ret);
			} else {
				//////////////
				std::string s_fixed_code = GetScriptJSCode(fn_final);
				if ( !s_fixed_code.size() ) {
					JS_SetPropertyStr(ctx, g_require_cache, fn_final.c_str(), JS_UNDEFINED);
					return JS_ThrowReferenceError(ctx, "module `%s` not found", JS_ToCString(ctx, argv[intptr_t(1L)]));
				}
				JSValueConst ret = JS_Eval(
					ctx, (char const*)(s_fixed_code.data()),
					uint64_t(uintptr_t(s_fixed_code.size())), fn_final.c_str(), JS_EVAL_TYPE_GLOBAL
				);
				//console.log(fn_final,raw.JS_IsException(ret))
				if ( JS_IsException(ret) ) {
					JS_SetPropertyStr(ctx, g_require_cache, fn_final.c_str(), JS_UNDEFINED);
					return ret;
				}
				JSValueConst ret_module = JS_Call(ctx, ret, JS_GetGlobalObject(ctx), 4, module_args.data());
				for (intptr_t i = 0; i < 4; i++) {
					JS_FreeValue(ctx, module_args[i]);
				}
				if ( JS_IsException(ret_module) ) {
					JS_SetPropertyStr(ctx, g_require_cache, fn_final.c_str(), JS_UNDEFINED);
					return ret_module;
				}
				JS_FreeValue(ctx, ret);
			}
			//after SetPropertyStr obj_module may have gotten freed
			//}
			//set it in the end so that one can re-require a module after an exception
			//JS_FreeValue(ctx,obj_module2)
		}
		return JS_GetPropertyStr(ctx, obj_module, "exports");
	}
	static JSValueConst JSGetEnv(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a name");
		}
		JC::StringOrError ret = ENV::get(ama::UnwrapString(argv[0]));
		if ( !ret ) { return JS_UNDEFINED; }
		return ama::WrapString(ret.some);
	}
	void DumpASTAsJSON(ama::Node* nd) {
		LazyInitScriptEnv();
		printf("%s\n", (char const*)(JS_ToCString(ama::jsctx, JS_JSONStringify(ama::jsctx, ama::WrapNode(nd), JS_NULL, JS_NewInt32(ama::jsctx, 2)))));
	}
	ama::Node* ParseCode(char const* code, JSValue options) {
		//for external code that calls ParseCode as an API
		LazyInitScriptEnv();
		options = ama::InheritOptions(options);
		ama::Node* nd_root = ama::ParseSimplePairing(code, options);
		if ( ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_indent_as_scope"), 0) ) {
			ama::ConvertIndentToScope(nd_root, options);
		}
		if ( ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_pointed_brackets"), 1) ) {
			ama::ParsePointedBrackets(nd_root);
		}
		ama::DelimitCLikeStatements(nd_root, options);
		//from here on, N_RAW no longer includes the root scope
		ama::CleanupDummyRaws(nd_root);
		nd_root->node_class = ama::N_FILE;
		//reset the flag, it could have been changed to '{','}' by ConvertIndentToScope
		nd_root->flags = (nd_root->flags & 0x10000) ? ama::FILE_SPACE_INDENT : 0;
		ama::ParseDependency(nd_root, options);
		ama::ParsePostfix(nd_root, options);
		ama::SanitizeCommentPlacement(nd_root);
		ama::CleanupDummyRaws(nd_root);
		if ( ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_keyword_statements"), 1) ) {
			ama::ParseKeywordStatements(nd_root, options);
		}
		if ( ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_scoped_statements"), 1) ) {
			ama::ParseScopedStatements(nd_root, options);
		}
		ama::ParseAssignment(nd_root, options);
		int has_c_conditional = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_c_conditional"), 1);
		int has_labels = ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_labels"), 1);
		if ( has_c_conditional || has_labels ) {
			ama::ParseColons(nd_root, has_c_conditional);
		}
		//nd_root.Validate();
		if ( ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_operators"), 1) ) {
			ama::ParseOperators(nd_root, options);
		}
		ama::CleanupDummyRaws(nd_root);
		ama::FixPriorityReversal(nd_root);
		if ( ama::UnwrapInt32(JS_GetPropertyStr(ama::jsctx, options, "parse_declarations"), 1) ) {
			ama::ParseDeclarations(nd_root, options);
		}
		//DumpASTAsJSON(nd_root);
		ama::NodifySemicolonAndParenthesis(nd_root);
		ama::CleanupDummyRaws(nd_root);
		ama::SanitizeCommentPlacement(nd_root);
		assert(!nd_root->p);
		return nd_root;
	}
	static JSValueConst JSParseCurrentFile(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv, int magic, JSValue* func_data) {
		JSValue options{};
		if ( argc < 1 ) {
			options = JS_NULL;
		} else {
			options = argv[0];
		}
		//extension-aware option preparation, default inheritance if necessary
		std::array<JSValueConst, intptr_t(2L)> prepare_option_args{
			func_data[1],
			options
		};
		options = JS_Invoke(ama::jsctx, JS_GetGlobalObject(ama::jsctx), JS_NewAtom(ama::jsctx, "__PrepareOptions"), 2, prepare_option_args.data());
		options = ama::InheritOptions(options);
		//JS_DupValue(ama::jsctx, func_data[1])
		char const* file_data = (char const*)(JSON::parse<uintptr_t>(ama::UnwrapString(func_data[0])));
		ama::Node* nd_root = ParseCode(file_data, options);
		nd_root->data = ama::UnwrapString(func_data[1]);
		return ama::WrapNode(nd_root);
	}
	static JSValueConst JSParseCode(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		JSValue options{};
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a code string");
		}
		if ( argc < 2 ) {
			options = JS_NULL;
		} else {
			options = argv[1];
		}
		return ama::WrapNode(ParseCode(JS_ToCString(ama::jsctx, argv[0]), options));
	}
	static JSValueConst JSGenerateCode(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		return ama::WrapString(ama::GenerateCode(ama::UnwrapNode(this_val), argc > 0 ? argv[0] : JS_NULL));
	}
	static JSValueConst JSGetUniqueTag(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		ama::Node* nd = ama::UnwrapNode(this_val);
		return ama::WrapString(JSON::stringify(uintptr_t(nd)));
	}
	static JSValueConst JSGetNodeFromUniqueTag(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if (argc < 1) {
			return JS_ThrowReferenceError(ctx, "need a tag");
		}
		ama::Node* addr = (ama::Node*)(JSON::parse<intptr_t>(ama::UnwrapStringSpan(argv[0])));
		if (ama::isValidNodePointer(addr)) {
			return ama::WrapNode(addr);
		} else {
			return JS_ThrowReferenceError(ctx, "invalid tag");
		}
	}
	static JSValueConst JSConsoleFlush(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		fflush(stdout);
		fflush(stderr);
		return JS_UNDEFINED;
	}
	int RunScriptOnFile(std::span<char> script, char const* file_name, char const* file_data) {
		//we must not hold any Node* here: the script could gc them and native-only pointers can get freed
		LazyInitScriptEnv();
		std::string s_fixed_code = JC::string_concat("(function(__filename,__dirname,ParseCurrentFile){\"use strict\";let require=__require.bind(null,__filename);", script, "\n})\n");
		JSValueConst ret = JS_Eval(
			ama::jsctx, s_fixed_code.data(),
			s_fixed_code.size(), file_name, JS_EVAL_TYPE_GLOBAL
		);
		if ( JS_IsException(ret) ) {
			//console.log(s_fixed_code);
			ama::DumpError(ama::jsctx);
			return 0;
		}
		////////////
		//the most light weight method to pass a pointer...
		std::array<JSValueConst, intptr_t(2L)> func_data{
			JS_NewString(ama::jsctx, JSON::stringify(uintptr_t(file_data)).c_str()),
			JS_NewString(ama::jsctx, file_name)
		};
		std::string dirname = path::dirname(JC::toSpan(file_name));
		if (!dirname.size()) {
			dirname.push_back('.');
		}
		std::array<JSValueConst, intptr_t(3L)> module_args{
			JS_NewString(ama::jsctx, file_name),
			ama::WrapString(path::toAbsolute(dirname)),
			JS_NewCFunctionData(
				ama::jsctx, JSParseCurrentFile,
				1, 0, 2, func_data.data()
			)
		};
		JSValueConst ret_script = JS_Call(ama::jsctx, ret, JS_GetGlobalObject(ama::jsctx), 3, module_args.data());
		for (intptr_t i = 0; i < 3; i++) {
			JS_FreeValue(ama::jsctx, module_args[i]);
		}
		JS_FreeValue(ama::jsctx, ret);
		if ( JS_IsException(ret_script) ) {
			ama::DumpError(ama::jsctx);
			return 0;
		}
		return 1;
	}
	int ProcessAmaFile(char const* fn, std::span<char> extra_script) {
		LazyInitScriptEnv();
		JC::StringOrError file_data = fs::readFileSync(JC::toSpan(fn));
		if ( !file_data ) {
			//console.error('unable to load', fn);
			return ama::PROCESS_AMA_NOT_FOUND;
		}
		std::string script_i = ama::FindAma(file_data.some);
		if ( extra_script.size() ) {
			script_i = JC::string_concat(extra_script, script_i);
		}
		//allow .(foo) in ama code
		if ( script_i--->indexOf(".(") >= 0 || script_i--->indexOf(".{") >= 0 ) {
			ama::Node* nd_root = ParseCode(script_i.c_str(), JS_UNDEFINED);
			ama::NodeofToASTExpression(nd_root);
			script_i = nd_root->toSource();
		}
		if ( !ama::RunScriptOnFile(script_i, fn, file_data->c_str()) ) {
			return ama::PROCESS_AMA_SCRIPT_FAILED;
		}
		return ama::PROCESS_AMA_SUCCESS;
	}
	static JSValueConst JSProcessAmaFile(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a file name");
		}
		return JS_NewInt32(ctx, ProcessAmaFile(JS_ToCString(ctx, argv[0]), argc < 2 ? (ama::gcstring("")) : ama::UnwrapString(argv[1])));
	}
	static JSValueConst JSCreateNode(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv, int magic) {
		ama::Node* c = (ama::Node*)(nullptr);
		ama::gcstring data{};
		for (int i = argc - 1; i >= 0; --i) {
			if ( data.empty() && JS_IsString(argv[i]) ) {
				data = ama::UnwrapString(argv[i]);
				continue;
			}
			if ( JS_IsNull(argv[i]) || JS_IsUndefined(argv[i]) ) { continue; }
			ama::Node* ndi = ama::UnwrapNode(argv[intptr_t(i)]);
			if ( !ndi ) {
				return JS_ThrowTypeError(ctx, "child %d is not a node", i);
			}
			if ( ndi->s ) {
				ama::Node* ndj = ndi;
				while ( ndj->s ) {
					ndj = ndj->s;
				}
				ama::cons(ndj, c);
				c = ndi;
			} else {
				c = ama::cons(ndi, c);
			}
		}
		ama::Node* nd = ama::CreateNode(magic, c)->setData(data);
		if ( magic == ama::N_STRING ) {
			nd->flags = ama::LITERAL_PARSED;
		}
		return ama::WrapNode(nd);
	}
	static JSValueConst JSCreateNodeRaw(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		//mutable ama::Node*+! c = (ama::Node*+)(NULL);
		//for(mutable int! i = argc - 1; i >= 1; --i) {
		//	const ama::Node*+! ndi = ama::UnwrapNode(argv[intptr_t(i)]);
		//	if( !ndi ) {
		//		return JS_ThrowTypeError(ctx, 'child %d is not a node', i);
		//	}
		//	c = ama::cons(ndi, c);
		//}
		if ( argc < 2 ) {
			return JS_ThrowReferenceError(ctx, "need a class and a node");
		}
		return ama::WrapNode(ama::CreateNode(ama::UnwrapInt32(argv[0], ama::N_NONE), ama::UnwrapNode(argv[1])));
	}
	//COULDDO: move to JS, base on process.stdfoo.write instead
	static JSValueConst JSConsoleWrite(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv, int magic) {
		for (intptr_t i = intptr_t(0L); i < intptr_t(argc); i += 1) {
			if ( magic & 1 ) {
				if ( i ) {
					printf(" ");
				}
			} else {
				if ( i ) {
					fprintf(stderr, " ");
				}
			}
			//////
			JSValueConst arg_i = argv[i];
			JSValueConst str = JS_IsString(arg_i) ? arg_i : JS_JSONStringify(
				ctx, arg_i,
				JS_NULL,
				JS_NewInt32(ctx, 0)
			);
			if ( JS_IsException(str) ) {
				return str;
			}
			char const* s = JS_ToCString(ctx, str);
			if ( magic & 1 ) {
				printf("%s", s);
			} else {
				fprintf(stderr, "%s", s);
			}
		}
		if (!(magic & 2)) {
			if ( magic & 1 ) {
				printf("\n");
			} else {
				fprintf(stderr, "\n");
			}
		}
		return JS_UNDEFINED;
	}
	static void NodeFinalizer(JSRuntime* rt, JSValueConst val) {
		ama::Node* nd = ama::UnwrapNode(val);
		assert(nd->tmp_flags & ama::TMPF_IS_NODE);
		ama::g_js_node_map.erase(nd);
	}
	typedef ama::Node * (*NodeFilter)(ama::Node*);
	typedef ama::Node * (*NodeFilterWithOption)(ama::Node*, JSValueConst);
	struct NodeFilterDesc {
		char const* name{};
		NodeFilter f{};
		NodeFilterWithOption fo{};
	};
	static std::vector<NodeFilterDesc> g_filters{
		NodeFilterDesc{"NodeofToASTExpression", ama::NodeofToASTExpression, nullptr},
		NodeFilterDesc{"CleanupDummyRaws", ama::CleanupDummyRaws, nullptr},
		NodeFilterDesc{"ParseOperators", nullptr, ama::ParseOperators},
		NodeFilterDesc{"AutoFormat", ama::AutoFormat, nullptr}
	};
	//new NodeFilterDesc!{
	//	name: 'StripBinaryOperatorSpaces',
	//	f: ama::StripBinaryOperatorSpaces
	//}
	static JSValueConst JSApplyNodeFilter(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv, int magic) {
		if ( g_filters[magic].fo ) {
			JSValue options = argc >= 1 ? argv[0] : JS_UNDEFINED;
			options = ama::InheritOptions(options);
			return ama::WrapNode(g_filters[magic].fo(ama::UnwrapNode(this_val), options));
		} else {
			return ama::WrapNode(g_filters[magic].f(ama::UnwrapNode(this_val)));
		}
	}
	static JSValueConst JSReadFileSync(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a path");
		}
		JC::StringOrError data = fs::readFileSync(ama::UnwrapString(argv[0]));
		if ( !data ) {
			return JS_NULL;
		}
		return JS_NewArrayBufferCopy(ctx, (uint8_t*)(data->data()), data->size());
	}
	static JSValueConst JSWriteFileSync(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 2 ) {
			return JS_ThrowReferenceError(ctx, "need a path and a string");
		}
		size_t len = int64_t(0uLL);
		char const* ptr = JS_ToCStringLen(ama::jsctx, &len, argv[1]);
		intptr_t n_written = fs::writeFileSync(ama::UnwrapString(argv[0]), std::span<char>(ptr, intptr_t(len)));
		return JS_NewInt64(ctx, n_written);
	}
	static JSValueConst JSExistsSync(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a path");
		}
		return JS_NewBool(ctx, fs::existsSync(ama::UnwrapString(argv[0])));
	}
	#if defined(_WIN32)
		static inline double toUnixTimestamp(FILETIME a) {
			return double(uint64_t(uint32_t(a.dwLowDateTime)) + (uint64_t(uint32_t(a.dwHighDateTime)) << 32)) * 1.0e-4 + -11644473600000.0;
		}
	#endif
	static JSValueConst JSStatSync(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		ama::gcstring fn = ama::UnwrapString(argv[intptr_t(0L)]);
		#if defined(_WIN32)
			std::vector<uint16_t> fnw = fs::PathToWindows(fn);
			WIN32_FIND_DATAW find_data{};
			HANDLE hfind = FindFirstFileW(LPCWSTR(fnw.data()), &find_data);
			if ( hfind && hfind != INVALID_HANDLE_VALUE ) {
				FindClose(hfind);
				JSValueConst ret = JS_NewObject(ctx);
				JS_SetPropertyStr(ctx, ret, "size", JS_NewInt64(ctx, uint64_t(find_data.nFileSizeLow) + (uint64_t(find_data.nFileSizeHigh) << 32)));
				JS_SetPropertyStr(
					ctx, ret, "atimeMs",
					JS_NewFloat64(ctx, ama::toUnixTimestamp(find_data.ftLastAccessTime) * 1000.0)
				);
				JS_SetPropertyStr(
					ctx, ret, "mtimeMs",
					JS_NewFloat64(ctx, ama::toUnixTimestamp(find_data.ftLastWriteTime) * 1000.0)
				);
				JS_SetPropertyStr(
					ctx, ret, "ctimeMs",
					JS_NewFloat64(ctx, ama::toUnixTimestamp(find_data.ftCreationTime) * 1000.0)
				);
				JS_SetPropertyStr(
					ctx, ret, "is_file",
					JS_NewInt32(ctx, 1)
				);
				JS_SetPropertyStr(
					ctx, ret, "is_dir",
					JS_NewInt32(ctx, (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0)
				);
				return ret;
			} else {
				return JS_ThrowReferenceError(ctx, "failed to stat \"%s\"", fn.c_str());
			}
		#else
			struct stat sb {};
			if ( stat(fn.c_str(), &sb) != 0 ) {
				return JS_ThrowReferenceError(ctx, "failed to stat \"%s\"", fn.c_str());
			}
			JSValueConst ret = JS_NewObject(ctx);
			JS_SetPropertyStr(ctx, ret, "dev", JS_NewInt64(ctx, int64_t(sb.st_dev)));
			JS_SetPropertyStr(ctx, ret, "mode", JS_NewInt64(ctx, int64_t(sb.st_mode)));
			JS_SetPropertyStr(ctx, ret, "nlink", JS_NewInt64(ctx, int64_t(sb.st_nlink)));
			JS_SetPropertyStr(ctx, ret, "uid", JS_NewInt64(ctx, int64_t(sb.st_uid)));
			JS_SetPropertyStr(ctx, ret, "gid", JS_NewInt64(ctx, int64_t(sb.st_gid)));
			JS_SetPropertyStr(ctx, ret, "rdev", JS_NewInt64(ctx, int64_t(sb.st_rdev)));
			JS_SetPropertyStr(ctx, ret, "ino", JS_NewInt64(ctx, int64_t(sb.st_ino)));
			JS_SetPropertyStr(ctx, ret, "size", JS_NewInt64(ctx, int64_t(sb.st_size)));
			JS_SetPropertyStr(ctx, ret, "blksize", JS_NewInt64(ctx, int64_t(sb.st_blksize)));
			JS_SetPropertyStr(ctx, ret, "blocks", JS_NewInt64(ctx, int64_t(sb.st_blocks)));
			JS_SetPropertyStr(ctx, ret, "is_file", JS_NewInt32(ctx, 1));
			JS_SetPropertyStr(ctx, ret, "is_dir", JS_NewInt32(ctx, S_ISDIR(sb.st_mode) ? 1 : 0));
			//JS_SetPropertyStr(ctx, ret, 'flags', JS_NewInt64(ctx, i64(sb.st_flags)));
			//JS_SetPropertyStr(ctx, ret, 'gen', JS_NewInt64(ctx, i64(sb.st_gen)));
			JS_SetPropertyStr(
				ctx, ret, "atimeMs",
				JS_NewFloat64(ctx, double(sb.st_atim.tv_sec) * 1000.0)
			);
			JS_SetPropertyStr(
				ctx, ret, "mtimeMs",
				JS_NewFloat64(ctx, double(sb.st_mtim.tv_sec) * 1000.0)
			);
			JS_SetPropertyStr(
				ctx, ret, "ctimeMs",
				JS_NewFloat64(ctx, double(sb.st_ctim.tv_sec) * 1000.0)
			);
			return ret;
		#endif
	}
	static JSValueConst JSPathNormalize(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a path");
		}
		return ama::WrapString(path::normalize(ama::UnwrapString(argv[0])));
	}
	static JSValueConst JSPathToAbsolute(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a path");
		}
		return ama::WrapString(path::toAbsolute(ama::UnwrapString(argv[0])));
	}
	static JSValueConst JSBufferToString(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		size_t size = 0;
		uint8_t const* ptr = JS_GetArrayBuffer(ctx, &size, this_val);
		if ( !ptr ) {
			return JS_ThrowReferenceError(ctx, "the buffer has to be an ArrayBuffer");
		}
		if ( argc > 0 && !JS_IsUndefined(argv[0]) ) {
			char const* encoding = JS_ToCString(ctx, argv[0]);
			if ( strcmp(encoding, "latin1") == 0 || strcmp(encoding, "binary") == 0 || strcmp(encoding, "ascii") == 0 ) {
				std::string ret{};
				for (size_t i = 0; i < size; i++) {
					unicode::AppendUTF8Char(ret, int(uint32_t(ptr[i])));
				}
				return JS_NewStringLen(ctx, (char const*)(ret.data()), ret.size());
			} else if ( strcmp(encoding, "hex") == 0 ) {
				char const* hex = "0123456789abcdef";
				std::string ret{};
				for (size_t i = 0; i < size; i++) {
					ret--->push(hex[int(uint32_t(ptr[i])) >> 4], hex[int(uint32_t(ptr[i])) & 15]);
				}
				return JS_NewStringLen(ctx, (char const*)(ret.data()), ret.size());
			} else if ( strcmp(encoding, "utf8") != 0 && strcmp(encoding, "utf-8") != 0 ) {
				return JS_ThrowReferenceError(ctx, "unsupported encoding %s", encoding);
			}
		}
		return JS_NewStringLen(ctx, (char const*)(ptr), size);
	}
	static JSValueConst JSGetPlaceHolder(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		return ama::WrapNode(ama::GetPlaceHolder());
	}
	static JSValueConst JSPathParse(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a path");
		}
		return ama::WrapString(JSON::stringify(path::parse(ama::UnwrapString(argv[0]))));
	}
	static JSValueConst JSPathIsAbsolute(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a path");
		}
		return JS_NewBool(ctx, path::isAbsolute(ama::UnwrapString(argv[0])));
	}
	static JSValueConst JSPathRelative(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 2 ) {
			return JS_ThrowReferenceError(ctx, "need 2 paths");
		}
		return ama::WrapString(path::relative(ama::UnwrapString(argv[0]), ama::UnwrapString(argv[1])));
	}
	static JSValueConst JSSystem(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a command");
		}
		return JS_NewInt32(ctx, system(JS_ToCString(ctx, argv[0])));
	}
	static JSValueConst JSChdir(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a command");
		}
		return JS_NewInt32(ctx, fs::chdir(ama::UnwrapString(argv[0])));
	}
	static JSValueConst JSCwd(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		return ama::WrapString(fs::cwd());
	}
	static JSValueConst JSByteLength(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a string");
		}
		size_t len = 0;
		JS_ToCStringLen(ctx, &len, argv[0]);
		return JS_NewInt64(ctx, len);
	}
	static JSValueConst JSByteAt(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 2 ) {
			return JS_ThrowReferenceError(ctx, "need a string and a location");
		}
		size_t len = 0;
		char const* s = JS_ToCStringLen(ctx, &len, argv[0]);
		int32_t index = ama::UnwrapInt32(argv[1], -1);
		if ( !s || size_t(index) >= len ) {
			return JS_UNDEFINED;
		}
		return JS_NewInt32(ctx, int32_t(uint32_t(uint8_t(s[index]))));
	}
	static JSValueConst JSByteSubstr(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 2 ) {
			return JS_ThrowReferenceError(ctx, "need a string, a start, and maybe a length");
		}
		size_t len = 0;
		char const* s = JS_ToCStringLen(ctx, &len, argv[0]);
		int32_t index = ama::UnwrapInt32(argv[1], -1);
		int32_t length = argc < 3 ? (len - index) : ama::UnwrapInt32(argv[2], -1);
		if ( !s || size_t(index) > len || size_t(index + length) > len || size_t(index) > size_t(index + length) ) {
			return JS_UNDEFINED;
		}
		return JS_NewStringLen(ctx, s + index, length);
	}
	static JSValueConst JSCons(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 2 ) {
			return JS_ThrowReferenceError(ctx, "need two nodes");
		}
		return ama::WrapNode(ama::cons(ama::UnwrapNode(argv[0]), ama::UnwrapNode(argv[1])));
	}
	static JSValueConst JSNodeGC(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		return JS_NewInt64(ctx, ama::gc());
	}
	static void SetupConfigDirs() {
		///////////
		char const* home_dir = getenv("HOME");
		if ( !home_dir ) {
			home_dir = "/";
		}
		ama::std_module_dir = path::normalize(JC::string_concat(home_dir, path::sep, ".ama_modules"));
		#if defined(_WIN32)
			char const* program_files = getenv("ProgramFiles");
			if ( !program_files ) {
				program_files = "\\";
			}
			ama::std_module_dir_global = path::normalize(JC::string_concat(program_files, path::sep, "ama\\ama_modules"));
		#else
			ama::std_module_dir_global = "/usr/share/ama_modules";
		#endif
	}
	static uint32_t g_native_library_classid = 0u;
	static JSValue g_native_library_proto = JS_UNDEFINED;
	static JSValueConst JSLoadNativeLibrary(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a file name");
		}
		void* hmodule{};
		#if defined(_WIN32)
			hmodule = (void*)(LoadLibraryA(JS_ToCString(ctx, argv[0])));
		#else
			hmodule = dlopen(JS_ToCString(ctx, argv[0]), RTLD_NOW);
		#endif
		if ( !hmodule ) {
			return JS_ThrowReferenceError(ctx, "failed to load module %s", JS_ToCString(ctx, argv[0]));
		}
		JSValue ret = JS_NewObjectProtoClass(ama::jsctx, g_native_library_proto, g_native_library_classid);
		JS_SetOpaque(ret, hmodule);
		return ret;
	}
	static void NativeLibraryFinalizer(JSRuntime* rt, JSValueConst val) {
		void* hmodule = JS_GetOpaque(val, g_native_library_classid);
		if ( hmodule ) {
			JS_SetOpaque(val, nullptr);
			#if defined(_WIN32)
				FreeLibrary(hmodule);
			#else
				dlclose(hmodule);
			#endif
		}
	}
	typedef int32_t(*NativeLibraryFunction)(JSValue);
	static JSValueConst NativeLibraryRun(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if ( argc < 1 ) {
			return JS_ThrowReferenceError(ctx, "need a symbol name and an optional argument");
		}
		void* hmodule = JS_GetOpaque(this_val, g_native_library_classid);
		if ( !hmodule ) {
			return JS_ThrowReferenceError(ctx, "panic: module already unloaded");
		}
		void* addr{};
		#if defined(_WIN32)
			addr = (void*)(GetProcAddress(HMODULE(hmodule), JS_ToCString(ctx, argv[0])));
		#else
			addr = dlsym(hmodule, JS_ToCString(ctx, argv[0]));
		#endif
		if ( !addr ) {
			return JS_ThrowReferenceError(ctx, "failed to find the symbol '%s'", JS_ToCString(ctx, argv[0]));
		}
		int32_t ret_code = NativeLibraryFunction(addr)(argc < 2 ? JS_UNDEFINED : argv[1]);
		return JS_NewInt32(ctx, ret_code);
	}
	static JSContext* g_sandbox_base_context = nullptr;
	static JSRuntime* g_sandbox_runtime = nullptr;
	static JSValue g_sandbox_object = JS_UNDEFINED;
	static JSValueConst JSRunInSandbox(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
		if (argc < 1 || !JS_IsString(argv[0])) {
			return JS_ThrowReferenceError(ctx, "need script code");
		}
		if (!g_sandbox_runtime) {
			//initialize the sandbox
			g_sandbox_runtime = JS_NewRuntime();
			g_sandbox_base_context = JS_NewContext(g_sandbox_runtime);
			JSValueConst global = JS_GetGlobalObject(g_sandbox_base_context);
			JS_SetPropertyStr(g_sandbox_base_context, global, "__global", global);
			JSValueConst obj_console = JS_NewObject(ama::jsctx);
			JS_SetPropertyStr(g_sandbox_base_context, global, "console", obj_console);
			JS_SetPropertyStr(
				g_sandbox_base_context, obj_console, "log", JS_NewCFunctionMagic(
					g_sandbox_base_context, JSConsoleWrite,
					"log", 0, JS_CFUNC_generic_magic, 1
				)
			);
			JS_SetPropertyStr(
				g_sandbox_base_context, obj_console, "error", JS_NewCFunctionMagic(
					g_sandbox_base_context, JSConsoleWrite,
					"error", 0, JS_CFUNC_generic_magic, 0
				)
			);
			/////
			ama::gcstring fn_initjs = ama::FindCommonJSModuleByPath((path::normalize(JC::string_concat(ama::std_module_dir, path::sep, "_sandbox.js"))));
			if ( fn_initjs.empty() ) {
				fn_initjs = ama::FindCommonJSModuleByPath((path::normalize(JC::string_concat(ama::std_module_dir_global, path::sep, "_sandbox.js"))));
			}
			if ( fn_initjs.empty() ) {
				//we NEED that
				fprintf(stderr, "panic: failed to find ${AMA_MODULES}/_sandbox.js, sandbox could break\n");
			} else {
				JC::StringOrError bootstrap_code = fs::readFileSync(fn_initjs);
				JSValueConst ret = JS_Eval(
					g_sandbox_base_context, bootstrap_code->c_str(),
					bootstrap_code->size(), fn_initjs.c_str(), JS_EVAL_TYPE_GLOBAL
				);
				if ( JS_IsException(ret) ) {
					ama::DumpError(g_sandbox_base_context);
				}
				JS_FreeValue(g_sandbox_base_context, ret);
			}
			g_sandbox_object = JS_GetPropertyStr(g_sandbox_base_context, global, "Sandbox");
		}
		//run the script in a temporary context
		JSContext* sbctx = JS_NewContext(g_sandbox_runtime);
		JS_SetPropertyStr(sbctx, JS_GetGlobalObject(sbctx), "Sandbox", JS_DupValue(sbctx, g_sandbox_object));
		JS_SetPropertyStr(sbctx, JS_GetGlobalObject(sbctx), "console", JS_DupValue(sbctx, g_sandbox_object));
		std::span<char> code = ama::UnwrapStringSpan(argv[0]);
		JSValueConst ret = JS_Eval(
			sbctx, code.c_str(),
			code.size(), "sandboxed code", JS_EVAL_TYPE_GLOBAL
		);
		if (JS_IsFunction(sbctx, ret)) {
			JSValue ret2 = JS_Call(sbctx, ret, JS_GetGlobalObject(sbctx), 0, nullptr);
			JS_FreeValue(sbctx, ret);
			ret = ret2;
		}
		JSValue my_ret = JS_UNDEFINED;
		if ( JS_IsException(ret) ) {
			ama::DumpError(sbctx);
			std::cerr << ("failed to run sandboxed code:") << std::endl;
			std::cerr << (code) << std::endl;
			JS_FreeContext(sbctx);
			return JS_ThrowReferenceError(ctx, "failed to run sandboxed code");
		} else {
			//UnwrapString is tied to our own ctx so don't use it
			size_t len = 0;
			const char *s = JS_ToCStringLen(sbctx, &len, ret);
			my_ret = JS_NewStringLen(ama::jsctx, s, len);
			JS_FreeValue(sbctx, ret);
		}
		JS_FreeContext(sbctx);
		return my_ret;
	}
	void InitScriptEnv() {
		ama::g_runtime_handle = JS_NewRuntime();
		ama::jsctx = JS_NewContext(ama::g_runtime_handle);
		#if defined(_WIN32)
			//fix the Windows terminal
			HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
			DWORD mode = 0;
			GetConsoleMode(hstdout, &mode);
			//4 is ENABLE_VIRTUAL_TERMINAL_PROCESSING
			SetConsoleMode(hstdout, mode | 4);
			//65001 is CP_UTF8
			SetConsoleCP(65001);
			SetConsoleOutputCP(65001);
		#endif
		///////////
		JS_NewClassID(&ama::g_node_classid);
		JSClassDef class_def{};
		class_def.class_name = "Node";
		class_def.finalizer = NodeFinalizer;
		JS_NewClass(ama::g_runtime_handle, ama::g_node_classid, &class_def);
		ama::g_node_proto = JS_NewObject(ama::jsctx);
		///////////
		SetupConfigDirs();
		ama::GeneratedJSBindings();
		JSValueConst global = JS_GetGlobalObject(ama::jsctx);
		JS_SetPropertyStr(ama::jsctx, global, "__global", global);
		JSValue js_node_class_names_array = JS_NewArray(ama::jsctx);
		JS_SetPropertyStr(ama::jsctx, global, "__node_class_names", js_node_class_names_array);
		JSValue js_node_builder_names_array = JS_NewArray(ama::jsctx);
		JS_SetPropertyStr(ama::jsctx, global, "__node_builder_names", js_node_builder_names_array);
		for (int i = 0; i < ama::g_node_class_names.size(); i += 1) {
			JS_SetPropertyStr(ama::jsctx, global, ama::g_node_class_names[i], JS_NewInt32(ama::jsctx, i));
			JS_SetPropertyStr(ama::jsctx, global, ama::g_builder_names[i], JS_NewCFunctionMagic(
				ama::jsctx, JSCreateNode,
				ama::g_builder_names[i], 0, JS_CFUNC_generic_magic, i
			));
			JS_SetPropertyUint32(ama::jsctx, js_node_class_names_array, i, JS_NewString(ama::jsctx, ama::g_node_class_names[i]));
			JS_SetPropertyUint32(ama::jsctx, js_node_builder_names_array, i, JS_NewString(ama::jsctx, ama::g_builder_names[i]));
		}
		#if defined(_WIN32)
			JS_SetPropertyStr(ama::jsctx, global, "__platform", ama::WrapString("win32"));
		#elif defined(__APPLE__)
			JS_SetPropertyStr(ama::jsctx, global, "__platform", ama::WrapString("darwin"));
		#else
			JS_SetPropertyStr(ama::jsctx, global, "__platform", ama::WrapString("linux"));
		#endif
		JS_SetPropertyStr(ama::jsctx, global, "__std_module_dir", ama::WrapString(ama::std_module_dir));
		JS_SetPropertyStr(ama::jsctx, global, "__std_module_dir_global", ama::WrapString(ama::std_module_dir_global));
		JS_SetPropertyStr(
			ama::jsctx, global, "CreateNode", JS_NewCFunction(
				ama::jsctx, JSCreateNodeRaw,
				"CreateNode", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "ParseCode", JS_NewCFunction(
				ama::jsctx, JSParseCode,
				"ParseCode", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__ResolveJSRequire", JS_NewCFunction(
				ama::jsctx, JSResolveJSRequire,
				"ResolveJSRequire", 2
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__require", JS_NewCFunction(
				ama::jsctx, JSRequire,
				"require", 2
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__getenv", JS_NewCFunction(
				ama::jsctx, JSGetEnv,
				"getenv", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__readFileSync", JS_NewCFunction(
				ama::jsctx, JSReadFileSync,
				"readFileSync", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__writeFileSync", JS_NewCFunction(
				ama::jsctx, JSWriteFileSync,
				"writeFileSync", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__existsSync", JS_NewCFunction(
				ama::jsctx, JSExistsSync,
				"existsSync", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__statSync", JS_NewCFunction(
				ama::jsctx, JSStatSync,
				"statSync", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__path_normalize", JS_NewCFunction(
				ama::jsctx, JSPathNormalize,
				"normalize", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__path_toAbsolute", JS_NewCFunction(
				ama::jsctx, JSPathToAbsolute,
				"toAbsolute", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__buffer_toString", JS_NewCFunction(
				ama::jsctx, JSBufferToString,
				"toString", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__path_parse", JS_NewCFunction(
				ama::jsctx, JSPathParse,
				"parse", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__path_isAbsolute", JS_NewCFunction(
				ama::jsctx, JSPathIsAbsolute,
				"isAbsolute", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__path_relative", JS_NewCFunction(
				ama::jsctx, JSPathRelative,
				"relative", 2
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__system", JS_NewCFunction(
				ama::jsctx, JSSystem,
				"__system", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__chdir", JS_NewCFunction(
				ama::jsctx, JSChdir,
				"__chdir", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__cwd", JS_NewCFunction(
				ama::jsctx, JSCwd,
				"__cwd", 0
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__byte_length", JS_NewCFunction(
				ama::jsctx, JSByteLength,
				"__byte_length", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__byte_at", JS_NewCFunction(
				ama::jsctx, JSByteAt,
				"__byte_at", 2
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__byte_substr", JS_NewCFunction(
				ama::jsctx, JSByteSubstr,
				"__byte_substr", 3
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "cons", JS_NewCFunction(
				ama::jsctx, JSCons,
				"cons", 2
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "ProcessAmaFile", JS_NewCFunction(
				ama::jsctx, JSProcessAmaFile,
				"ProcessAmaFile", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, global, "__RunInSandbox", JS_NewCFunction(
				ama::jsctx, JSRunInSandbox,
				"__RunInSandbox", 1
			)
		);
		JS_SetPropertyStr(ama::jsctx, global, "Node", ama::g_node_proto);
		JS_SetPropertyStr(
			ama::jsctx, ama::g_node_proto, "GetPlaceHolder", JS_NewCFunction(
				ama::jsctx, JSGetPlaceHolder,
				"GetPlaceHolder", 0
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, ama::g_node_proto, "gc", JS_NewCFunction(
				ama::jsctx, JSNodeGC,
				"gc", 0
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, ama::g_node_proto, "toSource", JS_NewCFunction(
				ama::jsctx, JSGenerateCode,
				"toSource", 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, ama::g_node_proto, "GetUniqueTag", JS_NewCFunction(
				ama::jsctx, JSGetUniqueTag,
				"GetUniqueTag", 0
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, ama::g_node_proto, "GetNodeFromUniqueTag", JS_NewCFunction(
				ama::jsctx, JSGetNodeFromUniqueTag,
				"GetNodeFromUniqueTag", 1
			)
		);
		for (int i = 0; i < g_filters.size(); i += 1) {
			JS_SetPropertyStr(
				ama::jsctx, ama::g_node_proto, g_filters[i].name, JS_NewCFunctionMagic(
					ama::jsctx, JSApplyNodeFilter,
					g_filters[i].name, g_filters[i].fo ? 2 : 1, JS_CFUNC_generic_magic, i
				)
			);
		}
		g_require_cache = JS_NewObjectProto(ama::jsctx, JS_NULL);
		JS_SetPropertyStr(
			ama::jsctx, global, "__require_cache", g_require_cache
		);
		///////////
		JSValueConst obj_console = JS_NewObject(ama::jsctx);
		JS_SetPropertyStr(ama::jsctx, global, "console", obj_console);
		JS_SetPropertyStr(
			ama::jsctx, obj_console, "log", JS_NewCFunctionMagic(
				ama::jsctx, JSConsoleWrite,
				"log", 0, JS_CFUNC_generic_magic, 1
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, obj_console, "error", JS_NewCFunctionMagic(
				ama::jsctx, JSConsoleWrite,
				"error", 0, JS_CFUNC_generic_magic, 0
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, obj_console, "write", JS_NewCFunctionMagic(
				ama::jsctx, JSConsoleWrite,
				"write", 0, JS_CFUNC_generic_magic, 3
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, obj_console, "writeError", JS_NewCFunctionMagic(
				ama::jsctx, JSConsoleWrite,
				"writeError", 0, JS_CFUNC_generic_magic, 2
			)
		);
		JS_SetPropertyStr(
			ama::jsctx, obj_console, "flush", JS_NewCFunction(
				ama::jsctx, JSConsoleFlush,
				"flush", 0
			)
		);
		/////////////
		//setup the native extension system
		JS_NewClassID(&ama::g_native_library_classid);
		JSClassDef class_def_nl{};
		class_def_nl.class_name = "NativeLibrary";
		class_def_nl.finalizer = NativeLibraryFinalizer;
		JS_NewClass(ama::g_runtime_handle, ama::g_native_library_classid, &class_def_nl);
		g_native_library_proto = JS_NewObject(ama::jsctx);
		JS_SetPropertyStr(ama::jsctx, global, "NativeLibrary", g_native_library_proto);
		JS_SetPropertyStr(ama::jsctx, g_native_library_proto, "run", JS_NewCFunction(ama::jsctx, NativeLibraryRun, "run", 1));
		JS_SetPropertyStr(ama::jsctx, g_native_library_proto, "load", JS_NewCFunction(ama::jsctx, JSLoadNativeLibrary, "load", 1));
		/////////////
		//run the JS part of the initialization
		ama::gcstring fn_initjs = ama::FindCommonJSModuleByPath((path::normalize(JC::string_concat(ama::std_module_dir, path::sep, "_init.js"))));
		if ( fn_initjs.empty() ) {
			fn_initjs = ama::FindCommonJSModuleByPath((path::normalize(JC::string_concat(ama::std_module_dir_global, path::sep, "_init.js"))));
		}
		if ( fn_initjs.empty() ) {
			//we NEED that
			fprintf(stderr, "panic: failed to find ${AMA_MODULES}/_init.js, things could break\n");
		} else {
			JC::StringOrError bootstrap_code = fs::readFileSync(fn_initjs);
			JSValueConst ret = JS_Eval(
				ama::jsctx, bootstrap_code->c_str(),
				bootstrap_code->size(), fn_initjs.c_str(), JS_EVAL_TYPE_GLOBAL
			);
			if ( JS_IsException(ret) ) {
				ama::DumpError(ama::jsctx);
			}
		}
	}
	void LazyInitScriptEnv() {
		if ( ama::g_runtime_handle ) { return; }
		InitScriptEnv();
	}
	ama::Node* Unparse();
};
