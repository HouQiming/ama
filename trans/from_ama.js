const amacpp=require('amacpp');
let nd_root=ParseCurrentFile({parse_indent_as_scope:1}).then(amacpp).Save(path.extname(__filename));
//make the per-file script take the translated source
ParseCurrentFile=function(){return nd_root;}
