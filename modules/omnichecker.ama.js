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

function DeferJob(djobs, nd, job_nd) {
	let ret = djobs.get(nd);
	if (!ret) {
		ret = [];
		djobs.set(nd, ret);
	}
	ret.push(job_nd);
}

let g_ptn_lazychild = .(Sandbox.LazyChild(.(Node.MatchAny('ctx')), .(Node.MatchAny(N_STRING, 'name')), .(Node.MatchAny(N_STRING, 'addr'))));
function dfsGenerate(nd, options) {
	if (options.hook) {
		let nd_ret = options.hook(nd);
		if (nd_ret) {
			return nd_ret;
		}
	}
	for (let t of options.templates) {
		let match = nd.Match(t.pattern);
		if (match && (!t.filter || t.filter(match))) {
			//defer on-child flagging to child: options._deferred_jobs
			if (t.set) {
				for (let key in t.set) {
					if (!match[key]) {
						throw new Error(['invalid set-job: ', JSON.stringify(key), ' not found on `', match.nd.dump(), '`'].join(''));
					}
					DeferJob(options._deferred_jobs, match[key], {set: t.set[key]});
				}
			}
			if (t.check) {
				for (let key in t.check) {
					if (!match[key]) {
						throw new Error(['invalid check-job: ', JSON.stringify(key), ' not found on `', match.nd.dump(), '`'].join(''));
					}
					DeferJob(options._deferred_jobs, match[key], {check: t.check[key]});
				}
			}
		}
	}
	let nd_ret = dfsGenerateDefault(nd, options);
	let all_sets = undefined;
	let all_checks = undefined;
	for (let job_nd of (options._deferred_jobs.get(nd) || [])) {
		if (job_nd.set) {
			if (!all_sets) {all_sets = {};}
			Object.assign(all_sets, job_nd.set);
		} else if (job_nd.check) {
			if (!all_checks) {all_checks = [];}
			//Object.assign(all_checks, job_nd.check);
			all_checks.push(job_nd.check);
		}
	}
	if (all_sets) {
		//all_sets.__addr=nd.GetUniqueTag();
		nd_ret = .(Sandbox.SetProperties(.(nd_ret), .(ParseCode(JSON.stringify(all_sets)))));
	}
	if (all_checks) {
		nd_ret = .(Sandbox.CheckProperties(ctx, .(nd_ret), .(nString(nd.GetUniqueTag())), .(ParseCode(JSON.stringify(all_checks)))));
	}
	return nd_ret.setCommentsBefore(nd.comments_before);
}

