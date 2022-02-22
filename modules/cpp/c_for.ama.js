//Parser for C `for(init;cond;iter){}` loops
'use strict';
module.exports = function ParseCFor(nd_for) {
	if (!(nd_for.node_class == N_SCOPED_STATEMENT && nd_for.data == 'for')) {
		return undefined;
	}
	if (!nd_for.c.isRawNode('(', ')')) {
		return undefined;
	}
	let raw_parts = nd_for.c.children;
	if (raw_parts.length != 5 || !raw_parts[1].isSymbol(';') || !raw_parts[3].isSymbol(';')) {
		return undefined;
	}
	return {
		init: raw_parts[0],
		cond: raw_parts[2],
		iter: raw_parts[4],
	};
};
