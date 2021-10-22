const path=require('path');
const jsism=require('cpp/jsism');
const short_types=require('cpp/short_types');
const sane_types=require('cpp/sane_types');
const sane_init=require('cpp/sane_init');
const sane_export=require('cpp/sane_export');
const move_operator=require('cpp/move_operator');
const unified_null=require('cpp/unified_null');
function ToCPP(nd_root,options){
	if(!options){options={};}
	(nd_root
		.StripRedundantPrefixSpace()
		.then(require('auto_semicolon'))
		.then(require('auto_paren'))
		.then(sane_types.FixArrayTypes)
		.then(require('cpp/auto_decl'))
		.then(require('cpp/typing').DeduceAuto)
		.then(require('cpp/auto_dot'))
		.then(require('cpp/cpp_indent'))
	);
	if(options.update_source){nd_root.Save();}
	return (nd_root
		.then(short_types)
		.then(sane_types)
		.then(sane_init)
		.then(sane_export)
		.then(move_operator)
		.then(unified_null)
		.then(jsism.EnableJSLambdaSyntax)
		.then(jsism.EnableJSON)
		.then(jsism.EnableConsole)
		.then(jsism.EnableSingleQuotedStrings)
		.then(require('cpp/auto_header'))
	);
}

function FromCPP(nd_root){
	return (nd_root
		.then(require('cpp/gentag').DropGeneratedCode)
		.StripRedundantPrefixSpace()
		.then(jsism.EnableJSLambdaSyntax.inverse)
		.then(unified_null.inverse)
		.then(move_operator.inverse)
		.then(sane_export.inverse)
		.then(sane_init.inverse)
		.then(sane_types.inverse)
		.then(short_types.inverse)
	);
}

module.exports=ToCPP;
module.exports.inverse=FromCPP;
