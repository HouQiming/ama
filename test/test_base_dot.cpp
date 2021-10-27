/*
@ama
const amacpp=require('cpp/amacpp');
let nd_root=ParseCurrentFile({parse_indent_as_scope:1});
//console.log(JSON.stringify(nd_root,null,1));
console.log(nd_root.then(amacpp).toSource({auto_space:0}));
*/
class Base{
	Base* x;
	int d;
};

class Derived:public Base{
	Derived* v;
};

int main(){
	Derived a;
	console.log(a.x.d);
	console.log(a.v.x.d);
	return 0;
}
