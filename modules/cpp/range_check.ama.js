'use strict';
const typing = require('cpp/typing');

/*
#filter Transform `[]` on `std::vector` into `.at()`, should be used after `require("sane_types")`.
*/
function EnableVectorRangeCheck(nd_root) {
	for (let nd_item of nd_root.FindAll(N_ITEM)) {
		if (nd_item.ComputeChildCount() != 2) {continue;}
		let nd_obj = nd_item.c;
		let type = typing.ComputeType(nd_obj);
		if (type && type.node_class == N_CALL_TEMPLATE && type.GetName() == 'vector') {
			let nd_index = nd_item.c.s;
			nd_index.Unlink();
			nd_obj.Unlink();
			nd_item.ReplaceWith(@(@(nd_obj).at(@(nd_index))));
		}
	}
}

/*
#filter Transform `.at()` on `std::vector` into `[]`, should be used before `require("sane_types").inverse`.
*/
function DisableVectorRangeCheck(nd_root) {
	for (let nd_at of nd_root.FindAll(N_CALL, 'at')) {
		if (nd_at.c.node_class != N_DOT || nd_at.ComputeChildCount() != 2) {continue;}
		let nd_obj = nd_at.c.c;
		let type = typing.ComputeType(nd_obj);
		if (type && type.node_class == N_CALL_TEMPLATE && type.GetName() == 'vector') {
			let nd_index = nd_at.c.s;
			nd_index.Unlink();
			nd_obj.Unlink();
			nd_at.ReplaceWith(nItem(nd_obj, nd_index));
		}
	}
}

EnableVectorRangeCheck.inverse = DisableVectorRangeCheck;

module.exports = EnableVectorRangeCheck;
