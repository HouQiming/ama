#include <stdio.h>

#if 0
	@ama
	let nd_cmake=require('cmake').LoadCMakeFile('/home/hqm/tp/ama/src/entry/CMakeLists.txt');
	//console.log(JSON.stringify(nd_cmake,null,1))
	//console.log(nd_cmake.toSource())
	console.log(nd_cmake.CMakeFindTarget('ama').toSource())
#endif

int main(){
	printf("hello world\n");
	return 0;
}
