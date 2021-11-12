/*
@ama
const checker=require('omnichecker');
let ret=checker.Check(ParseCurrentFile(),require('check/uninit'),require('check/undecl'),{inherit_comments:1,dump_code:1});
//console.log(JSON.stringify(ret,null,1));
*/
#include "../src/ast/node.hpp"

namespace NS{
	int foo=3;
}

int main() {
	int uninitialized;
	int b=NS::foo+undeclared+uninitialized;
	int c;
	for(int i=0;i<10;i++){
		if(i==3){
			c=7;
		}
		int d=c;
	}
	int e=c;
	return b;
}
