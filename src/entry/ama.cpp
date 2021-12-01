#include <stdlib.h>
#include <stdio.h>
#if defined(_WIN32)
	#include <windows.h>
#endif
#include <string>
#include "../util/dumpstack/dumpstack.h"
#include "../script/jsapi.hpp"
#include "../util/jc_array.h"
#include "../../modules/cpp/json/json.h"
#pragma no_auto_header()
/*#pragma add("ldflags", "amal");*/
static int RunCommandLineUtility(int argc, char const* const* argv) {
	ama::LazyInitScriptEnv();
	JSValue obj_process = JS_GetPropertyStr(ama::jsctx, JS_GetGlobalObject(ama::jsctx), "process");
	JSValue obj_argv = JS_GetPropertyStr(ama::jsctx, obj_process, "argv");
	JS_FreeValue(ama::jsctx, obj_process);
	for (int ai = 0; ai < argc; ai += 1) {
		JS_SetPropertyUint32(ama::jsctx, obj_argv, ai, JS_NewString(ama::jsctx, argv[ai]));
	}
	JS_FreeValue(ama::jsctx, obj_argv);
	//JSValue arg = JS_NewString(ama::jsctx, argv[i] + 2);
	JSAtom atom = JS_NewAtom(ama::jsctx, "RunCommandLineUtility");
	JSValue ret = JS_Invoke(ama::jsctx, JS_GetGlobalObject(ama::jsctx), atom, 0, nullptr);
	JS_FreeAtom(ama::jsctx, atom);
	int ret_code = 1;
	if (JS_IsException(ret)) {
		ama::DumpError(ama::jsctx);
		ret_code = 1;
	} else {
		ret_code = ama::UnwrapInt32(ret, 0);
		JS_FreeValue(ama::jsctx, ret);
	}
	return ret_code;
}

int main(int argc, char const* const* argv) {
	if ( argc < 2 ) {
		//no macros, no output file name
		//whatever you need, put in the script
		char const* help_argv[2] = {argv[0],"--help"};
		return RunCommandLineUtility(2, help_argv);
	}
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
	std::string extra_script{};
	int has_file = 0;
	int has_filters = 0;
	int need_dump = 0;
	for (int i = 1; i < argc; i += 1) {
		if ( strcmp(argv[i], "-s") == 0 && (i + 1) < argc ) {
			extra_script--->push(argv[i + 1], ";\n");
			i += 1;
			continue;
		}
		if ( strcmp(argv[i], "-f") == 0 && (i + 1) < argc ) {
			if (strcmp(argv[i + 1], "cmake.AutoCreate") != 0) {
				need_dump = 1;
			}
			extra_script--->push(
				"__pipeline.unshift(", JSON::stringify(JC::string_concat(argv[i + 1], ".setup?")), ");\n",
				"__pipeline.push(", JSON::stringify(argv[i + 1]), ");\n"
			);
			i += 1;
			continue;
		}
		if ( strcmp(argv[i], "-o") == 0 && (i + 1) < argc ) {
			need_dump = 0;
			extra_script--->push("__pipeline.push({full_path: require('path').resolve(", JSON::stringify(argv[i + 1]), ")},'Save');\n");
			i += 1;
			continue;
		}
		if (memcmp(argv[i], "--", 2) == 0) {
			return RunCommandLineUtility(argc, argv);
		}
		size_t n0 = extra_script.size();
		if (need_dump) {
			extra_script--->push("__pipeline.push(\"Print\");ParseCurrentFile();\n");
		}
		int err_code = ama::ProcessAmaFile(argv[i], extra_script);
		if (need_dump) {extra_script.resize(n0);}
		if ( err_code == ama::PROCESS_AMA_NOT_FOUND ) {
			fprintf(stderr, "unable to load %s\n", argv[i]);
		}
		if ( err_code == ama::PROCESS_AMA_EMPTY_SCRIPT ) {
			fprintf(stderr, "no script found in %s\n", argv[i]);
		} else if ( err_code <= 0 ) {
			return 1;
		}
		has_file = 1;
	}
	if ( !has_file ) {
		if ( extra_script.size() ) {
			if ( !ama::RunScriptOnFile(extra_script, "<command line>", extra_script.c_str()) ) {
				return 1;
			}
		} else {
			printf("usage: ama [-s script] <files>\n");
			return 1;
		}
	}
	return 0;
}
