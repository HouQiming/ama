//@ama ParseCurrentFile().then(require('./sjlj_exception.ama.js')).Save('.audit.c')
//ama test.c && cc test.audit.c -lm && ./a.out
#include <stdio.h>
#include <math.h>

float maybe_sqrt(float a){
	if(!(a>=0.f)){
		throw "bad input";
	}
	return sqrtf(a);
}

int main(){
	try{
		printf("maybe_sqrt(3.f) = %f\n", maybe_sqrt(3.f));
		printf("maybe_sqrt(-1.f) = %f\n", maybe_sqrt(-1.f));
	}catch(e){
		printf("got exception: %s\n", e);
	}
	return 0;
}
