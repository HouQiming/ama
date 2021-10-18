const path=require('path');
const jsism=require('cpp/jsism');
const sane_types=require('cpp/sane_types');
const sane_init=require('cpp/sane_init');
const sane_export=require('cpp/sane_export');
const move_operator=require('cpp/move_operator');
const unified_null=require('cpp/unified_null');
let nd_root=ParseCurrentFile({parse_indent_as_scope:1})
	.AutoSemicolon()
	.then(sane_types.FixArrayTypes)
	.then(require('cpp/typing').DeduceAuto);
	.Save()
	.then(sane_types,{view:{to:.(JC::array_base<.(Node.MatchAny('TElement'))>)}})
	.then(sane_init)
	.then(sane_export)
	.then(move_operator)
	.then(unified_null)
	.then(jsism.EnableJSLambdaSyntax)
	.then(jsism.EnableJSON)
	.then(require('cpp/auto_decl'))
	.then(require('cpp/auto_paren'))
	.then(require('cpp/auto_header'),{audit:path.join(__dirname,'test_sanetypes.header.audit.cpp')})
	.Save(path.extname(__filename));
//make the per-file script take the translated source
ParseCurrentFile=function(){return nd_root;}
