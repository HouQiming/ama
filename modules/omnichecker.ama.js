'use strict';
let omnichecker = module.exports;
const assert = require('assert');

function FindDef(nd_defroot) {
	for (let ndj = nd_defroot; ndj; ndj = ndj.PreorderNext(nd_defroot)) {
		if (ndj.node_class == N_REF && (ndj.flags & REF_DECLARED)) {
			return ndj;
		}
	}
	return undefined;
}

function dfsGenFlow(nd, options) {
	if (nd.node_class == N_SCOPE || nd.node_class == N_FILE) {
		let children = [];
		for (let ndi = nd.c; ndi; ndi = ndi.s) {
			children.push(dfsGenFlow(ndi, options));
		}
		return nScope.apply(null, children);
	}
	if (nd.node_class == N_AIR) {
		return nAir();
	}
	if (nd.node_class == N_FUNCTION) {
		//TODO: parameters all become variable-descs
		//we need to test the function with "default" bindings, as well as the non-default ones
		//calls become call validations
		let nd_paramlist = nd.c.s;
		assert(nd_paramlist.node_class == N_PARAMETER_LIST);
		//generate the testing code
		let param_names = [];
		let default_test_params = [];
		for (let ndi = nd_paramlist.c; ndi; ndi = ndi.s) {
			let nd_defroot = ndi;
			if (nd_defroot.node_class == N_ASSIGNMENT) {nd_defroot = nd_defroot.c;}
			let nd_def = FindDef(nd_defroot);
			if (nd_def) {
				if (options.GenerateDeclaredValue) {
					default_test_params.push(options.GenerateDeclaredValue(nd_def));
				}
				param_names.push(nd_def.data);
			} else {
				param_names.push('');
			}
		}
		//translate the body
		let nd_body = nd.LastChild();
		return .({
			let f = function(ctx_outer, params) {
				let ctx = Sandbox.LazyChild(ctx_outer, .(nString(nd.GetUniqueTag())));
				let vars = Sandbox.LazyChild(ctx, 'vars');
				Sandbox.AssignMany(vars, .(ParseCode(JSON.stringify(param_names))), params);
				.(dfsGenFlow(nd_body, options))/*no `;`*/
			}
			f(ctx, .(nRaw.apply(null, default_test_params).setFlags(0x5D5B/*[]*/)));
			Sandbox.Assign(vars, .(nString(nd.GetName())), {as_function: f});
		});
	}
	if (nd.node_class == N_CLASS) {
		//fields, base class, resolve methods using or-on-Assign
		//all those are handled under nd_body
		let nd_body = nd.LastChild();
		return .({
			let f = function(ctx_outer, params) {
				let ctx = Sandbox.LazyChild(ctx_outer, .(nString(nd.GetUniqueTag())));
				let vars = Sandbox.LazyChild(ctx, 'vars');
				.(dfsGenFlow(nd_body, options))/*no `;`*/
			}
			Sandbox.Assign(vars, .(nString(nd.GetName())), {as_class: f});
		});
	}
	if (nd.node_class == N_SCOPED_STATEMENT) {
		if (nd.data == 'if') {
			//TODO: fork / join
		}
		//TODO: LazyClone
	}
	//other stuff: find def and ref and eligible children
	//TODO: how do we deal with C++ types? options.GenerateDeclaredValue
	//we eventually need to resolve methods though
	//defer the method resolution to run-time: we still have the type tag
}

omnichecker.RunGeneratedCode = function(nd_generated, options) {
	//pre-run the runtime lib in a sandboxed QuickJS runtime, passing back result as JSON
	let ret = __RunInSandbox(['function(){\n', (options || {}).sandboxed_code || '', nd_generated.toSource(), '\n//\\""\\\'\'\\``*/\n}'].join(''));
	return ret && JSON.parse(ret);
}

//TODO: also develop the runtime lib
//TODO: don't over-generalize: dedicated dataflow generator first
//TODO: use the old JC3 convention that def carries value?
//again, runtime resolution of out-of-the-blue fields
omnichecker.GenerateDataflow = function(nd_root, options) {
	//options=Object.create(options||null);
	let nd_flowcode = dfsGenFlow(nd_root, options);
	let df_tree = omnichecker.RunGeneratedCode(.({
		let ctx = Object.create(null);
		let vars = Sandbox.LazyChild(ctx, 'vars');
		.(nd_flowcode)/*no `;`*/
		return JSON.stringify(ctx);
	}), options);
	//TODO: map names in g_dataflow back to node
}

//TODO: C++ options: {GenerateDeclaredValue}
