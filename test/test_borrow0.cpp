/*
@ama
const checker=require('omnichecker');
checker.Check(ParseCurrentFile(),require('check/borrow'),{dump_code:1});
*/
int main(){
	int* y;
	{
		int x = 5;
		y = &x;
		console.log(x);
	}
	*y += 1;
	return 0;
}
