'use strict';

function DeclScope(nd) {
	for (let ndi = nd; ndi; ndi = ndi.p) {
		if (ndi.node_class == N_SCOPE) {
			let nd_asgn = ndi.Owning(N_ASSIGNMENT);
			if (nd_asgn && nd_asgn.c.isAncestorOf(ndi)) {
				//hack out of destructuring
				return nd_asgn.Owning(N_SCOPE);
			}
			return ndi;
		}
		if (ndi.node_class == N_PARAMETER_LIST) {
			return ndi.p;
		}
	}
	return nd.Root();
}

/*
#filter Automatically resolve undeclared names.

Each undeclared variable will be auto-declared on first assignment.
If it's never assigned, this filter will search for global names in all available namespaces.
If still not found, this filter will search for a similarly-named variable at call sites of the current function and pass it in as a newly-declared parameter.

Before:
```C++
namespace cns{
  int c=0; 
};

int test(){
  return b+c;
}

int main(int argc){
  a=42;
  if(argc>=2){
    b=0;
    a=b+c;
    a+=test();
  }else{
    a+=100;
  }
  return a;
}
```
*/
function Translate(nd_root, options) {
	let all_refs = nd_root.FindAll(N_REF, null);
	//track the locally undeclared
	let scope_to_context = new Map();
	for (let nd_def of all_refs) {
		if (!(nd_def.flags & REF_DECLARED)) {continue;}
		let nd_scope = DeclScope(nd_def);
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
		let nd_owning_kwstmt = nd_ref.Owning(N_KEYWORD_STATEMENT);
		if (nd_owning_kwstmt && nd_owning_kwstmt.data == 'using') {continue;}
		let declared = 0;
		for (let nd_scope = nd_ref; nd_scope; nd_scope = nd_scope.p) {
			let ctx = scope_to_context.get(nd_scope);
			if (ctx && ctx.defs.has(nd_ref.data)) {declared = 1;break;}
		}
		if (!declared) {
			locally_undeclared.push(nd_ref);
		}
	}
	//worse is better: just make each first assignment decl, then look up the never-writtens
	let owner_to_context = new Map();
	let keyword = (options || {}).keyword || 'auto';
	for (let nd_ref of locally_undeclared) {
		if (nd_ref.flags == REF_WRITTEN) {
			//check already-declared-ness
			let declared = 0;
			for (let nd_scope = nd_ref; nd_scope; nd_scope = nd_scope.p) {
				let ctx = scope_to_context.get(nd_scope);
				if (ctx && ctx.defs.has(nd_ref.data)) {declared = 1;break;}
			}
			if (declared) {continue;}
			//make it a declaration
			let nd_tmp = Node.GetPlaceHolder();
			nd_ref.ReplaceWith(nd_tmp)
			nd_tmp.ReplaceWith(nRaw(nRef(keyword).setCommentsAfter(' '), nd_ref));
			nd_ref.flags |= REF_DECLARED;
			///////////
			//add it to the declarations
			let nd_scope = DeclScope(nd_ref);
			let ctx = scope_to_context.get(nd_scope);
			if (!ctx) {
				ctx = {defs: new Set()};
				scope_to_context.set(nd_scope, ctx);
			}
			ctx.defs.add(nd_ref.data);
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
	let auto_create_params = (options || {}).auto_create_params;
	if (auto_create_params == undefined) {auto_create_params = 1;}
	let keywords_class = new Set(((options || default_options).keywords_class || default_options.keywords_class).split(' '));
	let all_possible_names = undefined;
	for (let nd_ref of locally_undeclared) {
		if (nd_ref.flags & (REF_WRITTEN | REF_DECLARED)) {continue;}
		let nd_owner = nd_ref.Owner();
		let ctx = owner_to_context.get(nd_owner);
		if (ctx && ctx.writtens.has(nd_ref.data)) {continue;}
		if (nd_ref.p) {
			//check a few should-not-fix cases
			if (nd_ref.p.node_class == N_KEYWORD_STATEMENT && nd_ref.p.c == nd_ref && keywords_class.has(nd_ref.p.data)) {
				//struct forward decl
				continue;
			}
		}
		if (!all_possible_names) {
			//collect names
			const depends = require('depends');
			all_possible_names = new Map();
			for (let nd_root_i of depends.ListAllDependency(nd_root, true)) {
				for (let ndi = nd_root_i; ndi; ndi = ndi.PreorderNext(nd_root_i)) {
					if (ndi.node_class == N_SCOPE && ndi.p && (ndi.p.node_class == N_FUNCTION || ndi.p.node_class == N_CLASS && ndi.p.data != 'namespace')) {
						ndi = ndi.PreorderSkip();
						continue;
					}
					if (ndi.node_class == N_PARAMETER_LIST) {
						ndi = ndi.PreorderSkip();
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
		} else if (nd_owner.node_class == N_FUNCTION && nd_owner.GetName()) {
			//auto-param: check for the name at all call sites
			let name = nd_owner.GetName();
			let call_sites = nd_root.FindAll(N_CALL, name);
			let all_found = 1;
			for (let nd_call_site of call_sites) {
				let declared = 0;
				for (let nd_scope = nd_call_site; nd_scope; nd_scope = nd_scope.p) {
					let ctx = scope_to_context.get(nd_scope);
					if (ctx && ctx.defs.has(nd_ref.data)) {
						declared = 1;
						break;
					}
					if (nd_scope.node_class == N_FUNCTION || nd_scope.node_class == N_CLASS) {
						break;
					}
				}
				if (!declared) {
					all_found = 0;
					break;
				}
			}
			if (call_sites.length > 0 && all_found) {
				//the name is available at all call sites, create new parameter
				let defs = call_sites[0].Owner().FindAll(N_REF, nd_ref.data).filter(nd_def => (nd_def.flags & REF_DECLARED));
				if (defs.length > 0) {
					let nd_def = defs[0];
					if (nd_def.p.node_class == N_RAW) {
						nd_def = nd_def.p;
					}
					let nd_paramlist = nd_owner.c.s;
					if (nd_paramlist.FindAll(N_REF, nd_ref.data).filter(nd_def => (nd_def.flags & REF_DECLARED)).length > 0) {
						//dedup
						continue;
					}
					nd_paramlist.Insert(POS_BACK, nAssignment(nd_def.Clone(), nAir()));
				}
				for (let nd_call of call_sites) {
					nd_call.Insert(POS_BACK, nRef(nd_ref.data));
				}
			}
		}
	}
}

module.exports = Translate;
