#include <stdio.h>

/*
@ama
let nd_root=ParseCurrentFile();
//console.log(JSON.stringify(nd_root,null,2))
//console.log(nd_root.toSource())
nd_root
	.then(require('cpp/jsism').EnableConsole)
	.InsertCommentBefore('//ama output\n')
	.Save('.audit.cpp')
*/

int main(){
	console.log(
		"hello world",
		(0.25).toFixed(3).padStart(8),
		console.format(12,'+',34,'=',(12.0+34.0).toExponential())
	);
	return 0;
}
