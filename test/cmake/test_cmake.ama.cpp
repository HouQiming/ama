#include <stdio.h>

#if 0
	@ama
	require('cmake');
	let nd_root=ParseCurrentFile();
	//console.log(JSON.stringify(nd_root,null,2))
	//console.log(nd_root.toSource())
	nd_root
		.then(require('jsism').EnableConsole)
		.InsertCommentBefore('//ama output\n')
		.UpdateCMakeLists('./CMakeLists.txt');
#endif

int main(){
	console.log(
		"hello world",
		(0.25).toFixed(3).padStart(8),
		console.format(12,'+',34,'=',(12.0+34.0).toExponential())
	);
	return 0;
}
