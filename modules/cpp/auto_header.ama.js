'use strict';
const path = require('path');
const fs = require('fs');
const depends = require('depends');
const typing = require('cpp/typing');

/*
#filter Synchronize methods and functions to classes and headers

If you `#include "./foo.hpp"` in `foo.cpp`, this filter will also add function forward declarations to `foo.hpp`.
Use `#pragma no_auto_header()` to suppress this behavior.

Before:
```
struct TestClass{
	int a;
};
int TestClass::get_a(){
	return this->a;
}
void TestClass::set_a(int a){
	this->a = a;
}
```
*/
function Translate(nd_root, options) {
	if (path.extname(nd_root.data).startsWith('.h')) {return;}
	if (nd_root.Find(N_CALL, 'no_auto_header')) {return;}
	let header_file = (options || {}).header_file;
	if (!header_file && nd_root.data) {
		//search for same-name dependency first
		let my_name = path.parse(nd_root.data).name;
		for (let ndi of nd_root.FindAll(N_DEPENDENCY)) {
			if ((ndi.flags & DEP_TYPE_MASK) == DEP_C_INCLUDE && ( ndi.flags & DEPF_C_INCLUDE_NONSTR )) {
				continue;
			}
			let fn_dep = depends.Resolve(ndi);
			if (fn_dep && path.parse(fn_dep).name == my_name) {
				header_file = fn_dep;
				break;
			}
		}
	}
	if (!header_file) {
		header_file = nd_root.data || '';
		let pdot = header_file.lastIndexOf('.');
		if (pdot >= 0) {
			header_file = header_file.substr(0, pdot);
		}
		header_file = header_file + '.hpp';
	}
	//search for method implementation and globally exported functions
	let to_sync = [];
	for (let nd_func of nd_root.FindAll(N_FUNCTION, null)) {
		//before, param, after, scope
		//reject forward declarations
		if (nd_func.LastChild().node_class != N_SCOPE) {continue;}
		//reject private declarations
		if (nd_func.c.Find(N_REF, 'static')) {continue;}
		//reject in-class methods of private classes
		let nd_class = nd_func.Owning(N_CLASS);
		if (nd_class && nd_class.data != 'namespace') {continue;}
		if (!nd_func.GetFunctionNameNode()) {continue;}
		//gotta sync to header
		to_sync.push(nd_func);
	}
	//fast path: skip header loading if we don't have anything to sync
	if (!to_sync.length) {return;}
	//load the header
	let nd_header = undefined;
	if (!fs.existsSync(header_file)) {
		let header_name = path.parse(header_file).name.replace(/[^a-zA-Z0-9]+/g, '_').toUpperCase();
		nd_header = ParseCode([
			'#ifndef __', header_name, '_HEADER\n',
			'#define __', header_name, '_HEADER\n',
			'#endif\n'
		].join(''));
		nd_header.data = header_file;
	} else {
		nd_header = depends.LoadFile(header_file);
	}
	//sync to header
	let endifs = nd_header.FindAll(N_KEYWORD_STATEMENT, '#endif');
	let nd_endif = undefined;
	if (endifs.length) {
		nd_endif = endifs.pop();
	}
	let changed = 0;
	for (let nd_func of to_sync) {
		let nd_name = nd_func.GetFunctionNameNode();
		let names = [];
		while (nd_name.node_class == N_DOT) {
			names.push(nd_name.data);
			nd_name = nd_name.c;
		};
		if (nd_name.node_class != N_AIR) {
			names.push(nd_name.GetName());
		}
		let class_names = [];
		for (let ndi = nd_name.p; ndi; ndi = ndi.p) {
			if (ndi.node_class == N_CLASS) {
				class_names.push(ndi.GetName());
			}
		}
		//namespace deduplication
		if (names.length > 1) {
			let inner_starting_class = names[names.length - 1];
			let p_class_names = class_names.indexOf(inner_starting_class);
			if (p_class_names >= 0) {
				class_names.splice(0, p_class_names + 1);
			}
		}
		for (let i = 0; i < class_names.length; i++) {
			names.push(class_names[i]);
		}
		if (!names.length) {continue;}
		//look up in all possible namespaces
		let scopes = [nd_header];
		if (names.length > 1) {
			//finding in ANY dependent file prevents the sync
			scopes = typing.LookupClassesByNames(nd_header, names.slice(1), {include_dependency: 1});
		}
		let found = 0;
		for (let nd_scope of scopes) {
			//constructor forwards are found as calls
			if (nd_scope.Find(N_FUNCTION, names[0]) || nd_scope.Find(N_CALL, names[0])) {
				found = 1;
				break;
			}
		}
		if (!found) {
			//check for out-of-class implementation of a private class
			let body_scopes = typing.LookupClassesByNames(nd_root, names.slice(1), {});
			for (let nd_scope of body_scopes) {
				let nd_class = nd_scope.Owning(N_CLASS);
				if (nd_class && nd_class.data != 'namespace') {
					//use found=1 to prevent headersyncing
					found = 1;
					//sync to that private class instead
					if (!nd_scope.Find(N_FUNCTION, names[0]) && !nd_scope.Find(N_CALL, names[0])) {
						let nd_forward = nd_func.Clone().setCommentsBefore('').setCommentsAfter('');
						nd_forward.GetFunctionNameNode().ReplaceWith(nRef(names[0]).setCommentsBefore(' '))
						nd_forward.LastChild().ReplaceWith(nAir()).setCommentsBefore('').setCommentsAfter('');
						if (nd_forward.c.s.s.node_class == N_LABELED) {
							//C++ constructor
							nd_forward.c.s.s.ReplaceWith(nAir());
						}
						nd_forward = nSemicolon(nd_forward);
						nd_scope.Insert(POS_BACK, nd_forward);
						nd_forward.AutoFormat();
					}
					break;
				}
			}
		}
		if (found) {
			//we already have it, no need to sync
			//ignore overloading cases for now
			continue;
		}
		//assume all ::s along the way are namespaces
		//recreate them
		let failed = 0;
		while (failed < names.length) {
			failed += 1;
			scopes = (failed >= names.length ? [nd_header] : typing.LookupClassesByNames(nd_header, names.slice(failed), {}));
			if (scopes.length) {break;}
		}
		let nd_scope = scopes[0];
		changed = 1;
		let nd_insertion_root = undefined;
		for (let i = failed - 1; i > 0; i--) {
			let nd_namespace = nClass('namespace', nAir(), nRef(names[i]), nAir(), nScope());
			if (!nd_insertion_root) {
				nd_insertion_root = nd_namespace;
			}
			nd_scope.Insert(POS_BACK, nd_namespace);
			nd_scope = nd_namespace.LastChild();
		}
		let nd_forward = nd_func.Clone().setCommentsBefore('').setCommentsAfter('');
		nd_forward.GetFunctionNameNode().ReplaceWith(nRef(names[0]).setCommentsBefore(' '))
		nd_forward.LastChild().ReplaceWith(nAir()).setCommentsBefore('').setCommentsAfter('');
		nd_forward = nSemicolon(nd_forward);
		if (!nd_insertion_root) {
			nd_insertion_root = nd_forward;
		}
		nd_scope.Insert(POS_BACK, nd_forward);
		nd_insertion_root.AutoFormat();
	}
	if (changed) {
		//place #endif to EOF again
		if (nd_endif) {
			nd_endif.Unlink();
			nd_header.Insert(POS_BACK, nd_endif);
		}
		if (options && options.audit) {
			nd_header.Save({name: options.audit, full_path: options.audit});
		} else {
			nd_header.Save();
		}
	}
}

module.exports = Translate;
