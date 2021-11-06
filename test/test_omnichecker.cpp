/*
@ama
const checker=require('omnichecker');
checker.Check(ParseCurrentFile());
*/
#include "../src/ast/node.hpp"

namespace not_here{
	int foo=3;
}

int main() {
	int uninitialized;
	int b=foo+undeclared+uninitialized;
	return b;
}
