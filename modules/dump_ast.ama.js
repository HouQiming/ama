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

/*
#filter Dump detected classes
*/
dump_ast.Classes = function(nd_root) {
	const classes = require('class');
	for (let nd_class of nd_root.FindAll(N_CLASS)) {
		let desc = nd_class.ParseClass();
		if (desc) {
			desc.nd = desc.nd.dump();
			for (let i = 0; i < desc.base_classes.length; i++) {
				desc.base_classes[i] = desc.base_classes[i].dump();
			}
			for (let i = 0; i < desc.properties.length; i++) {
				if (desc.properties[i].node) {
					desc.properties[i].node = desc.properties[i].node.dump();
				}
			}
		}
		console.log(nd_class.data + ':', desc);
	}
	console.log('----------');
};
