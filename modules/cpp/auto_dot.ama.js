const typing = require('cpp/typing');

module.exports = function Transform(nd_root) {
	for (let nd_dot of nd_root.FindAll(N_DOT)) {
		if (!nd_dot.flags) {
			let type = typing.ComputeType(nd_dot.c);
			if (type && type.node_class == N_POSTFIX && type.data == '*') {
				nd_dot.flags = DOT_PTR;
			}
		}
	}
}