const typing=require('cpp/typing');
const assert=require('assert');

function TranslatePureFunction(nd_stmt){
	if(nd_stmt.node_class==N_SCOPE&&nd_stmt.c&&!nd_stmt.c.s||nd_stmt.node_class==N_SEMICOLON){
		return TranslatePureFunction(nd_stmt.c);
	}
	if(nd_stmt.node_class==N_KEYWORD_STATEMENT&&nd_stmt.data=='return'&&nd_stmt.c){
		return TranslatePureFunction(nd_stmt.c);
	}
	if(nd_stmt.node_class==N_EXTENSION_CLAUSE&&nd_stmt.data=='else'){
		let nd_else=TranslatePureFunction(nd_stmt.c.s);
		if(!nd_else){return undefined;}
		return @(else @(TranslatePureFunction(nd_else)));
	}
	if(nd_stmt.node_class==N_SCOPED_STATEMENT&&nd_stmt.data=='if'){
		let nd_then=TranslatePureFunction(nd_stmt.c.s);
		if(!nd_then){return undefined;}
		let nd_ret=@(if @(TranslatePureFunction(nd_stmt.c)) then @(TranslatePureFunction(nd_then)));
		for(let ndi=nd_stmt.c.s.s;ndi;ndi=ndi.s){
			let nd_else=TranslatePureFunction(ndi);
			if(!nd_else){return undefined;}
			nd_ret.Insert(POS_BACK,nd_else);
		}
		return nd_ret;
	}
	if(nd_stmt.node_class!=N_SCOPED_STATEMENT&&nd_stmt.node_class!=N_KEYWORD_STATEMENT&&nd_stmt.node_class!=N_SCOPE&&nd_stmt.node_class!=N_EXTENSION_CLAUSE){
		return nd_stmt.Clone();
	}
	return undefined;
}

module.exports=function Transform(nd_root){
	nd_root.FindAll(N_REF,'int').forEach(nd=>nd.ReplaceWith('Int'));
	nd_root.FindAll(N_BINOP,'%').forEach(nd=>nd.data='`mod`');
	for(let nd_func of nd_root.FindAll(N_FUNCTION)){
		let nd_paramlist=nd_func.c.s;
		let nd_body=nd_func.LastChild();
		let nd_rettype=nd_func.c.Clone();
		if(nd_body.node_class==N_AIR){
			nd_func.Unlink();
			continue;
		}
		if(nd_rettype.node_class==N_RAW){
			//remove the function name
			nd_rettype.LastChild().Unlink();
		}
		let nd_oneliner=undefined;
		if(nd_body.c&&!nd_body.c.s&&(nd_oneliner=TranslatePureFunction(nd_body.c))){
			nd_body=nd_oneliner;
		}else{
			//multi-line: do a Monad
			nd_body=nd_body.BreakSelf();
			//remove the {}
			for(let nd_scope of nd_body.FindAll(N_SCOPE)){
				nd_scope.flags|=SCOPE_FROM_INDENT;
			}
			//translate flow control
			for(let nd_if of nd_body.FindAll(N_SCOPED_STATEMENT,'if')){
				let nd_then=nd_if.c.s;
				let nd_tmp=nd_then.ReplaceWith(Node.GetPlaceHolder());
				nd_tmp.ReplaceWith(nKeywordStatement('then',nd_then).setCommentsBefore(' '));
			}
			for(let nd_for of nd_body.FindAll(N_SCOPED_STATEMENT,'for')){
				let nd_cond=nd_for.c;
				assert(nd_cond.node_class==N_BINOP&&nd_cond.data=='<');
				let nd_body=nd_for.LastChild();
				if(nd_body.c&&nd_body.LastChild().comments_after=='\n'){
					nd_body.LastChild().setCommentsAfter('');
				}
				nd_for.ReplaceWith(@(mapM_ (\ @(nd_cond.c.Clone()) -> do @(nd_body.BreakSelf()) ) [0 .. (@(nd_cond.c.s.Clone())-1)]))
			}
			//add the 'do'
			nd_body=nScopedStatement('do',nAir(),nd_body);
		}
		nd_func.ReplaceWith(@(
			@(nRef(nd_func.data)) @(nRaw.apply(
				null,
				nd_paramlist.children.map(nd=>nd.FindAllDef()[0]).filter(nd=>nd).map(nd=>nd.setCommentsBefore(' '))
			)) = @(nd_body)
		));
	}
	//translate console.log
	for (let nd_print of nd_root.FindAll(N_CALL,'log')) {
		if(nd_print.c.Match(@(console.log))){
			let ret=[];
			for(let ndi=nd_print.c.s;ndi;ndi=ndi.s){
				ret.push(@(print $ @(ndi.Clone())).setCommentsBefore('\n'));
			}
			nd_print.ReplaceWith(nScope.apply(null,ret).c);
		}
	}
	nd_root.FindAll(N_SEMICOLON).forEach(nd=>nd.ReplaceWith(nd.BreakChild()));
	return nd_root;
}
