'use strict';

/*
#filter Dump long N_RAW nodes.
*/
function Translate(nd_root) {
	for (let nd_raw of nd_root.FindAll(N_RAW)) {
		if (nd_raw.toSource().length >= 64) {
			console.log('-----------------');
			console.log(nd_raw.toSource());
			console.log('---');
			console.log(JSON.stringify(nd_raw, null, 1));
		}
	}
}

module.exports = Translate;
