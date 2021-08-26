/*
@ama
let nd_root=ParseCurrentFile().ParseOperators().StripBinaryOperatorSpaces();
console.log(JSON.stringify(nd_root,null,2))
console.log(nd_root.toSource())
*/

int main(){
	int a=3;
	int b=4;
	int c=~a++-b*-b<<(a%sizeof b/b*b/b*b);
	c+=a*b;
	return 0;
}
