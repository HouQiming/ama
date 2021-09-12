/*
@ama
let nd_root=ParseCurrentFile();
console.log(JSON.stringify(nd_root,null,2))
console.log(nd_root.toSource())
*/

#define JUMP(type) \
  sljit_emit_jump(compiler, (type))
