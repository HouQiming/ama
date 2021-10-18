//@ama ParseCurrentFile().then(require("jcs").TranslateJCS)
const path = require('path');
const jsism = require('cpp/jsism');
const sane_types = require('cpp/sane_types');
const sane_init = require('cpp/sane_init');
const sane_export = require('cpp/sane_export');
const move_operator = require('cpp/move_operator');
const unified_null = require('cpp/unified_null');
module.exports = ParseCurrentFile=>{
	let nd_root = ParseCurrentFile({parse_indent_as_scope: 1})
	.StripRedundantPrefixSpace()
	.then(jsism.EnableJSLambdaSyntax.inverse)
	.then(unified_null.inverse)
	.then(move_operator.inverse)
	.then(sane_export.inverse)
	.then(sane_init.inverse)
	.then(sane_types.inverse, {view: {to: .(JC::array_base<.(Node.MatchAny('TElement'))>)}})
	.Save('.audit.cpp');
	//////////////////////
	console.log(JSON.stringify(nd_root, null, 1));
	console.flush();
	//////////////////////
	nd_root
	.StripRedundantPrefixSpace()
	.then(require('auto_semicolon'))
	.then(sane_types.FixArrayTypes)
	.then(require('cpp/typing').DeduceAuto)
	.then(require('cpp/auto_paren'))
	.then(sane_types, {view: {to: .(JC::array_base<.(Node.MatchAny('TElement'))>)}})
	.then(sane_init)
	.then(sane_export)
	.then(move_operator)
	.then(unified_null)
	.then(jsism.EnableJSLambdaSyntax)
	.then(jsism.EnableJSON)
	.then(require('cpp/auto_decl'))
	.then(require('cpp/auto_header'))
	.Save('.aba.audit.cpp');
	__system('diff ' + nd_root.data + ' ' + nd_root.data.replace(/\.cpp$/, '.aba.audit.cpp'));
	return nd_root;
};
