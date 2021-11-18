const assert = require('assert');
const typing = require('cpp/typing');
module.exports = {
	templates: [
		{
			pattern: @(&@(Node.MatchAny('foo'))),
			nd: (values, extra_args, vars, name, ctx)=> {
				let utag = vars['<utag>'];
				let other_pointers = Sandbox.FindValues(ctx.vars, value => {
					return value.ref_vars_utag == utag && value.ref_name == name;
				});
				if (other_pointers.length) {
					let errors = [];
					for (let value of other_pointers) {
						if (value.ref_mutable || extra_args.ref_mutable) {
							errors.push({
								value: value,
								value_message: ['`', name, '` is already borrowed ', value.ref_mutable ? 'mutably' : 'immutably'].join(''),
								error: ['and cannot be borrowed again ', extra_args.ref_mutable ? 'mutably' : 'immutably'].join('')
							});
						}
					}
					if (errors.length) {
						return errors;
					}
				}
				return values.map(v=>Sandbox.set(v, {
					ref_vars_utag: utag,
					ref_name: name,
					ref_mutable: extra_args.ref_mutable,
				}));
			},
			nd_extra_args: match=> {
				let ref_mutable = 1;
				//pass-to-const test
				//cast / assignment / call
				let type_test = undefined;
				let nd_parent = match.nd.p;
				while (nd_parent && nd_parent.node_class == N_PAREN) {
					nd_parent = nd_parent.p;
				}
				if (nd_parent && nd_parent.node_class == N_CLASS && !nd_parent.c.isAncestorOf(match.nd)) {
					//cast / call case
					let type_func = typing.ComputeType(nd_parent.c);
					if (type_func && type_func.node_class == N_FUNCTION) {
						let nd_paramlist = type_func.c.s;
						assert(nd_paramlist.node_class == N_PARAMETER_LIST);
						let ndi = nd_paramlist.c;
						let ndj = nd_parent.c.s;
						while (ndi && ndj && !ndj.isAncestorOf(match.nd)) {
							ndi = ndi.s;
							ndj = ndj.s;
						}
						if (ndi && ndj && ndi.node_class == N_ASSIGNMENT) {
							type_test = typing.ComputeType(ndi.c);
						}
					}
				}
				if (nd_parent && nd_parent.node_class == N_ASSIGNMENT && nd_parent.c.s.isAncestorOf(match.nd)) {
					//assignment case
					type_test = typing.ComputeType(nd_parent.c);
				}
				//rough const test
				while (type_test && (type_test.node_class == N_PREFIX || type_test.node_class == N_POSTFIX)) {
					if (type_test.data == 'const') {
						ref_mutable = 0;
						break;
					}
					type_test = type_test.c;
				}
				return {ref_mutable: ref_mutable};
			}
		}
	],
	destructors: [
		(values, vars, name) => {
			let utag = vars['<utag>'];
			let utag_parent = Sandbox.ctx_map[utag].utag_parent;
			if (utag_parent == undefined) {return;}
			let ctx_parent = Sandbox.ctx_map[utag_parent];
			let dangling_pointers = Sandbox.FindValues(ctx_parent.vars, value => {
				return value.ref_vars_utag == utag && value.ref_name == name;
			});
			if (dangling_pointers.length) {
				return dangling_pointers.map(value_ptr => {
					return {
						error: 'to `{code}`',
						value: value_ptr,
						value_message: 'found dangling pointer'
					}
				});
			}
		}
	]
};
