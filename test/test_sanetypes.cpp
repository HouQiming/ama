#include <stdio.h>

/*
@ama
let nd_root=ParseCurrentFile()
	.then(require('cpp/sane_types'))
	.Save('.audit.cpp');
console.log(JSON.stringify(nd_root,null,1));
nd_root.then(require('cpp/sane_types').inverse)
	.Save('.aba.audit.cpp')
*/

int[] test(char[:] a,uint32_t[]*[:]& b){
	int[] a;
	float[9] b;
	Map<char[],double> c;
	return 0;
}

ama::ExecNode*[] ama::ExecSession::ComputeReachableSet(ama::ExecNode*[:] entries, int32_t dir) {
	ama::ExecNode*[] Q{};
	Map<ama::ExecNode*, intptr_t> inQ{};
	for ( ama::ExecNode * & ed: entries ) {
		Q.push_back(ed);
		JC::map_set(inQ, ed, 1);
	}
	for (int qi = 0; qi < Q.size(); qi += 1) {
		if ( dir & ama::REACH_FORWARD ) {
			for (ama::ExecNodeExtraLink* link = Q[qi]->next.more; link; link = link->x) {
				ama::ExecNode* edi = link->target;
				if ( !JC::map_get(inQ, edi) ) {
					JC::push(Q, edi);
					JC::map_set(inQ, edi, 1);
				}
			}
		}
		if ( dir & ama::REACH_BACKWARD ) {
			for (ama::ExecNodeExtraLink* link = Q[qi]->prev.more; link; link = link->x) {
				ama::ExecNode* edi = link->target;
				if ( !JC::map_get(inQ, edi) ) {
					JC::push(Q, edi);
					JC::map_set(inQ, edi, 1);
				}
			}
		}
	}
	return std::move(Q);
}
