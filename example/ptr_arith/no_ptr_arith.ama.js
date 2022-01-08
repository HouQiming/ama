const typing=require('cpp/typing');

function isPointer(nd){
	let type=typing.ComputeType(nd);
	return type&&type.node_class==N_POSTFIX&&type.data=='*';
}

module.exports=function(nd_root){
	for(let nd_arith of nd_root.FindAll(N_ASSIGNMENT).filter(nd=>nd.data).concat(nd_root.FindAll(N_BINOP))){
		if(isPointer(nd_arith.c)){
			console.log(nd_arith.c.FormatFancyMessage('arithmetic on pointer `'+nd_arith.c.dump()+'` is not allowed', MSG_COLORED|MSG_WARNING));
		}else if(isPointer(nd_arith.c.s)){
			console.log(nd_arith.c.FormatFancyMessage('arithmetic on pointer `'+nd_arith.c.s.dump()+'` is not allowed', MSG_COLORED|MSG_WARNING));
		}
	}
	//suppress the default code dumping
	Node.Print=function(){};
	return nd_root;
}
