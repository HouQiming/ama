const amacpp=require('cpp/amacpp');
const path=require('path');
let nd_root=ParseCurrentFile({parse_indent_as_scope:1}).then(amacpp);
nd_root.Save(path.extname(__filename));
//make the per-file script take the translated source
ParseCurrentFile=function(){return nd_root;}
