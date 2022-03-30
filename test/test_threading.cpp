#include <stdio.h>
#include <thread>
#include "../src/script/jsapi.hpp"
#include "../src/ast/nodegc.hpp"

int n=100000;

void test(int id){
	std::vector<ama::Node*> nds;
	for(int i=0;i<n;i++){
		if(i%10000==0){printf("thread %d: parse %d\n",id,i);}
		nds.push_back(ama::DefaultParseCode("function test(){return 42+'test';}"));
	}
	for(int i=0;i<n;i++){
		if(i%10000==0){printf("thread %d: validate %d\n",id,i);}
		nds[i]->Validate();
	}
}

void thread_main(){
	test(1);
}

int main(){
	ama::enable_threading=1;
	ama::LazyInitScriptEnv();
	std::thread other_thread(thread_main);
	test(0);
	other_thread.join();
	ama::gc();
	puts("good");
	return 0;
}
