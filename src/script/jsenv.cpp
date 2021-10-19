#include <stdio.h>
#include "fs.hpp"
#include "path.hpp"
#include "../../modules/cpp/json/json.h"
#include "console.hpp"
#include "jsenv.hpp"
#include "../util/jc_array.h"
#include <string>
#include <vector>
#include "../util/jc_unique_string.h"
#include <memory>
#include <unordered_map>
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
	struct PackageJSON {
		JC::unique_string main{};
	};
	static JC::unique_string CommonJSLoadAsFile(JC::unique_string fn) {
		if ( fs::existsSync(fn) ) {
			return fn;
		}
		if ( fs::existsSync(JC::string_concat(fn, ".js")) ) {
			return JC::array_cast<JC::unique_string>(JC::string_concat(fn, ".js"));
		}
		if ( fs::existsSync(JC::string_concat(fn, ".json")) ) {
			return JC::array_cast<JC::unique_string>(JC::string_concat(fn, ".json"));
		}
		//put native options last to minimize the hijacking risk
		JC::unique_string fn_native{};
		#if defined(_WIN32)
			fn_native = JC::array_cast<JC::unique_string>(JC::string_concat(fn, ".dll"));
		#elif defined(__APPLE__)
			fn_native = JC::array_cast<JC::unique_string>(JC::string_concat(path::dirname(fn), "/lib", path::basename(fn), ".dylib"));
		#else
			fn_native = JC::array_cast<JC::unique_string>(JC::string_concat(path::dirname(fn), "/lib", path::basename(fn), ".so"));
		#endif
		//console.log(fn_native);
		if ( fs::existsSync(fn_native) ) {
			return fn_native;
		}
		return nullptr;
	}
	JC::unique_string FindCommonJSModuleByPath(JC::unique_string fn) {
		if ( fs::DirExists(fn) ) {
			if ( fs::existsSync(JC::string_concat(fn, ".js")) ) {
				return JC::array_cast<JC::unique_string>(JC::string_concat(fn, ".js"));
			}
			if ( fs::existsSync(JC::string_concat(fn, ".json")) ) {
				return JC::array_cast<JC::unique_string>(JC::string_concat(fn, ".json"));
			}
			JC::unique_string fn_index = fn;
			std::shared_ptr<std::string> s_package_json = fs::readFileSync(path::normalize(JC::string_concat(fn, path::sep, "package.json")));
			if ( s_package_json != nullptr ) {
				PackageJSON package_json = JSON::parse<PackageJSON>(s_package_json);
				if ( package_json.main != nullptr ) {
					JC::unique_string fn_main = JC::array_cast<JC::unique_string>(path::normalize(JC::string_concat(fn, path::sep, package_json.main)));
					JC::unique_string ret = CommonJSLoadAsFile(fn_main);
					if ( ret != nullptr ) {
						return ret;
					}
					fn_index = fn_main;
				}
			}
			if ( fs::existsSync(JC::string_concat(fn_index, "/index.js")) ) {
				return JC::array_cast<JC::unique_string>(JC::string_concat(fn_index, "/index.js"));
			}
			if ( fs::existsSync(JC::string_concat(fn_index, "/index.json")) ) {
				return JC::array_cast<JC::unique_string>(JC::string_concat(fn_index, "/index.json"));
			}
			return nullptr;
		}
		return CommonJSLoadAsFile(fn);
	}
	JC::unique_string FindCommonJSModule(JC::unique_string fn_required, JC::unique_string dir_base) {
		JC::unique_string ret{};
		JC::unique_string dir = dir_base;
		for (; ;) {
			if ( path::basename(dir) != "node_modules" ) {
				ret = FindCommonJSModuleByPath(JC::array_cast<JC::unique_string>(path::normalize(JC::string_concat(dir, path::sep, "node_modules", path::sep, fn_required))));
				if ( ret != nullptr ) {
					return ret;
				}
			}
			std::string dir_upper = path::dirname(dir);
			if ( dir == dir_upper ) {
				break;
			}
			dir = JC::array_cast<JC::unique_string>(dir_upper);
		}
		return nullptr;
	}
	std::unordered_map<JC::unique_string, int> GetPrioritizedList(JSValueConst options, char const* name) {
		JC::unique_string s_binops = ama::UnwrapString(JS_GetPropertyStr(ama::jsctx, options, name));
		std::unordered_map<JC::unique_string, int> ret{};
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
					ret--->set(JC::array_cast<JC::unique_string>(s_binop--->subarray(0, s_binop.size() - 1)), priority);
					priority += 1;
				} else {
					ret--->set(JC::array_cast<JC::unique_string>(s_binop), priority);
				}
			}
		}
		return std::move(ret);
	}
	JSValue InheritOptions(JSValueConst options) {
		return JS_Invoke(ama::jsctx, JS_GetGlobalObject(ama::jsctx), JS_NewAtom(ama::jsctx, "__InheritOptions"), 1, &options);
	}
};
namespace JSON {
	template<>
	struct ParseFromImpl<ama::PackageJSON> {
		typedef void type;
		template<typename T = ama::PackageJSON>
		static ama::PackageJSON parseFrom(JSONParserContext& ctx, ama::PackageJSON**) {
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
				switch ( *ctx.begin ) {
					case '\"': default: {
						ctx.begin += 1;
						if ( ctx.TrySkipName("main\"") ) {
							ctx.SkipColon();
							if ( ctx.error ) {
								return std::move(ret);
							}
							ret.main = JSON::parseFrom(ctx, (decltype(ret.main)**)(nullptr));
							if ( ctx.error ) {
								return std::move(ret);
							}
						} else {
							ctx.SkipStringBody();
							ctx.SkipField();
						}
						break;
					}
				}
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
};
