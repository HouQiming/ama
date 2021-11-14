//A simple language feature: convert JS-like console.log to C++ iostream
//See ../modules/cpp/jsism.ama.js for the implementation
#include <stdio.h>

//@ama
//let nd_root=ParseCurrentFile();
//nd_root
//	.then(require('cpp/jsism').EnableConsole)
//	.InsertCommentBefore('//ama output\n')
//	.Save('.audit.cpp')

int main(){
	console.log(
		"hello world",
		(0.25).toFixed(3).padStart(8),
		console.format(12,'+',34,'=',(12.0+34.0).toExponential())
	);
	return 0;
}
