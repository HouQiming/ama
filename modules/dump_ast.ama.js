'use strict';

/*
#filter Dump the AST (Abstract Syntax Tree).
*/
function Translate(nd_root) {
	//for(let ndi=nd_root;ndi;ndi=ndi.PreorderNext()){
	//	if(ndi.node_class==N_STRING){ndi.GetStringValue();}
	//}
	console.log(JSON.stringify(nd_root, null, 1));
}

module.exports = Translate;
