/*
@ama
const checker=require('omnichecker');
checker.Check(ParseCurrentFile(),require('check/borrow'),{dump_code:1});
*/
int x = 5;
int* y = &x;
*y += 1;
console.log(x);
