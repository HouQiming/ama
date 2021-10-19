#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "../util/dumpstack/dumpstack.h"
#include "../script/jsapi.hpp"
#include "../util/jc_array.h"
#pragma no_auto_header()
/*#pragma add("ldflags", "amal");*/
int main(int argc, char const* const* argv) {
	if ( argc < 2 ) {
		//no macros, no output file name
		//whatever you need, put in the script
		printf("usage: ama [-s script] [files]\n");
		return 1;
	}
	std::string extra_script{};
	int has_file = 0;
	for (int i = 1; i < argc; i += 1) {
		if ( strcmp(argv[i], "-s") == 0 && (i + 1) < argc ) {
			extra_script--->push(argv[i + 1], ";\n");
			i += 1;
			continue;
		}
		int err_code = ama::ProcessAmaFile(argv[i], extra_script);
		if ( err_code == ama::PROCESS_AMA_NOT_FOUND ) {
			fprintf(stderr, "unable to load %s\n", argv[i]);
		}
		if ( err_code <= 0 ) {
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
