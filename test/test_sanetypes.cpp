#include <stdio.h>

/*
@ama
const sane_types=require('cpp/sane_types');
const sane_init=require('cpp/sane_init');
const move_operator=require('cpp/move_operator');
let nd_root=ParseCurrentFile()
	.then(sane_types)
	.then(sane_init)
	.then(move_operator)
	.Save('.audit.cpp');
//console.log(JSON.stringify(nd_root,null,1));
nd_root
	.then(move_operator.inverse)
	.then(sane_init.inverse)
	.then(sane_types.inverse)
	.Save('.aba.audit.cpp')
*/

namespace ama {
	typedef int(*FGeneratorHook)(ama::CodeGenerator*,ama::Node*);
	typedef int(*FGeneratorHookComment)(ama::CodeGenerator*,const char*,intptr_t,int,intptr_t);
	struct CodeGenerator {
		//ignore offsets for now
		//COULDDO: provide a separate service to convert some (unicode) tag into cite-like comments
		std::string code;
		Node* nd_current;
		void const* opaque;
		FGeneratorHook hook;
		FGeneratorHookComment hook_comment{};
		intptr_t scope_indent_level = intptr_t(0L);
		intptr_t p_last_indent = intptr_t(0L);
		int32_t tab_width = 4;
		int8_t auto_space = 1;
		int8_t tab_indent = 1;
		void GenerateDefault(ama::Node* nd);
		void GenerateComment(int is_after, intptr_t expected_indent_level, JC::array_base<char> comment);
		void Generate(ama::Node* nd);
		void GenerateSpaceBefore(ama::Node* nd_next);
		void GenerateIndent(intptr_t expected_indent_level);
		void GenerateCommentDefault(int is_after, intptr_t expected_indent_level, JC::array_base<char> comment);
		void GenerateSpaceAfter(ama::Node* nd_last);
		void GenerateSpaceBetween(ama::Node* nd_last, ama::Node* nd_next);
	};
}

int[] test(char[:] a,uint32_t[]*[:]& b){
	int[] a;
	float[9]! b;
	Map<char[],double> c;
	return 0;
}

ama::ExecNode*[] ama::ExecSession::ComputeReachableSet(ama::ExecNode*[:] entries, int32_t dir) {
	ama::ExecNode*[] Q{};
	Map<ama::ExecNode*, intptr_t> inQ{};
	for ( ama::ExecNode*& ed: entries ) {
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
	ama::CodeGenerator gctx{{}, nullptr, nullptr, nullptr, nullptr, intptr_t(0L), intptr_t(0L), 4, 1};
	return <<Q;
}
