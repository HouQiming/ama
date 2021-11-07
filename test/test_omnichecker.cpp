/*
@ama
const checker=require('omnichecker');
let ret=checker.Check(ParseCurrentFile(),require('check/uninit'),require('check/undecl'));
//console.log(JSON.stringify(ret,null,1));
*/
#include "../src/ast/node.hpp"

namespace NS{
	int foo=3;
}

int main() {
	int uninitialized;
	int b=NS::foo+undeclared+uninitialized;
	return b;
}
