//Quality-of-life script: automatically create a cmake project for this file
//and its dependencies, then build and run
//////////
//@ama
//require('cmake');
//const path=require('path');
//ParseCurrentFile().CreateCXXCMakeTarget(path.resolve(__dirname,'./CMakeLists.txt'),{build:"debug",run:[]});
#include <stdio.h>
#include "func.h"

int main(){
	printf("hello world #%d\n", ComputeSomeNumber());
	return 0;
}
