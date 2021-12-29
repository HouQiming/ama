const typing = require('cpp/typing');

/*
#filter Automatically deduce `->` or `::` from `.`
Before:
```C++
namespace ama{
	struct Node{...};
}
ama.Node* GetChild(ama.Node* nd){
	return nd.c;
}
```
*/
module.exports = function Translate(nd_root) {
	for (let nd_dot of nd_root.FindAll(N_DOT)) {
		if (!nd_dot.flags) {
			if (nd_dot.c.isRef('this')) {
				nd_dot.flags = DOT_PTR;
				continue;
			}
			let type = typing.ComputeType(nd_dot.c);
			//unique_ptr has .get()... so no == '^' check
			if (type && type.node_class == N_POSTFIX && (type.data == '*')) {
				nd_dot.flags = DOT_PTR;
			} else if (type && type.node_class == N_CLASS && type.data == 'namespace') {
				nd_dot.flags = DOT_CLASS;
			}
		}
	}
};