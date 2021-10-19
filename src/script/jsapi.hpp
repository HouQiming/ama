#ifndef _JSAPI_JCH_HPP
#define _JSAPI_JCH_HPP
#include "../ast/node.hpp"
#include "jsenv.hpp"
#include "../util/jc_array.h"
/*#pragma add("jc_files", "./jsapi.jc");*/
namespace ama {
	int RunScriptOnFile(std::span<char> script, char const* file_name, char const* file_data);
	void InitScriptEnv();
	ama::Node* ParseCode(char const* code, JSValue options);
	void LazyInitScriptEnv();
	void DumpASTAsJSON(ama::Node* nd);
	static const int PROCESS_AMA_NOT_FOUND = -1;
	static const int PROCESS_AMA_SCRIPT_FAILED = 0;
	static const int PROCESS_AMA_SUCCESS = 1;
	int ProcessAmaFile(char const* fn, std::span<char> extra_script);
};

#endif
