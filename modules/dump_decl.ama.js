'use strict';
const typing = require('cpp/typing');

/*
#filter Dump detected declarations
*/
function Translate(nd_root) {
	for (let nd_def of nd_root.FindAllDef()) {
		console.log(nd_def.data + ':', typing.ComputeType(nd_def).dump());
	}
	console.log('----------');
}

module.exports = Translate;
