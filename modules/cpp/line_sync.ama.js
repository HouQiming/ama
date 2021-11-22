'use strict';
/*
#filter Allow using `#line __AMA_LINE__` to synchronize object file line numbers to `foo.ama.cpp`
The filter simply replaces `__AMA_LINE__` with actual line numbers.
Before:
```C++
#pragma gen(some_generated_code)
#line __AMA_LINE__
int main(){
	return 0;
}
```
*/
function Translate(nd_root) {
	for (let nd_line of nd_root.FindAll(N_KEYWORD_STATEMENT, '#line')) {
		nd_line.c.ReplaceWith(nNumber((nd_line.ComputeLineNumber() + 2).toString()));
	}
	return nd_root;
}

function Untranslate(nd_root) {
	for (let nd_line of nd_root.FindAll(N_KEYWORD_STATEMENT, '#line')) {
		nd_line.c.ReplaceWith(nRef('__AMA_LINE__'));
	}
	return nd_root;
}

Translate.inverse = Untranslate;
module.exports = Translate;
