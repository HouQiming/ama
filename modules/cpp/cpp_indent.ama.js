'use strict';
/*
#filter Indent-based scoping for C / C++ / Javascript 
Before:
```C++
int main()
	puts("hello world");
	return 0;
```
*/
module.exports = function(nd_root) {
	//Fix overzealously scoped C Pre-Processor commands
	//N_SCOPE inside N_KEYWORD_STATEMENT
	for (let nd_stmt of nd_root.FindAll(N_KEYWORD_STATEMENT)) {
		if (!nd_stmt.data.startsWith('#')) {continue;}
		let nd_scope = nd_stmt.c;
		let base_indent_level = nd_stmt.indent_level;
		if (nd_scope && nd_scope.node_class == N_RAW) {
			base_indent_level += nd_scope.indent_level;
			nd_scope = nd_scope.LastChild();
		}
		if (!(nd_scope && nd_scope.node_class == N_SCOPE)) {continue;}
		base_indent_level += nd_scope.indent_level;
		for (let ndi = nd_scope.c; ndi; ndi = ndi.s) {
			ndi.AdjustIndentLevel(base_indent_level);
		}
		nd_scope.indent_level = 0;
		nd_scope.Unlink();
		if (!nd_stmt.c) {
			nd_stmt.Insert(POS_BACK, nAir());
		}
		let nd_last = nd_scope.LastChild();
		if (nd_last.comments_after.endsWith('\n')) {
			nd_last.comments_after = nd_last.comments_after.substr(0, nd_last.comments_after.length - 1);
		}
		let nd_statements = nd_scope.BreakChild();
		nd_stmt.Insert(POS_AFTER, nd_statements);
	}
};
module.exports.setup = function(code, options) {
	if (code.indexOf('@' + 'disable_cpp_indent') >= 0) {return;}
	Object.assign(options, {parse_indent_as_scope: 1,auto_curly_bracket: 1,parse_indent_as_scope_but_merge_cpp_ctor_lines: 1});
};

