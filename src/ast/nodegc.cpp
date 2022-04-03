#include <vector>
#include "node.hpp"
#include "../script/jsenv.hpp"
#include "../util/jc_array.h"
#include "../util/mempool.hpp"
namespace ama {
	extern thread_local ama::TMemoryPool g_node_pool;
	static inline void mark(std::vector<ama::Node*>& Q, ama::Node* nd) {
		if ( nd && !(nd->tmp_flags & ama::TMPF_GC_MARKED) ) {
			assert(nd->tmp_flags & ama::TMPF_IS_NODE);
			nd->tmp_flags |= ama::TMPF_GC_MARKED;
			Q--->push(nd);
			nd->data.gcmark();
			nd->comments_before.gcmark();
			nd->comments_after.gcmark();
		}
	}
	intptr_t gc() {
		#ifndef NDEBUG
		do {
			//check that we don't have premature TMPF_GC_MARKED
			std::vector<ama::Node*> node_ranges = ama::GetAllPossibleNodeRanges();
			for (int i = 0; i < node_ranges.size(); i += 2) {
				for (ama::Node* nd = node_ranges[i]; nd != node_ranges[i + 1]; nd += 1) {
					assert(!(nd->tmp_flags & ama::TMPF_GC_MARKED));
				}
			}
		} while (0);
		#endif
		//mark: if nd is alive, so is nd.p
		//so we only mark root nodes and only propagate along c / s
		std::vector<ama::Node*> Q{};
		for ( auto && pair_nd_v: ama::g_js_node_map ) {
			//we could have hacky ->p which screws up rooting
			ama::Node const* nd = pair_nd_v.first;
			mark(Q, (ama::Node*)nd);
			mark(Q, nd->Root());
			/////////
			//if (!nd->p && nd->node_class == ama::N_FILE) {
			//	fprintf(stderr, "marked root %s\n", nd->data.data());
			//} else {
			//	ama::Node* nd_root = nd->Root();
			//	if (nd_root->node_class == ama::N_FILE) {
			//		fprintf(stderr, "marked child of %s\n", nd_root->data.data());
			//	} else {
			//		fprintf(stderr, "marked middle-node %d %s\n", int(nd->node_class), nd->data.data());
			//	}
			//}
		}
		//ama::mark(Q, ama::g_placeholder);
		//recursion
		for (size_t i = intptr_t(0L); i < Q.size(); i += intptr_t(1L)) {
			ama::Node* nd = Q[i];
			ama::mark(Q, nd->c);
			ama::mark(Q, nd->s);
		}
		//sweep
		intptr_t n_freed = intptr_t(0L);
		intptr_t n_kept = intptr_t(0L);
		intptr_t n_swept = intptr_t(0L);
		//reorganize the free list
		//just drop the placeholder
		ama::g_placeholder = nullptr;
		ama::g_free_nodes = nullptr;
		std::vector<ama::TBlockHeader*> blocks_kept{};
		for (ama::TBlockHeader* block = g_node_pool.block; block;) {
			ama::Node* nd_begin = (ama::Node*)(uintptr_t(block) + sizeof(ama::TBlockHeader));
			ama::Node* nd_end = nullptr;
			if (block == g_node_pool.block) {
				nd_end = ((ama::Node*)g_node_pool.front);
			} else {
				nd_end = ((ama::Node*)(uintptr_t(block) + sizeof(ama::TBlockHeader) + (block->size - sizeof(ama::TBlockHeader)) / sizeof(ama::Node) * sizeof(ama::Node)));
			}
			ama::Node* nd_free_node_before = ama::g_free_nodes;
			intptr_t n_kept_before = n_kept;
			for (ama::Node* nd = nd_begin; nd != nd_end; nd += 1) {
				n_swept += 1;
				if ( (nd->tmp_flags & (ama::TMPF_GC_MARKED | ama::TMPF_IS_NODE)) == ama::TMPF_IS_NODE ) {
					//unreachable but un-free, release
					//console.log('freed node', nd.node_class, nd.data == NULL ? "NULL" : nd.data.c_str());
					//fprintf(stderr,"free node %p\n",nd);
					n_freed += 1;
					//nd->data = ama::gcstring();
					//nd->comments_before = ama::gcstring();
					//nd->comments_after = ama::gcstring();
					////////////
					memset((void*)(nd), 0, sizeof(ama::Node));
					////////////
					nd->tmp_flags = 0;
					nd->s = ama::g_free_nodes;
					ama::g_free_nodes = nd;
				} else if ( nd->tmp_flags & ama::TMPF_IS_NODE ) {
					//clear the marked flag for the next gc
					//fprintf(stderr,"keep node %p\n",nd);
					n_kept += 1;
					nd->tmp_flags &= ~ama::TMPF_GC_MARKED;
					//if (!nd->p && nd->node_class == ama::N_FILE) {
					//	fprintf(stderr, "keep file %s\n", nd->data.data());
					//}
				} else {
					//it's free, put into the (now reorganized) free list
					nd->s = ama::g_free_nodes;
					ama::g_free_nodes = nd;
				}
			}
			ama::TBlockHeader* block_next = block->next;
			if (n_kept_before < n_kept) {
				//keep the block
				blocks_kept.push_back(block);
				//console.log('keep', block);
			} else {
				//free the block
				if (block == g_node_pool.block) {
					//if we free the current block, reset front and sz_free
					g_node_pool.block = nullptr;
					g_node_pool.front = nullptr;
					g_node_pool.sz_free = 0;
				}
				//remove the current block from the free list
				ama::g_free_nodes = nd_free_node_before;
				//console.log('free', block);
				free(block);
			}
			block = block_next;
		}
		//rebuild the memory pool linked list
		g_node_pool.block = nullptr;
		for (intptr_t i = blocks_kept.size() - 1; i >= 0; i--) {
			blocks_kept[i]->next = g_node_pool.block;
			g_node_pool.block = blocks_kept[i];
		}
		//console.log((void*)g_node_pool.block, (void*)g_node_pool.front, g_node_pool.sz_free, g_node_pool.block_size, ama::g_free_nodes);
		//fprintf(stderr, "n_kept = %d, n_freed = %d, n_swept = %lld\n", int(n_kept), int(n_freed), (long long)n_swept);
		ama::gcstring_gcsweep();
		return n_freed;
	}
	void DropAllMemoryPools() {
		ama::poolRelease(&g_node_pool);
		ama::gcstring_drop_pool();
		ama::g_free_nodes = nullptr;
	}
}
