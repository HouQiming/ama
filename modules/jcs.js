'use strict'
let jcs=module.exports;

jcs.TranslateJCS=function(nd_root){
	for(let nd of nd_root.FindAll(N_BINOP,'!==')){
		nd.data='!=';
	}
	for(let nd of nd_root.FindAll(N_BINOP,'===')){
		nd.data='==';
	}
	//any require would create a circular dependency, so copy the code from cpp/autoparen
	for (let ndi of nd_root.FindAll(N_SCOPED_STATEMENT, null)) {
		if (ndi.c.node_class != N_PAREN && !ndi.c.isRawNode('(', ')')) {
			let nd_arg = ndi.c;
			let nd_tmp = Node.GetPlaceHolder()
			nd_arg.ReplaceWith(nd_tmp);
			if (nd_arg.node_class == N_LABELED && nd_arg.c.s.node_class == N_AIR) {
				nd_arg = nd_arg.c;
			}
			nd_tmp.ReplaceWith(nParen(nd_arg));
		}
	}
	nd_root.then(require('auto_semicolon')).Save();
	for(let nd of nd_root.FindAll(N_BINOP,'!=')){
		nd.data='!==';
	}
	for(let nd of nd_root.FindAll(N_BINOP,'==')){
		nd.data='===';
	}
	nd_root.NodeofToASTExpression().Save('.js');
	return nd_root;
}
