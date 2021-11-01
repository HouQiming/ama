#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "mempool.hpp"
namespace ama {
	void* poolAlloc(ama::TMemoryPool* ppool, intptr_t sz, intptr_t BLOCK_SIZE) {
		intptr_t align = sz & -sz;
		if ( align > 16 ) { align = 16; } else if ( align < 1 ) { align = 1; }
		return poolAllocAligned(ppool, sz, align, BLOCK_SIZE);
	}
	void* poolAllocAligned(ama::TMemoryPool* ppool, intptr_t sz, intptr_t align, intptr_t BLOCK_SIZE) {
		intptr_t align_offset = -intptr_t(ppool->front) & (align - 1);
		if ( ppool->sz_free < (align_offset + sz) ) {
			if ( !ppool->block_size ) {
				ppool->block_size = BLOCK_SIZE;
			}
			intptr_t sz_alloc = (ppool->block_size > (sz + align) ? ppool->block_size : sz + align) + sizeof(ama::TBlockHeader);
			ama::TBlockHeader* new_block = (ama::TBlockHeader*)(calloc(1, sz_alloc));
			new_block->next = ppool->block;
			new_block->size = sz_alloc - sizeof(ama::TBlockHeader);
			ppool->front = (char*)(new_block) + sizeof(ama::TBlockHeader);
			ppool->sz_free = sz_alloc - sizeof(ama::TBlockHeader);
			ppool->block = new_block;
			align_offset = -intptr_t(ppool->front) & (align - 1);
		}
		char* ret = ppool->front + align_offset;
		ppool->front = ret + sz;
		ppool->sz_free -= align_offset + sz;
		return (void*)(ret);
	}
	void poolRelease(ama::TMemoryPool* ppool) {
		for (ama::TBlockHeader* i = ppool->block; i;) {
			ama::TBlockHeader* next = i->next;
			free(i);
			i = next;
		}
		memset(ppool, 0, sizeof(ama::TMemoryPool));
	}
};
