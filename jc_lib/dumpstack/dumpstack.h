#ifndef __DUMPSTACK_H
#define __DUMPSTACK_H
namespace DumpStack {
	void EnableDump();
	void PrintCallStack();
	volatile extern int g_dump_all_threads;
}
#endif
