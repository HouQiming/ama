#ifndef _MEMPOOL_JCH_HPP
#define _MEMPOOL_JCH_HPP
#include <stdint.h>
/*#pragma add("jc_files", "./mempool.jc");*/
namespace ama {
	struct TBlockHeader {
		TBlockHeader* next{};
		intptr_t size{};
	};
	//a zeroed TMemoryPool is an empty pool
	struct TMemoryPool {
		TBlockHeader* block{};
		intptr_t block_size{};
		char* front{};
		intptr_t sz_free{};
	};
	void* poolAlloc(ama::TMemoryPool* ppool, intptr_t sz, intptr_t BLOCK_SIZE);
	void* poolAllocAligned(ama::TMemoryPool* ppool, intptr_t sz, intptr_t align, intptr_t BLOCK_SIZE);
	void poolRelease(ama::TMemoryPool* ppool);
};

#endif
