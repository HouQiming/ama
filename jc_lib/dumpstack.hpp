#ifndef _DUMPSTACK_JCH_HPP
#define _DUMPSTACK_JCH_HPP
#include "jc_platform.h"
#if JC_OS == JC_OS_WINDOWS
	/*#pragma add("ldflags", "DbgHelp.lib");*/
#else
	/*#pragma add("ldflags", "-ldl");*/
	/*#pragma add("cflags", "-funwind-tables");*/
#endif
#include "./dumpstack/dumpstack.h"
/*#pragma add("h_files", "./dumpstack/dumpstack.h");*/
/*#pragma add("c_files", "./dumpstack/linux_bt.cpp");*/
/*#pragma add("c_files", "./dumpstack/win_dbghelp.cpp");*/
/*#pragma add("c_files", "./dumpstack/enable_dump.cpp");*/

#endif
