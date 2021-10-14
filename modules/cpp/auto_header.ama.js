'use strict';
//@ama ParseCurrentFile().then(require("jcs").TranslateJCS)
const path = require('path');
const fs = require('fs');
const depends = require('depends');

function GetFunctionNameNode(nd_func) {
	let nd_name = nd_func.c;
	if (nd_name.node_class == N_RAW) {
		nd_name = nd_name.LastChild();
	}
	if (nd_name && nd_name.node_class != N_REF && nd_name.node_class != N_DOT) {
		return undefined;
	}
	return nd_name;
}

function Transform(nd_root, options) {
	let header_file = (options || {}).header_file;
	if (!header_file) {
		//search for same-name dependency first
		let my_name = path.parse(nd_root.data).name;
		for (let ndi of nd_root.FindAll(N_DEPENDENCY, null)) {
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
		header_file = nd_root.data;
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
		//reject methods of private classes
		let nd_class = nd_func.Owning(N_CLASS);
		if (nd_class && nd_class.data != 'namespace') {continue;}
		if (!GetFunctionNameNode(nd_func)) {continue;}
		//gotta sync to header
		to_sync.push(nd_func);
	}
	//fast path: skip header loading if we don't have anything to sync
	if (!to_sync.length) {return;}
	//load the header
	let nd_header = undefined;
	if (!fs.existsSync(header_file)) {
		let header_name = path.parse(header_file).name;
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
	let endifs = nd_header.FindAll(N_REF, '#endif');
	let nd_endif = undefined;
	if (endifs.length) {
		nd_endif = endifs.pop();
	}
	let changed = 0;
	for (let nd_func of to_sync) {
		let nd_name = GetFunctionNameNode(nd_func);
		let names = [];
		while (nd_name.node_class == N_DOT) {
			names.push(nd_name.data)
			nd_name = nd_name.c;
		};
		if (nd_name.node_class != N_AIR) {
			names.push(nd_name.GetName());
		}
		if (!names.length) {continue;}
		let nd_scope = nd_header;
		let failed = 0;
		for (let i = names.length - 1; i > 0; i--) {
			let classes = nd_scope.FindAll(N_CLASS, names[i]).filter(nd=>nd.LastChild().node_class == N_SCOPE);
			if (classes.length > 0) {
				nd_scope = classes[0].LastChild();
			} else {
				failed = i;
				break;
			}
		}
		if (!failed && nd_scope.Find(N_FUNCTION, names[0])) {
			//we already have it, no need to sync
			//ignore overloading cases for now
			continue;
		}
		//assume all ::-s along the way are namespaces
		//TODO: make sure we insert before the last #endif
		changed = 1;
		let nd_insertion_root = undefined;
		for (let i = failed; i > 0; i--) {
			let nd_namespace = nClass('namespace', nAir(), nRef(names[i]), nAir(), nScope());
			if (!nd_insertion_root) {
				nd_insertion_root = nd_namespace;
			}
			nd_scope.Insert(POS_BACK, nd_namespace);
			nd_scope = nd_namespace.LastChild();
		}
		let nd_forward = nd_func.Clone().setCommentsBefore('').setCommentsAfter('');
		GetFunctionNameNode(nd_forward).ReplaceWith(nRef(names[0]).setCommentsBefore(' '))
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
		nd_endif.Unlink();
		nd_header.Insert(POS_BACK, nd_endif);
		if (options.audit) {
			nd_header.Save({name: options.audit});
		} else {
			nd_header.Save();
		}
	}
}

module.exports = Transform;
