#ifndef _JSENV_JCH_HPP
#define _JSENV_JCH_HPP
#include "./quickjs/src/quickjs.h"
#include "../ast/node.hpp"
#include <string>
#include <vector>
#include "../util/jc_array.h"
#include "../util/gcstring.h"
#include <unordered_map>
/*#pragma add("jc_files", "./jsenv.jc");*/
/*#pragma add("h_files", "./quickjs/src/quickjs.h");*/
/*#pragma add("h_files", "./quickjs/src/cutils.h");*/
/*#pragma add("h_files", "./quickjs/src/list.h");*/
/*#pragma add("h_files", "./quickjs/src/quickjs.h");*/
/*#pragma add("h_files", "./quickjs/src/libregexp.h");*/
/*#pragma add("h_files", "./quickjs/src/libregexp-opcode.h");*/
/*#pragma add("h_files", "./quickjs/src/libunicode.h");*/
/*#pragma add("h_files", "./quickjs/src/libunicode-table.h");*/
/*#pragma add("h_files", "./quickjs/src/quickjs-opcode.h");*/
/*#pragma add("h_files", "./quickjs/src/quickjs-atom.h");*/
/*#pragma add("c_files", "./quickjs/src/quickjs.c");*/
/*#pragma add("c_files", "./quickjs/src/libregexp.c");*/
/*#pragma add("c_files", "./quickjs/src/libunicode.c");*/
/*#pragma add("c_files", "./quickjs/src/cutils.c");*/
namespace ama {
	extern thread_local JSContext* jsctx;
	extern thread_local JSRuntime* g_runtime_handle;
	extern thread_local std::string std_module_dir;
	JSContext* GetGlobalJSContext();
	void DumpError(JSContext* ctx);
	static inline ama::gcstring UnwrapString(JSValueConst val) {
		size_t len = int64_t(0uLL);
		char const* ptr = JS_ToCStringLen(ama::jsctx, &len, val);
		auto ret = ama::gcstring(ptr, intptr_t(len));
		JS_FreeCString(ama::jsctx, ptr);
		return std::move(ret);
	}
	static inline std::string UnwrapStringResizable(JSValueConst val) {
		size_t len = int64_t(0uLL);
		char const* ptr = JS_ToCStringLen(ama::jsctx, &len, val);
		auto ret = std::string(ptr, intptr_t(len));
		JS_FreeCString(ama::jsctx, ptr);
		return std::move(ret);
	}
	//static inline char[:] UnwrapStringSpan(JSValueConst val) {
	//	size_t len = i64(0uLL);
	//	char const* ptr = JS_ToCStringLen(ama::jsctx, &len, val);
	//	auto ret=char[:](ptr, iptr(len));
	//	//JS_FreeCString(ama::jsctx,ptr);
	//	return ret;
	//}
	static inline JSValueConst WrapString(std::span<char> s) {
		return JS_NewStringLen(ama::jsctx, s.data(), s.size());
	}
	static inline int32_t UnwrapInt32(JSValueConst val, int32_t dflt) {
		int32_t ret = 0;
		if ( JS_IsUndefined(val) || JS_ToInt32(ama::jsctx, &ret, val) < 0 ) {
			ret = dflt;
		}
		return ret;
	}
	extern thread_local uint32_t g_node_classid;
	extern thread_local JSValue g_node_proto;
	extern thread_local JSValue g_js_global;
	extern thread_local std::unordered_map<ama::Node const*, JSValue> g_js_node_map;
	static inline ama::Node* UnwrapNode(JSValueConst val) {
		ama::Node* nd = (ama::Node*)(JS_GetOpaque(val, g_node_classid));
		if (nd) {assert(nd->tmp_flags & ama::TMPF_IS_NODE);}
		return nd;
	}
	JSValueConst WrapNode(ama::Node const* nd);
	static inline JSValueConst WrapNodeArray(std::span<ama::Node*> nds) {
		JSValueConst ret = JS_NewArray(jsctx);
		for (intptr_t i = intptr_t(0L); i < intptr_t(nds.size()); i += 1) {
			JS_SetPropertyUint32(jsctx, ret, uint32_t(uintptr_t(i)), ama::WrapNode(nds[i]));
		}
		return ret;
	}
	static inline std::vector<ama::Node*> UnwrapNodeArray(JSValueConst val) {
		int64_t lg = int64_t(0LL);
		JS_ToInt64(jsctx, &lg, JS_GetPropertyStr(jsctx, val, "length"));
		std::vector<ama::Node*> ret((intptr_t(lg)));
		for (intptr_t i = intptr_t(0L); i < intptr_t(lg); i += 1) {
			ret[i] = UnwrapNode(JS_GetPropertyUint32(jsctx, val, uint32_t(uintptr_t(i))));
		}
		return std::move(ret);
	}
	std::string FindCommonJSModuleByPath(std::span<char> fn);
	std::string FindCommonJSModule(std::span<char> fn_required, std::span<char> dir_base);
	extern thread_local std::vector<char const*> g_builder_names;
	extern thread_local std::vector<char const*> g_node_class_names;
	extern thread_local std::vector<char const*> g_builtin_modules;
	std::unordered_map<ama::gcstring, int> GetPrioritizedList(JSValueConst options, char const* name);
	extern thread_local std::string std_module_dir_global;
	JSValue CallJSMethod(JSValue this_val, char const* name, std::span<JSValue> args);
	JSValue CallJSMethodFree(JSValue this_val, char const* name, std::span<JSValue> args);
	JSValue RequireJSModule(char const* name);
};

#endif
