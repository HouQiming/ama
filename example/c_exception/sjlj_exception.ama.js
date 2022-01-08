module.exports=function(nd_root,options){
	nd_root.Insert(POS_FRONT,@(
	#include <memory.h>
	#include <stdlib.h>
	#include <setjmp.h>
	static const char* g_exception = NULL;
	static jmp_buf g_current_try;
	));
	for(let nd_throw of nd_root.FindAll(N_KEYWORD_STATEMENT,'throw')){
		if(nd_throw.c){
			nd_throw.ReplaceWith(@((g_exception=@(nd_throw.BreakChild()),longjmp(g_current_try, 1))));
		}
	}
	for(let nd_try of nd_root.FindAll(N_SCOPED_STATEMENT,'try')){
		let nd_catch=nd_try.FindAllWithin(BOUNDARY_ONE_LEVEL,N_EXTENSION_CLAUSE,'catch')[0];
		let nd_catch_body=nd_catch&&nd_catch.c.BreakSibling()||@(;);
		let nd_caught_var=nd_catch&&nd_catch.BreakChild()||@(e);
		nd_try.ReplaceWith(@(
			jmp_buf outer_try;
			memcpy(&outer_try,&g_current_try,sizeof(jmp_buf));
			if(setjmp(g_current_try)){
				const char* @(nd_caught_var) = g_exception;
				memcpy(&g_current_try,&outer_try,sizeof(jmp_buf));
				@(nd_catch_body||@(;))
			}else{
				@(nd_try.c.s.BreakChild())
			}
			memcpy(&g_current_try,&outer_try,sizeof(jmp_buf));
		));
	}
	let nd_main=nd_root.Find(N_FUNCTION,'main');
	if(nd_main&&nd_main.LastChild().node_class==N_SCOPE){
		nd_main.LastChild().Insert(POS_FRONT,@(
			if(setjmp(g_current_try)){
				printf("unhandled exception %s\n", g_exception);
				abort();
			}
		).setIndent(4));
	}
	return nd_root;
}
