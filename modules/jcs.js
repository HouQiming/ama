'use strict'
let jcs=module.exports;

jcs.TranslateJCS=function(nd_root){
	for(let nd of nd_root.FindAll(N_SYMBOL,'!==')){
		nd.data='!=';
	}
	for(let nd of nd_root.FindAll(N_SYMBOL,'===')){
		nd.data='==';
	}
	nd_root.Save()
	for(let nd of nd_root.FindAll(N_SYMBOL,'!=')){
		nd.data='!==';
	}
	for(let nd of nd_root.FindAll(N_SYMBOL,'==')){
		nd.data='===';
	}
	nd_root.NodeofToASTExpression().Save('.js');
	return nd_root;
}
