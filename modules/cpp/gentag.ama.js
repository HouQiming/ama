'use strict'
const assert = require('assert');
let gentag = module.exports;

gentag.FindGenTag = function(nd_root, nd_pattern, want_all) {
	let ret = [];
	for (let nd_pragma of nd_root.FindAll(N_KEYWORD_STATEMENT, '#pragma')) {
		let nd_arg = nd_pragma.c;
		if (!nd_arg || nd_arg.node_class != N_CALL || nd_arg.c.node_class != N_REF) {continue;}
		if ((nd_arg.c.data == 'gen' || nd_arg.c.data == 'gen_begin') && nd_arg.c.s) {
			let match = nd_arg.c.s.Match(nd_pattern);
			if (match) {
				match.gentag = nd_pragma;
				if (want_all) {
					ret.push(match);
				} else {
					return match;
				}
			}
		}
	}
	if (want_all) {
		return ret;
	} else {
		return undefined;
	}
};

gentag.FindAllGenTags = function(nd_root, nd_pattern) {
	return gentag.FindGenTag(nd_root, nd_pattern, true);
};

gentag.UpdateGenTagContent = function(nd_gentag, nd_new) {
	assert(nd_gentag.c.node_class == N_CALL && nd_gentag.c.c.node_class == N_REF);
	if (nd_gentag.c.GetName() == 'gen_begin') {
		let nd_end = nd_gentag.s;
		let count = 0;
		while (nd_end) {
			if (nd_end.node_class == N_KEYWORD_STATEMENT && nd_end.data == '#pragma' && nd_end.c && nd_end.c.node_class == N_CALL) {
				if (nd_end.c.GetName() == 'gen_begin') {
					count += 1;
				} else if (nd_end.c.GetName() == 'gen_end') {
					count -= 1;
					if (count < 0) {break;}
				}
			}
			nd_end = nd_end.s;
		}
		if (nd_end) {
			if (!nd_new) {
				let nd_empty_gentag = nd_gentag.Clone();
				nd_empty_gentag.c.c.ReplaceWith(nRef('gen'));
				nd_gentag.ReplaceUpto(nd_end, nd_empty_gentag);
			} else {
				nd_gentag.s.ReplaceUpto(nd_end.Prev(), nd_new);
				nd_new.AutoFormat();
			}
			return;
		}
		//if we can't find an ending tag, assume it's broken and treat as non-begin 'gen'
	}
	nd_gentag.c.c.data = 'gen_begin';
	let nd_end = nd_gentag.Clone();
	nd_end.c.c.data = 'gen_end';
	if (nd_end.comments_before.length > 1) {
		nd_end.comments_before = '\n';
	}
	nd_gentag.Insert(POS_AFTER, nd_end);
	if (nd_new) {
		nd_gentag.Insert(POS_AFTER, nd_new);
		nd_new.AutoFormat();
	}
};

gentag.DropGeneratedCode = function(nd_root) {
	let all_gen_begins = [];
	for (let nd_pragma of nd_root.FindAll(N_KEYWORD_STATEMENT, '#pragma')) {
		let nd_arg = nd_pragma.c;
		if (!nd_arg || nd_arg.node_class != N_CALL || nd_arg.c.node_class != N_REF) {continue;}
		if (nd_arg.c.data == 'gen_begin' && nd_arg.c.s) {
			all_gen_begins.push(nd_pragma);
		}
	}
	for (let nd_gentag of all_gen_begins) {
		gentag.UpdateGenTagContent(nd_gentag, null);
	}
};
/*
#filter Generic inverse filter for things like `jsism.EnableJSON`
This filter replaces code inside:
```C++
#pragma gen_begin(foo)
#pragma gen_end(foo)
```
with
```C++
#pragma gen(foo)
```

We recommend custom filters that generate C++ to follow this convention to put generated code inside `gen_begin` and `gen_end`.
That allows the generated code to be cleared when synchronized back to `.ama.cpp` files.  
*/
gentag.GeneratedCode = function() {};
gentag.GeneratedCode.inverse = gentag.DropGeneratedCode;
