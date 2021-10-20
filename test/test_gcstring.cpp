//hack to simplify compilation
#include "../src/util/gcstring.cpp"
using namespace std;

int main(){
	{
		ama::gcstring a0="";
		ama::gcstring a1="a";
		ama::gcstring a2="aa";
		ama::gcstring a3="aaa";
		ama::gcstring a4="aaaa";
		ama::gcstring a5="aaaaa";
		ama::gcstring a6="aaaaaa";
		ama::gcstring a7="aaaaaaa";
		ama::gcstring a8="aaaaaaaa";
		cout<<a0.size()<<a1.size()<<a2.size()<<a3.size()<<a4.size()<<a5.size()<<a6.size()<<a7.size()<<a8.size()<<endl;
	}
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
	ama::gcstring_gcsweep();
	return 0;
}
