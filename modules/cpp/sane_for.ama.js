'use strict'
module.exports = function Transform(nd_root) {
	for (let nd_for of nd_root.FindAll(N_SCOPED_STATEMENT, 'for')) {
		let nd_range = nd_for.c;
		if (nd_range.node_class == N_PAREN || nd_range.node_class == N_LABELED) {
			nd_range = nd_range.c;
		}
		//for-in
		if (nd_range.node_class == N_BINOP && (nd_range.data == 'in' || nd_range.data == 'of')) {
			nd_range.node_class = N_LABELED;
			nd_range.data = '';
		}
		//parse for(:) as declaration
		if (nd_range.node_class == N_LABELED) {
			let nd_def = nd_range.c;
			if (nd_def.node_class == N_REF) {
				nd_def.flags |= REF_WRITTEN;
			} else if (nd_def.node_class == N_RAW && nd_def.c && nd_def.LastChild().node_class == N_REF) {
				nd_def.LastChild().flags |= REF_WRITTEN | REF_DECLARED;
			}
		}
	}
}
