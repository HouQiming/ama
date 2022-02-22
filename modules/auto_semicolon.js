'use strict';
/*
#filter Automatically add ';' for C / C++ / Javascript. 
Before:
```C++
int main() {
	puts("hello world")
	return 0
}
```
*/
module.exports = function(nd_root) {
	for (let nd of nd_root.FindAll(N_RAW, null)) {
		if (nd.flags & 0xffff) {continue;}
		if (!nd.p || (nd.p.node_class !== N_SCOPE && nd.p.node_class !== N_FILE)) {continue;}
		if (!nd.c || !nd.c.s) {continue;}
		if (nd.Owning(N_NODEOF)) {continue;}
		//we are a N_RAW statement with at least 2 children
		let new_children = [nd.c];
		for (let ndi = nd.c.s; ndi; ndi = ndi.s) {
			if (ndi.comments_before.indexOf('\n') >= 0 && ndi.node_class !== N_SCOPE && ndi.indent_level === ndi.Prev().indent_level) {
				new_children.push(ndi);
			}
		}
		if (new_children.length > 1) {
			//we have multiple statements
			for (let i = new_children.length - 1; i >= 0; i--) {
				if (i > 0) {
					new_children[i].Prev().BreakSibling();
				} else if(new_children[i].p) {
					new_children[i].p.BreakChild();
				}
			}
			for (let i = 0; i < new_children.length; i++) {
				let ndi = new_children[i];
				let nd_test = ndi;
				while ((nd_test.node_class === N_SCOPED_STATEMENT || nd_test.node_class === N_KEYWORD_STATEMENT ||
				nd_test.node_class === N_EXTENSION_CLAUSE) && nd_test.c) {
					nd_test=nd_test.LastChild()
				}
				if (nd_test.node_class !== N_SCOPE && (ndi.node_class === N_KEYWORD_STATEMENT && !ndi.data.startsWith('#') || 
					ndi.node_class === N_ASSIGNMENT || 
					ndi.node_class === N_CALL ||
					ndi.node_class === N_POSTFIX ||
					ndi.node_class === N_PREFIX
				)) {
					ndi=ndi.toSingleNode();
					new_children[i] = nDelimited(ndi).setIndent(ndi.indent_level);
					ndi.indent_level=0;
				}
			}
			nd.ReplaceWith(nScope.apply(null, new_children).c);
		}
	}
	for (let nd of nd_root.FindAll(N_SCOPE, null).concat([nd_root])) {
		if (nd.Owning(N_NODEOF)) {continue;}
		if(nd.p&&nd.p.node_class===N_SCOPED_STATEMENT&&nd.p.data==='enum'){continue;}
		let found_semic=0;
		for(let ndi=nd.c;ndi;ndi=ndi.s){
			if(ndi.node_class===N_DELIMITED){
				found_semic=1;
				break;
			}
		}
		if(!found_semic){
			let nd_parent=nd;
			while(nd_parent&&(nd_parent.node_class===N_SCOPE||nd_parent.isRawNode(0,0))){
				nd_parent=nd_parent.p;
			}
			if(nd_parent&&nd_parent.node_class!==N_SCOPED_STATEMENT&&nd_parent.node_class!==N_FUNCTION&&nd_parent.node_class!==N_CLASS){
				//initializer-ish scope, don't auto-semicolon
				continue;
			}
		}
		//let ndi=nd.LastChild();
		//if(!ndi){continue;}
		for (let ndi = nd.c; ndi; ndi = ndi.s) {
			if (ndi.s && (ndi.s.isSymbol(',') || ndi.s.isSymbol(';'))) {continue;}
			let nd_test = ndi;
			while ((nd_test.node_class === N_SCOPED_STATEMENT || nd_test.node_class === N_KEYWORD_STATEMENT ||
			nd_test.node_class === N_EXTENSION_CLAUSE) && nd_test.c) {
				nd_test=nd_test.LastChild()
			}
			if (nd_test.node_class !== N_SCOPE && (ndi.node_class === N_KEYWORD_STATEMENT && !ndi.data.startsWith('#') || 
				ndi.node_class === N_ASSIGNMENT || 
				ndi.node_class === N_CALL
			)) {
				let nd_tmp = Node.GetPlaceHolder();
				ndi.ReplaceWith(nd_tmp);
				ndi = nd_tmp.ReplaceWith(nSemicolon(ndi));
				//ndi.c.AdjustIndentLevel(-ndi.indent_level);
				//shove trailing comments after the ;
				let all_comments = [];
				for (let ndj = ndi; ; ndj = ndj.LastChild()) {
					if (ndj.comments_after) {
						all_comments.push(ndj.comments_after);
					}
					if (ndj.node_class == N_SCOPE || ndj.node_class == N_OBJECT || ndj.node_class == N_ARRAY || 
					ndj.node_class == N_CALL || ndj.node_class == N_CALL_TEMPLATE || ndj.node_class == N_SCOPED_STATEMENT ||
					ndj.node_class == N_TYPED_OBJECT || !ndj.c) {break;}
				}
				if (all_comments.length > 0) {
					for (let ndj = ndi; ; ndj = ndj.LastChild()) {
						if (ndj.comments_after) {
							ndj.comments_after = '';
						}
						if (ndj.node_class == N_SCOPE || ndj.node_class == N_OBJECT || ndj.node_class == N_ARRAY || 
						ndj.node_class == N_CALL || ndj.node_class == N_CALL_TEMPLATE || ndj.node_class == N_SCOPED_STATEMENT ||
						ndj.node_class == N_TYPED_OBJECT || !ndj.c) {break;}
					}
					ndi.comments_after = all_comments.reverse().join('');
				}
			}
		}
	}
	return nd_root;
}
