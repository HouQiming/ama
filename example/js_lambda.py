'''
@ama
let nd_root=ParseCurrentFile();
//multi-line lambda
for(let nd_func of nd_root.FindAll(N_FUNCTION)){
	//before, paramlist, after, body
	if(nd_func.c.s.s.isSymbol('=>')){
		let nd_stmt=nd_func.ParentStatement();
		let name='LongLambdaL'+nd_stmt.ComputeLineNumber().toString();
		nd_func.c.ReplaceWith(nRaw(nRef('def'),nRef(name).setCommentsBefore(' ')).setCommentsBefore('\n'));
		nd_func.c.s.flags&=~PARAMLIST_UNWRAPPED;
		nd_func.c.s.s.data=':';
		let nd_scope=nd_func.LastChild();
		nd_scope.flags=SCOPE_FROM_INDENT;
		if(nd_scope.LastChild()){
			nd_scope.LastChild().comments_after='';
		}
		nd_func.ReplaceWith(nRef(name));
		nd_func.indent_level=nd_stmt.indent_level;
		nd_stmt.Insert(POS_BEFORE,nd_func);
	}
}
//methodified 'map'
for(let nd_call of nd_root.FindAll(N_CALL,'map')){
	if(nd_call.c.node_class==N_DOT){
		let nd_obj=nd_call.c.c;
		nd_call.ReplaceWith(.(map(.(nd_call.c.s),.(nd_obj))))
	}
}
nd_root.Save('.audit.py');
console.log(nd_root.toSource());
'''

def main():
	input=[1,2,3]
	print(list(input.map(lambda v:v+1)))
	acc=[0]
	print(list(input.map(v=>{
		acc[0]+=v
		return acc[0]
	})))

main()
