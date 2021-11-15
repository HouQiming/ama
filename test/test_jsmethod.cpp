/*
@ama
let nd_root=ParseCurrentFile();
console.log(JSON.stringify(nd_root,null,1));
console.log(nd_root.toSource());
*/

Node.setFlags = function(flags) {
	this.flags = flags;
	return this;
}
