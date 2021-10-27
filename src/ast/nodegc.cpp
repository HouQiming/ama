#include "node.hpp"
#include "../script/jsenv.hpp"
#include <vector>
#include "../util/jc_array.h"
namespace ama {
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
		for ( auto&& pair_nd_v: ama::g_js_node_map ) {
			//we could have hacky ->p which screws up rooting
			ama::Node const* nd = pair_nd_v.first;
			mark(Q, (ama::Node*)nd);
			mark(Q, nd->Root());
		}
		ama::mark(Q, ama::GetPlaceHolder());
		//recursion
		for (size_t i = intptr_t(0L); i < Q.size(); i += intptr_t(1L)) {
			ama::Node* nd = Q[i];
			ama::mark(Q, nd->c);
			ama::mark(Q, nd->s);
		}
		//sweep
		std::vector<ama::Node*> node_ranges = ama::GetAllPossibleNodeRanges();
		intptr_t n_freed = intptr_t(0L);
		for (int i = 0; i < node_ranges.size(); i += 2) {
			for (ama::Node* nd = node_ranges[i]; nd != node_ranges[i + 1]; nd += 1) {
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
				} else if ( (nd->tmp_flags & (ama::TMPF_GC_MARKED | ama::TMPF_IS_NODE)) == (ama::TMPF_GC_MARKED | ama::TMPF_IS_NODE) ) {
					//clear the marked flag for the next gc
					//fprintf(stderr,"keep node %p\n",nd);
					nd->tmp_flags &= ~ama::TMPF_GC_MARKED;
				}
			}
		}
		ama::gcstring_gcsweep();
		return n_freed;
	}
};
