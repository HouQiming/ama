#include <iostream>
#include "../modules/cpp/json/json.h"

/*
@ama
ParseCurrentFile().then(require('cpp/jsism').EnableJSON).Save('.audit.cpp');
*/

struct TestStruct{
	int a;
	float b;
	std::string s;
};

int main(){
	TestStruct s{42,3.14f,"hello world"};
	std::cout<<JSON.stringify(s)<<std::endl;
	s=JSON.parse<TestStruct>("{\"a\":24,\"aa\":44,\"s\":\"world hello\",\"b\":14.3f,\"bb\":1.43f}");
	std::cout<<s.a<<' '<<s.b<<' '<<s.s<<std::endl;
	return 0;
}
