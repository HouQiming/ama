#include <string>
#include <vector>
#include <unordered_map>
#include <stdio.h>
#include "../util/jc_array.h"
#include "../util/fs.hpp"
#include "../util/path.hpp"
#include "../../modules/cpp/json/json.h"
#include "jsenv.hpp"
struct PackageJSON {
	std::string main{};
};
namespace ama {
	JSContext* jsctx{};
	JSRuntime* g_runtime_handle{};
	uint32_t g_node_classid = 0u;
	JSValue g_node_proto = JS_NULL;
	std::string std_module_dir{};
	std::string std_module_dir_global{};
	std::vector<char const*> g_builder_names{};
	std::vector<char const*> g_node_class_names{};
	std::unordered_map<ama::Node const*, JSValue> g_js_node_map{};
	JSValueConst WrapNode(ama::Node const* nd) {
		if ( !nd ) {
			return JS_NULL;
		}
		JSValue ret = g_js_node_map[nd];
		if ( !JS_IsObject(ret) ) {
			ret = JS_NewObjectProtoClass(jsctx, g_node_proto, g_node_classid);
			JS_SetOpaque(ret, (void*)(nd));
			JS_PreventExtensions(jsctx, ret);
			g_js_node_map--->set(nd, ret);
			return ret;
		} else {
			return JS_DupValue(jsctx, ret);
		}
	}
	void DumpError(JSContext* ctx) {
		JSValueConst ret = JS_GetException(ctx);
		fprintf(stderr, "%s\n", JS_ToCString(ctx, ret));
		JSValueConst err_stack = JS_GetPropertyStr(ctx, ret, "stack");
		fprintf(stderr, "%s", JS_ToCString(ctx, err_stack));
		fflush(stderr);
		JS_FreeValue(ctx, err_stack);
	}
	static std::string CommonJSLoadAsFile(std::span<char> fn) {
		if ( fs::existsSync(fn) ) {
			return JC::array_cast<std::string>(fn);
		}
		if ( fs::existsSync(JC::string_concat(fn, ".ama.js")) ) {
			return (JC::string_concat(fn, ".ama.js"));
		}
		if ( fs::existsSync(JC::string_concat(fn, ".js")) ) {
			return (JC::string_concat(fn, ".js"));
		}
		if ( fs::existsSync(JC::string_concat(fn, ".json")) ) {
			return (JC::string_concat(fn, ".json"));
		}
		//put native options last to minimize the hijacking risk
		std::string fn_native{};
		#if defined(_WIN32)
			fn_native = (JC::string_concat(fn, ".dll"));
		#elif defined(__APPLE__)
			fn_native = (JC::string_concat(path::dirname(fn), "/lib", path::basename(fn), ".dylib"));
		#else
			fn_native = (JC::string_concat(path::dirname(fn), "/lib", path::basename(fn), ".so"));
		#endif
		//console.log(fn_native);
		if ( fs::existsSync(fn_native) ) {
			return fn_native;
		}
		return "";
	}
	std::string FindCommonJSModuleByPath(std::span<char> fn) {
		if ( fs::DirExists(fn) ) {
			if ( fs::existsSync(JC::string_concat(fn, ".js")) ) {
				return (JC::string_concat(fn, ".js"));
			}
			if ( fs::existsSync(JC::string_concat(fn, ".json")) ) {
				return (JC::string_concat(fn, ".json"));
			}
			std::string fn_index = JC::array_cast<std::string>(fn);
			JC::StringOrError s_package_json = fs::readFileSync(path::normalize(JC::string_concat(fn, path::sep, "package.json")));
			if ( !!s_package_json ) {
				PackageJSON package_json = JSON::parse<PackageJSON>(s_package_json.some);
				if ( package_json.main.size() ) {
					std::string fn_main = (path::normalize(JC::string_concat(fn, path::sep, package_json.main)));
					std::string ret = CommonJSLoadAsFile(fn_main);
					if ( !ret.empty() ) {
						return ret;
					}
					fn_index = fn_main;
				}
			}
			if ( fs::existsSync(JC::string_concat(fn_index, "/index.js")) ) {
				return (JC::string_concat(fn_index, "/index.js"));
			}
			if ( fs::existsSync(JC::string_concat(fn_index, "/index.json")) ) {
				return (JC::string_concat(fn_index, "/index.json"));
			}
			return "";
		}
		return CommonJSLoadAsFile(fn);
	}
	std::string FindCommonJSModule(std::span<char> fn_required, std::span<char> dir_base) {
		std::string ret{};
		std::string dir = JC::array_cast<std::string>(dir_base);
		for (; ;) {
			if ( path::basename(dir) != "node_modules" ) {
				ret = FindCommonJSModuleByPath(std::string(path::normalize(JC::string_concat(dir, path::sep, "node_modules", path::sep, fn_required))));
				if ( !ret.empty() ) {
					return ret;
				}
			}
			std::string dir_upper = path::dirname(dir);
			if ( dir == dir_upper ) {
				break;
			}
			dir = std::string(dir_upper);
		}
		return "";
	}
	std::unordered_map<ama::gcstring, int> GetPrioritizedList(JSValueConst options, char const* name) {
		std::span<char> s_binops = ama::UnwrapStringSpan(JS_GetPropertyStr(ama::jsctx, options, name));
		std::unordered_map<ama::gcstring, int> ret{};
		int priority = 1;
		//coulddo: SSE / NEON vectorization
		size_t I0 = intptr_t(0L);
		for (size_t I = 0; I <= s_binops.size(); ++I) {
			//char ch=(I<str.size()?str[I]:0)
			if ( I >= s_binops.size() || s_binops[I] == ' ' ) {
				//this scope will be if-wrapped
				size_t I00 = I0;
				I0 = I + 1;
				//the return will be replaced
				std::span<char> s_binop(s_binops.data() + I00, I - I00);
				if ( s_binop--->endsWith('\n') ) {
					ret--->set(ama::gcstring(s_binop--->subarray(0, s_binop.size() - 1)), priority);
					priority += 1;
				} else {
					ret--->set(ama::gcstring(s_binop), priority);
				}
			}
		}
		return std::move(ret);
	}
	JSValue CallJSMethod(JSValue this_val, char const* name, std::span<JSValue> args) {
		JSAtom atom = JS_NewAtom(ama::jsctx, name);
		JSValue ret = JS_Invoke(jsctx, this_val, atom, int(args.size()), args.data());
		JS_FreeAtom(ama::jsctx, atom);
		for (JSValue & val: args) {
			JS_FreeValue(ama::jsctx, val);
			val = JS_UNDEFINED;
		}
		return ret;
	}
	JSValue CallJSMethodFree(JSValue this_val, char const* name, std::span<JSValue> args) {
		JSValue ret = CallJSMethod(this_val, name, args);
		JS_FreeValue(ama::jsctx, this_val);
		return ret;
	}
	JSValue RequireJSModule(char const* name) {
		return CallJSMethod(JS_GetGlobalObject(jsctx), "__require", std::vector<JSValue>{
			JS_NewString(jsctx, "./."),
			JS_NewString(jsctx, name)
		});
	}
};
#pragma gen_begin(JSON::parse<PackageJSON>)
namespace JSON {
	template <>
	struct ParseFromImpl<PackageJSON> {
		//`type` is only used for SFINAE
		typedef void type;
		template <typename T = PackageJSON>
		static PackageJSON parseFrom(JSONParserContext& ctx, PackageJSON**) {
			T ret{};
			if ( ctx.begin == ctx.end ) {
				return std::move(ret);
			}
			ctx.SkipSpace();
			if ( *ctx.begin != '{' ) {
				ctx.error = "'{' expected";
				ctx.error_location = ctx.begin;
				return std::move(ret);
			}
			ctx.begin += 1;
			ctx.SkipSpace();
			while ( ctx.begin != ctx.end && ctx.begin[0] != '}' ) {
				ctx.SkipSpace();
				if ( !ctx.TrySkipName("\"main\"") ) {
					goto skip;
				} else {
					{
						ctx.SkipColon();
						if ( ctx.error ) {
							return std::move(ret);
						}
						ret.main = JSON::parseFrom(ctx, (std::string**)(NULL));
						if ( ctx.error ) {
							return std::move(ret);
						}
						goto done;
					}
				}
				skip:
				ctx.SkipStringBody();
				ctx.SkipField();
				done:
				if ( ctx.error ) {
					return std::move(ret);
				}
				ctx.SkipSpace();
				if ( ctx.begin != ctx.end && (*ctx.begin == ',' || *ctx.begin == '}') ) {
					if ( *ctx.begin == ',' ) {
						ctx.begin += 1;
					}
				} else {
					ctx.error = "',' or '}' expected";
					ctx.error_location = ctx.begin;
					return std::move(ret);
				}
			}
			if ( ctx.begin != ctx.end ) {
				ctx.begin += 1;
			}
			return std::move(ret);
		}
	};
}
#pragma gen_end(JSON::parse<PackageJSON>)
