//DO NOT use ama features: this is a "chicken" file which gets called when formatting other JS files
'use strict';

function Transform(nd_root) {
	for (let ndi of nd_root.FindAll(N_SCOPED_STATEMENT, null)) {
		if(ndi.data==='template'){continue;}
		if(ndi.data==='for'&&ndi.c.node_class === N_AIR&&ndi.c.s.node_class===N_SCOPE&&ndi.p&&ndi.p.isRawNode(0,0)){
			//relaxed for syntax
			let nd_container=ndi.p;
			let nd_init=ndi.c.s.c||nAir();
			nd_body=nd_container.LastChild();
			if(nd_body.node_class===N_SCOPE){
				if(!nd_init.s&&nd_init.node_class===N_SEMICOLON){nd_init=nd_init.c;}
				ndi.ReplaceWith(nd_init);
				nd_body.Unlink();
				let nd_tmp=Node.GetPlaceHolder();
				nd_container.ReplaceWith(nd_tmp);
				nd_container.flags=0x2928;//()
				nd_tmp.ReplaceWith(nScopedStatement('for',nd_container,nd_body));
				continue;
			}
		}
		if (ndi.c.node_class !== N_PAREN && !ndi.c.isRawNode('(', ')') && ndi.c.node_class !== N_AIR) {
			let nd_arg = ndi.c;
			let nd_tmp = Node.GetPlaceHolder()
			nd_arg.ReplaceWith(nd_tmp);
			if (nd_arg.node_class === N_LABELED && nd_arg.c.s.node_class === N_AIR) {
				nd_arg = nd_arg.c;
			}
			nd_tmp.ReplaceWith(nParen(nd_arg));
		}
	}
}

module.exports = Transform;
