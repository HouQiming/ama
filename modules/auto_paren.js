'use strict';

/*
#filter Enable Python-style `if foo:` / `for foo:` / ... in C / C++ / Javascript.
This filter will add '()' and `{}` automatically.
Before:
```C++
int main() {
	if rand()&1:{
		puts("rand()&1");
	}
	for int i=0;i<10;i++
		printf("iteration %d\n",i);
	return 0;
}
```
*/
function Translate(nd_root) {
	let all_scoped=nd_root.FindAll(N_SCOPED_STATEMENT, null);
	for (let ndi of all_scoped) {
		if(ndi.data==='template'){continue;}
		if(ndi.data==='for'&&ndi.c.node_class === N_AIR&&ndi.c.s.node_class!==N_AIR){
			//relaxed for syntax: `for i=0;i<n;i++{...}`
			if(ndi.p&&ndi.p.isRawNode(0,0)){
				let nd_container=ndi.p;
				let nd_init=ndi.c.s;
				if(nd_init.node_class==N_SCOPE){nd_init=nd_init.c||nAir();}
				nd_body=nd_container.LastChild();
				if(nd_body.node_class===N_SCOPE){
					if(!nd_init.s&&nd_init.node_class===N_SEMICOLON){nd_init=nd_init.c;}
					ndi.ReplaceWith(nd_init.Unlink());
					nd_body.Unlink();
					let nd_tmp=Node.GetPlaceHolder();
					nd_container.ReplaceWith(nd_tmp);
					nd_container.flags=0x2928;//()
					nd_tmp.ReplaceWith(nScopedStatement('for',nd_container,nd_body));
					continue;
				}
			}else if(ndi.s&&ndi.s.s){
				//we don't have cpp/cpp_indent
				let nd_cond=ndi.s;
				let nd_update_and_body=ndi.s.s;
				let need_semic=0;
				if(nd_update_and_body.node_class===N_SEMICOLON&&nd_update_and_body.c.node_class===N_RAW&&nd_update_and_body.c.c&&nd_update_and_body.c.c.s){
					nd_update_and_body=nd_update_and_body.ReplaceWith(nd_update_and_body.BreakChild());
					need_semic=1;
				}
				if(nd_update_and_body.node_class===N_RAW&&nd_update_and_body.c&&nd_update_and_body.c.s){
					let nd_update=ndi.s.s;
					let nd_init=ndi.c.s;
					if(nd_init.node_class==N_SCOPE){nd_init=nd_init.c||nAir();}
					ndi.c.s.ReplaceWith(nAir());
					let nd_tmp=Node.GetPlaceHolder();
					ndi.ReplaceUpto(nd_update_and_body,nd_tmp);
					ndi.c.ReplaceWith(nRaw(nd_init.Unlink(),ndi.BreakSibling()).setFlags(0x2928));
					nd_update.ReplaceWith(nd_update_and_body.c.Unlink());
					if(!nd_update_and_body.c.s){nd_update_and_body=nd_update_and_body.c;}
					if(need_semic){
						nd_update_and_body=nSemicolon(nd_update_and_body);
					}
					ndi.c.s.ReplaceWith(nd_update_and_body);
					nd_tmp.ReplaceWith(ndi);
				}
			}
		}
		if (ndi.c.node_class !== N_PAREN && !ndi.c.isRawNode('(', ')') && ndi.c.node_class !== N_AIR) {
			let nd_arg = ndi.c;
			let nd_tmp = Node.GetPlaceHolder()
			nd_arg.ReplaceWith(nd_tmp);
			if (nd_arg.node_class === N_LABELED && nd_arg.c.s.node_class === N_AIR) {
				nd_arg = nd_arg.c;
			}
			nd_tmp.ReplaceWith(nParen(nd_arg));
		}
	}
	//auto-scope
	for (let ndi of all_scoped.concat(nd_root.FindAll(N_EXTENSION_CLAUSE, null))) {
		if(ndi.data==='template'){continue;}
		if(ndi.c.s&&ndi.c.s.node_class!==N_SCOPE&&ndi.c.s.node_class!==N_AIR){
			if(ndi.data=='else'&&ndi.c.s.node_class==N_SCOPED_STATEMENT&&ndi.c.s.data=='if'){
				continue;
			}
			let nd_tmp = Node.GetPlaceHolder();
			let nd_body=ndi.c.s;
			nd_body.SanitizeCommentPlacement();
			nd_body.ReplaceWith(nd_tmp);
			let nd_scoped_body = nScope(nd_body);
			nd_tmp.ReplaceWith(nd_scoped_body);
			nd_body.comments_before=nd_scoped_body.comments_before;nd_scoped_body.comments_before='';
			nd_body.comments_after=nd_scoped_body.comments_after;nd_scoped_body.comments_after='';
			nd_body.AdjustIndentLevel(nd_scoped_body.indent_level);
			if(nd_body.comments_before.indexOf('\n')>=0){
				nd_body.comments_after=nd_body.comments_after+'\n';
			}
			nd_scoped_body.indent_level=0;
		}
	}
}

module.exports = Translate;
