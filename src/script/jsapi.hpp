#ifndef _JSAPI_JCH_HPP
#define _JSAPI_JCH_HPP
#include "../ast/node.hpp"
#include "jsenv.hpp"
#include "../util/jc_array.h"
/*#pragma add("jc_files", "./jsapi.jc");*/
namespace ama {
	int RunScriptOnFile(std::span<char> script, char const* file_name, char const* file_data);
	void InitScriptEnv();
	ama::Node* DefaultParseCode(char const* code);
	void LazyInitScriptEnv();
	void DumpASTAsJSON(ama::Node* nd);
	static const int PROCESS_AMA_EMPTY_SCRIPT = -2;
	static const int PROCESS_AMA_NOT_FOUND = -1;
	static const int PROCESS_AMA_SCRIPT_FAILED = 0;
	static const int PROCESS_AMA_SUCCESS = 1;
	int ProcessAmaFile(char const* fn, std::span<char> extra_script);
	ama::Node* LoadFile(char const* fn);
	ama::Node* ComputeType(ama::Node* nd);
	JSValue DeepMatch(ama::Node* nd, ama::Node* nd_pattern);
	void DropTypeCache();
	void DropDependsCache();
	JSValue MatchAll(ama::Node* nd, ama::Node* nd_pattern);
};

#endif
