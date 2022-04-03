#ifndef _NODEGC_JCH_HPP
#define _NODEGC_JCH_HPP
/*#pragma add("jc_files", "./nodegc.jc");*/
namespace ama {
	intptr_t gc();
	//warning: all Node* will become dangling pointers after DropAllMemoryPools()
	void DropAllMemoryPools();
};

#endif