function dfsGenerateDefault(nd, options) {
	if (nd.node_class == N_FUNCTION) {
		//we need to test the function with "default" bindings, as well as the non-default ones
		//calls become call validations
		let nd_paramlist = nd.c.s;
		assert(nd_paramlist.node_class == N_PARAMETER_LIST);
		//generate the testing code
		let param_names = [];
		let default_params = [];
		for (let ndi = nd_paramlist.c; ndi; ndi = ndi.s) {
			let nd_defroot = ndi;
			if (nd_defroot.node_class == N_ASSIGNMENT) {nd_defroot = nd_defroot.c;}
			let nd_def = FindDef(nd_defroot);
			if (nd_def) {
				param_names.push(nd_def.data);
			} else {
				param_names.push('');
			}
			default_params.push(defGenFlow(ndi, options));
		}
		//translate the body
		let nd_body = nd.LastChild();
		return .((function() {
			let f = function(ctx_outer, params) {
				let ctx = Sandbox.LazyChildScope(ctx_outer, .(nString(nd.GetUniqueTag())));
				let vars = ctx.vars;
				Sandbox.AssignMany(vars, .(ParseCode(JSON.stringify(param_names))), params);
				.(dfsGenerate(nd_body, options))/*no `;`*/
				return ctx;
			}
			f(ctx, .(nRaw.apply(null, default_params).setFlags(0x5D5B/*[]*/)));
			let value = Sandbox.FunctionValue(.(nString(nd.GetUniqueTag())), f.bind(null, ctx));
			return .(
				nd.GetName() ? 
					options.enable_operator_overloading ?
						.(Sandbox.MergePossibility(vars, .(nString(nd.GetName())), value)) :
						.(Sandbox.Assign(vars, .(nString(nd.GetName())), value)) : 
					.(value)
			);
		})());
	}
	if (nd.node_class == N_CLASS) {
		//fields, base class, resolve methods using or-on-Assign
		//all those are handled under nd_body
		let nd_body = nd.LastChild();
		return .(
			Sandbox.Assign(vars, .(nString(nd.GetName())), Sandbox.ClassValue(.(nString(nd.GetUniqueTag())), function(ctx_outer) {
				let ctx = Sandbox.LazyChildScope(ctx_outer, .(nString(nd.GetUniqueTag())));
				let vars = ctx.vars;
				.(dfsGenerate(nd_body, options))/*no `;`*/
				return ctx;
			}.bind(null, ctx)))
		);
	}
	if (nd.node_class == N_REF || nd.node_class == N_DOT && nd.c.node_class == N_AIR) {
		//LazyChild should work for parent-scope name resolution: LazyChildScope inherits the parent context
		//COULDDO: using scopes - create non-enumerable shadows
		if (nd.flags & REF_DECLARED) {
			//set the declared node too
			//we can't possibly have dots here
			return .(Sandbox.Declare(vars, .(nString(nd.GetName())), .(nString(nd.GetUniqueTag()))));
		} else {
			return .(Sandbox.LazyChild(vars, .(nString(nd.GetName())), .(nString(nd.GetUniqueTag()))));
		}
	}
	if (nd.node_class == N_DOT) {
		return .(Sandbox.LazyChild(Sandbox.LazyChild(.(dfsGenerate(nd.c, options)), 'vars'), .(nString(nd.GetName())), .(nString(nd.GetUniqueTag()))));
	}
	if (nd.node_class == N_ITEM) {
		//generate subscript values without using them: redundant args
		let subscripts = [];
		for (let ndi = nd.c.s; ndi; ndi = ndi.s) {
			subscripts.push(dfsGenerate(ndi, options));
		}
		return .(Sandbox.LazyChild(.(dfsGenerate(nd.c, options)), 'element', .(nString(nd.GetUniqueTag())), .(subscripts)));
	}
	if (nd.node_class == N_ASSIGNMENT) {
		let nd_lhs = dfsGenerate(nd.c, options);
		let nd_rhs = dfsGenerate(nd.c.s, options);
		let match = nd_lhs.Match(g_ptn_lazychild);
		if (match) {
			match.ctx.Unlink();
			match.name.Unlink();
			return .(Sandbox.Assign(.(match.ctx), .(match.name), .(nd_rhs), .(match.addr)));
		}
		//we don't understand it...
		//return nScope(nd_lhs, nd_rhs);
		return .(Sandbox.DummyValue(.(nString(nd.GetUniqueTag())), .(nd_lhs), .(nd_rhs)));
	}
	if (nd.node_class == N_CALL) {
		let children = [];
		for (let ndi = nd.c; ndi; ndi = ndi.s) {
			children.push(dfsGenerate(ndi, options));
		}
		return .(Sandbox.Call(ctx, .(nString(nd.GetUniqueTag())), .(nRaw.apply(null, children).setFlags(0x5d5b))));
	}
	if (nd.node_class == N_SCOPED_STATEMENT) {
		if (nd.data == 'if') {
			let nd_cond = dfsGenerate(nd.c, options);
			let nd_then = dfsGenerate(nd.c.s, options);
			let nd_else = nd.c.s.s ? dfsGenerate(nd.c.s.s, options) : nScope();
			//fork / join, treat the condition normally
			return .((function(ctx_if) {
				let ctx = ctx_if;
				let ctx_then = Sandbox.LazyCloneScope(ctx_if, .(nString(nd.GetUniqueTag())), 'in the then-clause', .(nd_cond));
				{
					let ctx = ctx_then;
					let vars = Sandbox.LazyChild(ctx, 'vars');
					.(nd_then)/*no `;`*/
				}
				let ctx_else = Sandbox.LazyCloneScope(ctx_if, .(nString(nd.GetUniqueTag())), 'in the else-clause');
				{
					let ctx = ctx_else;
					let vars = Sandbox.LazyChild(ctx, 'vars');
					.(nd_else)/*no `;`*/
				}
				Sandbox.MergeContext(ctx_if, [ctx_then, ctx_else]);
			})(ctx));
		}
		if (nd.GetCFGRole() == CFG_LOOP) {
			let children = [];
			let nd_loop_body = undefined;
			for (let ndi = nd.c; ndi; ndi = ndi.s) {
				if (ndi.node_class == N_SCOPE && !nd_loop_body) {
					nd_loop_body = ndi;
				} else {
					children.push(dfsGenerate(ndi, options));
				}
			}
			//loops: change to for-twice
			return .((function(ctx_loop) {
				{
					let ctx = ctx_loop;
					.(nScope.apply(null, children));
				}
				let iterations = [ctx_loop];
				for (let i = 0; i < 2; i++) {
					let ctx = Sandbox.LazyCloneScope(iterations[iterations.length - 1], .(nString(nd.GetUniqueTag())), i == 0 ? 'in the first iteration' : 'in subsequent iterations');
					let vars = Sandbox.LazyChild(ctx, 'vars');
					.(dfsGenerate(nd_loop_body, options))/*no `;`*/
					iterations.push(ctx);
				}
				Sandbox.MergeContext(ctx_loop, iterations);
			})(ctx));
		}
	}
	if (nd.node_class == N_KEYWORD_STATEMENT) {
		if (nd.data == 'return') {
			let nd_value = dfsGenerate(nd.c, options);
			return .(Sandbox.Assign(ctx, 'return', .(nd_value), .(nString(nd.GetUniqueTag()))));
		}
	}
	//just treat scopes and air as dummy values
	//it's just {} so no point recording, but we need {}
	//it's pointless to record node-level information at run time then associate it back with a node
	let children = [.(Sandbox.DummyValue), nString(nd.GetUniqueTag())];
	for (let ndi = nd.c; ndi; ndi = ndi.s) {
		children.push(dfsGenerate(ndi, options));
	}
	return nCall.apply(null, children);
}

