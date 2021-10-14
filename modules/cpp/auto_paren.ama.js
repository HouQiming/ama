'use strict';
//@ama ParseCurrentFile().then(require("jcs").TranslateJCS)

function Transform(nd_root) {
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
}

module.exports = Transform;
