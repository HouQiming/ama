'use strict';
/*
#filter Extend C++ `for` syntax with `for(foo of bar)` and `for(i<foo)` 
Before:
```C++
int main() {
	std::vector<int> a;
	for(int i<10){
		a.push_back(i);
	}
	for(int i in a){
		std::cout<<i;
	}
	return 0;
}
```
*/
module.exports = function Transform(nd_root) {
	for (let nd_for of nd_root.FindAll(N_SCOPED_STATEMENT, 'for')) {
		let nd_range = nd_for.c;
		if (nd_range.node_class == N_PAREN || nd_range.node_class == N_LABELED) {
			nd_range = nd_range.c;
		}
		//for-in
		let nd_in_of = nd_range.Find(N_BINOP, 'in') || nd_range.Find(N_BINOP, 'of');
		if (nd_in_of) {
			nd_in_of.node_class = N_LABELED;
			nd_in_of.data = '';
		} else if (nd_range.node_class == N_BINOP && (nd_range.data == '<' || nd_range.data == '<=')) {
			let nd_i = nd_range.c;
			let nd_n = nd_range.c.s;
			let nd_tmp = Node.GetPlaceHolder();
			nd_for.c.ReplaceWith(nd_tmp);
			nd_range.BreakSibling();
			nd_tmp.ReplaceWith(@((auto @(nd_i.Clone()) = 0; @(nd_range); @(nd_i.Clone())++)));
			continue;
		} else if (nd_range.node_class == N_RAW && nd_range.c && nd_range.LastChild().node_class == N_BINOP && (nd_range.LastChild().data == '<' || nd_range.LastChild().data == '<=')) {
			let nd_cmp = nd_range.LastChild().Unlink()
			let nd_i = nd_cmp.c;
			let nd_n = nd_cmp.c.s;
			let nd_tmp = Node.GetPlaceHolder();
			nd_for.c.ReplaceWith(nd_tmp);
			nd_tmp.ReplaceWith(@((@(nd_range) @(nd_i.Clone()) = 0; @(nd_cmp); @(nd_i.Clone())++)));
			continue;
		}
		//parse for(:) as declaration
		if (nd_range.node_class == N_LABELED) {
			let nd_def = nd_range.c;
			if (nd_def.node_class == N_REF) {
				nd_def.flags |= REF_WRITTEN;
			} else if (nd_def.node_class == N_RAW && nd_def.c && nd_def.LastChild().node_class == N_REF) {
				nd_def.LastChild().flags |= REF_WRITTEN | REF_DECLARED;
			}
		}
	}
};
