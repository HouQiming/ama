//hack to simplify compilation
#include "../src/util/gcstring.cpp"
using namespace std;

int main(){
	ama::gcstring a="he";
	cout<<a.val<<' '<<a<<endl;
	a=a+"llo";
	cout<<a.val<<' '<<a<<endl;
	ama::gcstring b="hello";
	cout<<a.size()<<' '<<b.size()<<endl;
	cout<<(a==b)<<' '<<(a!=b)<<' '<<(a<=b)<<endl;
	cout<<(a=="hello world")<<' '<<(a!="hello world")<<' '<<(a<="hello world")<<endl;
	b=" world";
	cout<<a.size()<<' '<<b.size()<<endl;
	cout<<(a==b)<<' '<<(a!=b)<<' '<<(a<=b)<<endl;
	cout<<(a=="hello world")<<' '<<(a!="hello world")<<' '<<(a<="hello world")<<endl;
	a=a+" world";
	cout<<a<<endl;
	b="hello world";
	cout<<a.size()<<' '<<b.size()<<endl;
	cout<<(a==b)<<' '<<(a!=b)<<' '<<(a<=b)<<endl;
	cout<<(a=="hello world")<<' '<<(a!="hello world")<<' '<<(a<="hello world")<<endl;
	return 0;
}
