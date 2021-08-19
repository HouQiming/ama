'use strict'
const v3macro=module.exports;
v3macro.ASTConstructor=function(my_call,args,cns_name){
	let nd=nNull()
	for(let i=args.length-1;i>=0;i--){
		nd=nCall(nRef('cons'),args[i],nd);
	}
	//return nCall(nRef('NODE').dot('CreateNode').setFlags(DOT_CLASS),nRef(cns_name),nd)
	return nCall(nRef('ama').dot('CreateNode').setFlags(DOT_CLASS),nRef(cns_name),nd)
}
