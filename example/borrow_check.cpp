//Rust-like borrow checking
//@ama
//const checker=require('omnichecker');
//checker.Check(ParseCurrentFile(),require('check/cpp/borrow'));
int main(){
	int* y;
	{
		int x = 5;
		y = &x;
		int const* z = &x;
	}
	*y += 1;
	return 0;
}