omnichecker.RunGeneratedCode = function(nd_generated, options) {
	//pre-run the runtime lib in a sandboxed QuickJS runtime, passing back result as JSON
	let ret = __RunInSandbox(['(function(){\n', (options || {}).sandboxed_code || '', nd_generated.toSource(), '\n//\\""\\\'\'\\``*/\n})'].join(''));
	return ret && JSON.parse(ret);
}

//don't over-generalize: dedicated dataflow generator first
omnichecker.Check = function(nd_root, ...all_options) {
	let options = Object.create(null);
	options.default_value = {};
	options.templates = [];
	for (let options_i of all_options) {
		for (let key in options_i) {
			if (key == 'templates') {
				for (let t of options_i.templates) {
					options.templates.push(t);
				}
			} else if (key == 'default_value') {
				Object.assign(options.default_value, options_i.default_value);
			} else {
				options[key] = options_i[key];
			}
		}
	}
	if (options.enable_operator_overloading == undefined) {
		options.enable_operator_overloading = 1;
	}
	if (options.dump_errors == undefined) {
		options.dump_errors = 1;
	}
	if (options.colored == undefined) {
		options.colored = 1;
	}
	options._deferred_jobs = new Map();
	let nd_flowcode = dfsGenerate(nd_root, options);
	let ret = omnichecker.RunGeneratedCode(.({
		Sandbox.node_to_value = Object.create(null);
		Sandbox.default_value = .(ParseCode(JSON.stringify(options.default_value || {})));
		Sandbox.errors = [];
		let ctx = Object.create(null);
		ctx.utag = 0;
		ctx.utag_parent = -1;
		ctx.utag_addr = .(nString(nd_root.GetUniqueTag()));
		Sandbox.ctx_map.push(ctx);
		let vars = Sandbox.LazyChild(ctx, 'vars');
		.(nd_flowcode)/*no `;`*/
		return JSON.stringify({
			node_to_value: Sandbox.node_to_value,
			errors: Sandbox.errors,
			ctx_map: Sandbox.ctx_map
		});
	}), options);
	if (options.dump_errors) {
		for (let err of ret.errors) {
			let nd_loc = Node.GetNodeFromUniqueTag(err.addr);
			//origin tracking - ret.ctx_map
			for (let utag = err.origin; utag > 0; utag = ret.ctx_map[utag].utag_parent) {
				let ctx = ret.ctx_map[utag];
				let nd_here = Node.GetNodeFromUniqueTag(ctx.utag_addr);
				let msg = 'in';
				if (ctx.utag_clause != undefined) {
					msg = ctx.utag_clause;
				}
				console.write(nd_here.FormatFancyMessage(msg, options.colored ? MSG_COLORED : 0));
			}
			console.write(nd_loc.FormatFancyMessage(err.msg.replace(/\{code\}/g, nd_loc.dump()), options.colored ? MSG_COLORED | MSG_WARNING : MSG_WARNING));
		}
	}
	return ret;
}
