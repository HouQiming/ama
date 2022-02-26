'use strict';

/*
#filter Dump the AST (Abstract Syntax Tree).
*/
function Translate(nd_root) {
	nd_root.Validate();
	console.log(JSON.stringify(nd_root, null, 1));
}

module.exports = Translate;

let dump_ast = module.exports;
/*
#filter Dump parsed string values
*/
dump_ast.Strings = function(nd_root) {
	for (let nd_str of nd_root.FindAll(N_STRING)) {
		console.log(JSON.stringify(nd_str.GetStringValue()));
	}
	console.log('----------');
};

/*
#filter Dump detected declarations
*/
dump_ast.Declarations = function(nd_root) {
	const typing = require('cpp/typing');
	for (let nd_def of nd_root.FindAllDef()) {
		console.log(nd_def.data + ':', typing.ComputeType(nd_def).dump());
	}
	console.log('----------');
};
