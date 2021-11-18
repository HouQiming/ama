'''
@ama
let nd_root=ParseCurrentFile();
//multi-line lambda
for(let nd_func of nd_root.FindAll(N_FUNCTION)){
	if(nd_func.c.isRef('lambda')&&nd_func.LastChild().comments_before.indexOf('\n')>=0){
		let nd_stmt=nd_func.ParentStatement(); 
		let code=nd_func.toSource()+";";
		let nd_reparsed=ParseCode(code,nd_root.GetCompleteParseOption()).Find(N_FUNCTION);
		if(!nd_reparsed){continue;}
		let name='LongLambdaL'+nd_stmt.ComputeLineNumber().toString();
		let func_children=nd_reparsed.children;
		func_children[0].ReplaceWith(nRaw(nRef('def'),nRef(name).setCommentsBefore(' ')));
		func_children[1].flags&=~PARAMLIST_UNWRAPPED;
		func_children[1].comments_before='';
		nd_reparsed.comments_after='';
		nd_func.ReplaceWith(nRef(name)).setCommentsAfter('');
		nd_stmt.Insert(POS_BEFORE,nd_reparsed.setIndent(nd_stmt.indent_level).setCommentsBefore('\n'));
	}
}
//methodified 'map'
for(let nd_call of nd_root.FindAll(N_CALL,'map')){
	if(nd_call.c.node_class==N_DOT){
		let nd_obj=nd_call.c.c;
		nd_call.ReplaceWith(@(map(@(nd_call.c.s),@(nd_obj))))
	}
}
nd_root.Save('.audit.py');
console.log(JSON.stringify(nd_root,null,1));
console.log(nd_root.toSource());
'''

def main():
	input=[1,2,3]
	print(list(input.map(lambda v:v+1)))
	acc=[0]
	print(list(input.map(lambda v:
		acc[0]+=v
		return acc[0]
	)))

main()
