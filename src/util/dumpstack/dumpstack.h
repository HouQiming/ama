#ifndef __DUMPSTACK_H
#define __DUMPSTACK_H
#pragma add("c_files","enable_dump.cpp")
#pragma add("c_files","linux_bt.cpp")
#pragma add("c_files","win_dbghelp.cpp")
namespace DumpStack {
	void EnableDump();
	void PrintCallStack();
	volatile extern int g_dump_all_threads;
}
#endif
