'use strict';

/*
#filter Dump the AST (Abstract Syntax Tree).
*/
function Translate(nd_root) {
	nd_root.Validate();
	console.log(JSON.stringify(nd_root, null, 1));
}

module.exports = Translate;
