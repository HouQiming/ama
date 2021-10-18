const path=require('path');
const jsism=require('cpp/jsism');
const sane_types=require('cpp/sane_types');
const sane_init=require('cpp/sane_init');
const sane_export=require('cpp/sane_export');
const move_operator=require('cpp/move_operator');
const unified_null=require('cpp/unified_null');
ParseCurrentFile()
	.StripRedundantPrefixSpace()
	.then(jsism.EnableJSLambdaSyntax.inverse)
	.then(unified_null.inverse)
	.then(move_operator.inverse)
	.then(sane_export.inverse)
	.then(sane_init.inverse)
	.then(sane_types.inverse,{view:{to:.(JC::array_base<.(Node.MatchAny('TElement'))>)}})
	.Save('.ama' + path.extname(__filename));
//ignore the per-file script
return;
