'use strict';

function Transform(nd_root, options) {
	let all_refs = nd_root.FindAll(N_REF, null);
	//track the locally undeclared
	let scope_to_context = new Map();
	for (let nd_def of all_refs) {
		if (!(nd_def.flags & REF_DECLARED)) {continue;}
		let nd_scope = nd_def.Owning(N_SCOPE) || nd_root;
		let nd_asgn = nd_def.Owning(N_ASSIGNMENT);
		if (nd_asgn && nd_asgn.c.isAncestorOf(nd_scope)) {
			//hack out of destructuring
			nd_scope = nd_asgn.Owning(N_SCOPE);
		}
		let ctx = scope_to_context.get(nd_scope);
		if (!ctx) {
			ctx = {defs: new Set()};
			scope_to_context.set(nd_scope, ctx);
		}
		ctx.defs.add(nd_def.data);
	}
	//find the locally undeclared: then declare them or do name search
	let locally_undeclared = [];
	for (let nd_ref of all_refs) {
		if (nd_ref.flags & REF_DECLARED) {continue;}
		let declared = 0;
		for (let nd_scope = nd_ref; nd_scope; nd_scope = nd_scope.p) {
			let ctx = scope_to_context.get(nd_scope);
			if (ctx && ctx.defs.has(nd_ref.data)) {declared = 1;break;}
		}
		if (!declared) {
			locally_undeclared.push(nd_ref);
		}
	}
	//worse is better: just make all assignments decl, then look up the never-writtens
	let owner_to_context = new Map();
	let keyword = (options || {}).keyword || 'auto';
	for (let nd_ref of locally_undeclared) {
		if (nd_ref.flags == REF_WRITTEN) {
			//make it a declaration
			let nd_tmp = Node.GetPlaceHolder();
			nd_ref.ReplaceWith(nd_tmp)
			nd_tmp.ReplaceWith(nRaw(nRef(keyword).setCommentsAfter(' '), nd_ref));
		}
		if (nd_ref.flags & REF_WRITTEN) {
			let nd_owner = nd_ref.Owner();
			let ctx = owner_to_context.get(nd_owner);
			if (!ctx) {
				ctx = {writtens: new Set()};
				owner_to_context.set(nd_owner, ctx);
			}
			ctx.writtens.add(nd_ref.data);
		}
	}
	//look up the never-writtens
	let all_possible_names = undefined;
	for (let nd_ref of locally_undeclared) {
		if (nd_ref.flags & REF_WRITTEN) {continue;}
		let nd_owner = nd_ref.Owner();
		let ctx = owner_to_context.get(nd_owner);
		if (ctx && ctx.writtens.has(nd_ref.data)) {continue;}
		if (!all_possible_names) {
			//collect names
			const depends = require('depends');
			all_possible_names = new Map();
			for (let nd_root_i of [nd_root].concat(depends.ListAllDependency(nd_root, true).map(fn=>depends.LoadFile(fn)))) {
				for (let ndi = nd_root_i; ndi; ndi = ndi.PreorderNext(nd_root_i)) {
					if (ndi.node_class == N_SCOPE && ndi.p && (ndi.p.node_class == N_FUNCTION || ndi.p.node_class == N_CLASS && ndi.p.data != 'namespace')) {
						ndi = ndi.PreorderLastInside();
						continue;
					}
					if (ndi.node_class == N_REF && (ndi.flags & REF_DECLARED)) {
						let defs = all_possible_names.get(ndi.data);
						if (!defs) {
							defs = [];
							all_possible_names.set(ndi.data, defs);
							defs.push(ndi);
						}
					}
				}
			}
		}
		let defs = all_possible_names.get(nd_ref.data);
		if (defs && defs.length == 1) {
			//resolve it
			let names = [];
			let nd_def = defs[0];
			for (let ndi = nd_def; ndi; ndi = ndi.p) {
				if (ndi.node_class == N_CLASS && nd_def != ndi.c.s || ndi.node_class == N_REF) {
					names.push(ndi.GetName());
				}
			}
			let nd_ret = nRef(names.pop());
			while (names.length > 0) {
				nd_ret = nd_ret.dot(names.pop()).setFlags(DOT_CLASS);
			};
			nd_ref.ReplaceWith(nd_ret);
		}
	}
}

module.exports = Transform;
