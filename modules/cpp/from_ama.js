const amacpp=require('cpp/amacpp');
const path=require('path');
let nd_root=ParseCurrentFile({parse_indent_as_scope:1,auto_curly_bracket:1,parse_indent_as_scope_but_merge_cpp_ctor_lines:1}).then(amacpp,{update_source:1});
nd_root.Save(path.extname(__filename));
//make the per-file script take the translated source
ParseCurrentFile=function(){return nd_root;}
