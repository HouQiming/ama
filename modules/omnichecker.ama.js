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

let g_ptn_readname = .(Sandbox.ReadName(.(Node.MatchAny('ctx')), .(Node.MatchAny(N_STRING, 'name')), .(Node.MatchAny(N_STRING, 'addr'))));
let g_ptn_declare = .(Sandbox.Declare(.(Node.MatchAny('ctx')), .(Node.MatchAny(N_STRING, 'name')), .(Node.MatchAny(N_STRING, 'addr'))));
function dfsGenerate(nd, options) {
	if (options.hook) {
		let nd_ret = options.hook(nd);
		if (nd_ret) {
			return nd_ret;
		}
	}
	let actions = [];
	for (let i = 0; i < options.templates.length; i++) {
		let t = options.templates[i];
		let match = nd.Match(t.pattern);
		if (match && (!t.filter || t.filter(match))) {
			for (let key in t) {
				if (key == 'pattern' || key == 'filter' || key.endsWith('_extra_args')) {continue;}
				if (!match[key]) {
					throw new Error(['invalid action: ', JSON.stringify(key), ' not found on `', match.nd.dump(), '`'].join(''));
				}
				let key_extra_args = key + '_extra_args';
				let nd_extra_args = t[key_extra_args] ? t[key_extra_args](match) : nNumber('0');
				DeferJob(options._deferred_jobs, match[key], {action: t[key],nd_extra_args: nd_extra_args,priority: i});
			}
		}
	}
	let nd_ret = dfsGenerateDefault(nd, options);
	let cb_dedup = new Set();
	let call_wrapped = 0;
	let jobs = (options._deferred_jobs.get(nd) || []);
	jobs.sort((a, b)=>a.priority - b.priority);
	for (let job_nd of jobs) {
		let action = job_nd.action;
		let nd_extra_args = job_nd.nd_extra_args;
		if (typeof(action) == 'function') {action = action.toString();}
		let name = options._named_callbacks.get(action);
		if (!name) {
			name = '_cb' + options._callback_name.toString();
			options._callback_name += 1;
			options._named_callbacks.set(action, name);
		}
		if (!cb_dedup.has(name)) {
			cb_dedup.add(name);
			if (!call_wrapped) {
				call_wrapped = 1;
				let nd_core = nd_ret;
				if (nd_core.node_class == N_CALL && nd_core.GetName() == 'DummyValue') {
					let nd_var = nd_core.Find(N_CALL, 'ReadName');
					if (nd_var) {
						nd_core = nd_var;
					}
				}
				let match = nd_core.Match(g_ptn_readname);
				if (match) {
					nd_ret = .(Sandbox.RunHooks(.(nd_ret), .(nd_extra_args), .(match.ctx.Clone()), .(nString(match.name.GetName())), ctx, .(nString(nd.GetUniqueTag()))));
				} else {
					nd_ret = .(Sandbox.RunHooks(.(nd_ret), .(nd_extra_args), vars, .(nString(nd.GetName())), ctx, .(nString(nd.GetUniqueTag()))));
				}
			}
			//nd_ret=nCall(nRef(name),nd_ret);
			nd_ret.Insert(POS_BACK, nRef(name));
		}
	}
	//inherit comments for debugging
	if (options.inherit_comments) {
		nd_ret.setCommentsBefore(nd.comments_before);
	}
	return nd_ret;
}

function dfsGenerateDefault(nd, options) {
	if (nd.node_class == N_FUNCTION) {
		//we need to test the function with "default" bindings, as well as the non-default ones
		//calls become call validations
		let nd_paramlist = nd.c.s;
		assert(nd_paramlist.node_class == N_PARAMETER_LIST);
		//generate the testing code
		let param_names = [];
		let default_params = [nAir()];
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
				.(dfsGenerate(nd_body, options));
				return ctx;
			}
			f(ctx, .(nItem.apply(null, default_params)));
			let value = Sandbox.FunctionValue(ctx, .(nString(nd.GetUniqueTag())), f.bind(null, ctx));
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
			Sandbox.Assign(vars, .(nString(nd.GetName())), Sandbox.ClassValue(ctx, .(nString(nd.GetUniqueTag())), function(ctx_outer) {
				let ctx = Sandbox.LazyChildScope(ctx_outer, .(nString(nd.GetUniqueTag())));
				let vars = ctx.vars;
				.(dfsGenerate(nd_body, options));
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
			return .(Sandbox.Declare(ctx, .(nString(nd.GetName())), .(nString(nd.GetUniqueTag()))));
		} else {
			return .(Sandbox.ReadName(vars, .(nString(nd.GetName())), .(nString(nd.GetUniqueTag()))));
		}
	}
	if (nd.node_class == N_DOT) {
		return .(Sandbox.ReadName(Sandbox.GetVarsContainer(.(nString(nd.GetUniqueTag())), .(dfsGenerate(nd.c, options))), .(nString(nd.GetName())), .(nString(nd.GetUniqueTag()))));
	}
	if (nd.node_class == N_ITEM) {
		//generate subscript values without using them: redundant args
		let subscripts = [];
		for (let ndi = nd.c.s; ndi; ndi = ndi.s) {
			subscripts.push(dfsGenerate(ndi, options));
		}
		return .(Sandbox.ReadName(Sandbox.GetVarsContainer(.(nString(nd.GetUniqueTag())), .(dfsGenerate(nd.c, options))), '[element]', .(nString(nd.GetUniqueTag())), .(subscripts)));
	}
	if (nd.node_class == N_ASSIGNMENT) {
		let nd_lhs = dfsGenerate(nd.c, options);
		let nd_rhs = dfsGenerate(nd.c.s, options);
		let nd_var = nd_lhs;
		while (nd_var.node_class == N_CALL) {
			let name = nd_var.GetName();
			if (name == 'RunHooks') {
				nd_var = nd_var.c.s;
			} else if (name == 'DummyValue') {
				let nd_decl = nd_var.Find(N_CALL, 'Declare');
				if (nd_decl) {
					nd_var = nd_decl;
				}
				break;
			} else {
				break;
			}
		}
		let match = nd_var.Match(g_ptn_readname);
		if (match) {
			match.ctx.Unlink();
			match.name.Unlink();
			match.addr.Unlink();
			let nd_replaced = .(Sandbox.Assign(.(match.ctx), .(match.name), .(nd_rhs), .(match.addr)));
			if (nd_lhs == match.nd) {nd_lhs = nd_replaced;} else {match.nd.ReplaceWith(nd_replaced);}
			return nd_lhs;
		}
		match = nd_var.Match(g_ptn_declare);
		if (match) {
			match.ctx.Unlink();
			match.name.Unlink();
			match.addr.Unlink();
			let nd_replaced = .(Sandbox.Declare(.(match.ctx), .(match.name), .(match.addr), .(nd_rhs)));
			if (nd_lhs == match.nd) {nd_lhs = nd_replaced;} else {match.nd.ReplaceWith(nd_replaced);}
			return nd_lhs;
		}
		//we don't understand it...
		//console.log(nd.dump(),nd_lhs.dump())
		return .(Sandbox.DummyValue(ctx, .(nString(nd.GetUniqueTag())), .(nd_lhs), .(nd_rhs)));
	}
	if (nd.node_class == N_CALL) {
		let children = [nAir()];
		for (let ndi = nd.c; ndi; ndi = ndi.s) {
			children.push(dfsGenerate(ndi, options));
		}
		return .(Sandbox.Call(ctx, .(nString(nd.GetUniqueTag())), .(nItem.apply(null, children))));
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
					let vars = ctx.vars;
					.(nd_then);
					Sandbox.MergeContext(ctx_if, [ctx_then]);
					Sandbox.EndScope(ctx);
				}
				let ctx_else = Sandbox.LazyCloneScope(ctx_if, .(nString(nd.GetUniqueTag())), 'in the else-clause');
				{
					let ctx = ctx_else;
					let vars = ctx.vars;
					.(nd_else);
					Sandbox.MergeContext(ctx_if, [ctx_else]);
					Sandbox.EndScope(ctx);
				}
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
			return .((function(ctx_before) {
				let ctx_loop = Sandbox.LazyCloneScope(ctx_before, .(nString(nd.GetUniqueTag())), 'loop initialization');
				{
					let ctx = ctx_loop;
					let vars = ctx.vars;
					.(nScope.apply(null, children));
				}
				let iterations = [ctx_loop];
				for (let i = 0; i < 2; i++) {
					let ctx = Sandbox.LazyCloneScope(iterations[iterations.length - 1], .(nString(nd.GetUniqueTag())), i == 0 ? 'in the first iteration' : 'in subsequent iterations');
					let vars = ctx.vars;
					.(dfsGenerate(nd_loop_body, options));
					Sandbox.MergeContext(ctx_loop, [ctx]);
					Sandbox.EndScope(ctx);
					iterations.push(ctx);
				}
				Sandbox.MergeContext(ctx_before, [ctx_loop]);
				Sandbox.EndScope(ctx_loop);
			})(ctx));
		}
	}
	if (nd.node_class == N_KEYWORD_STATEMENT) {
		if (nd.data == 'return') {
			let nd_value = dfsGenerate(nd.c, options);
			//yes, we need a second level of ()
			return .((ctx['return'] = .(nd_value)));
		}
	}
	if (nd.node_class == N_PAREN || nd.node_class == N_SEMICOLON ) {
		//avoid clutter
		return dfsGenerate(nd.c, options);
	}
	if (nd.node_class == N_SCOPE) {
		let children = [];
		for (let ndi = nd.c; ndi; ndi = ndi.s) {
			children.push(nSemicolon(dfsGenerate(ndi, options)));
		}
		return .((function(ctx_outer) {
			let ctx = Sandbox.LazyCloneScope(ctx_outer, .(nString(nd.GetUniqueTag())), undefined);
			let vars = ctx.vars;
			.(nScope.apply(null, children).c)/*no ;*/
			Sandbox.MergeContext(ctx_outer, [ctx]);
			Sandbox.EndScope(ctx);
		})(ctx));
	}
	//just treat scopes and air as dummy values
	//it's just {} so no point recording, but we need {}
	//it's pointless to record node-level information at run time then associate it back with a node
	let children = [.(Sandbox.DummyValue), nRef('ctx'), nString(nd.GetUniqueTag())];
	for (let ndi = nd.c; ndi; ndi = ndi.s) {
		children.push(dfsGenerate(ndi, options));
	}
	return nCall.apply(null, children);
}

omnichecker.RunGeneratedCode = function(nd_generated, options) {
	//pre-run the runtime lib in a sandboxed QuickJS runtime, passing back result as JSON
	let src = nd_generated.AutoFormat().toSource();
	let ret = __RunInSandbox(['(function(){\n', (options || {}).sandboxed_code || '', src, '\n//\\""\\\'\'\\``*/\n})'].join(''));
	if (options.dump_code) {
		console.log(src);
	}
	return ret && JSON.parse(ret);
}

//don't over-generalize: dedicated dataflow generator first
omnichecker.Check = function(nd_root, ...all_options) {
	let options = Object.create(null);
	options.default_value = {};
	options.templates = [];
	options.destructors = [];
	for (let options_i of all_options) {
		for (let key in options_i) {
			if (key == 'templates') {
				for (let t of options_i.templates) {
					options.templates.push(Object.create(t));
				}
			} else if (key == 'destructors') {
				for (let dtor of options_i.destructors) {
					options.destructors.push(dtor);
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
	options._named_callbacks = new Map();
	options._callback_name = 0;
	let nd_flowcode = dfsGenerate(nd_root, options);
	let callback_defs = [];
	for (let cb_pair of options._named_callbacks) {
		callback_defs.push('const ', cb_pair[1], '=', cb_pair[0], ';');
	}
	for (let cb of options.destructors) {
		callback_defs.push('Sandbox.destructors.push(', cb.toString(), ');');
	}
	for (let nd_dummy of nd_flowcode.FindAll(N_CALL, 'DummyValue')) {
		if (nd_dummy.p && (nd_dummy.p.node_class == N_SCOPE || nd_dummy.p.node_class == N_SEMICOLON)) {
			let nd_src = (nd_dummy.p.node_class == N_SEMICOLON ? nd_dummy.p : nd_dummy);
			let level = nd_src.indent_level;
			for (let ndi = nd_dummy.c.s.s.s; ndi; ) {
				let ndi_next = ndi.s;
				ndi.Unlink();
				ndi.AdjustIndentLevel(level);
				nd_src.Insert(POS_BEFORE, nSemicolon(ndi));
				ndi = ndi_next;
			}
			nd_src.Unlink();
		}
	}
	let ret = omnichecker.RunGeneratedCode(.({
		Sandbox.Reset();
		Sandbox.default_value = .(ParseCode(JSON.stringify(options.default_value || {})));
		let ctx = Object.create(null);
		ctx.utag = 0;
		ctx.utag_parent = -1;
		ctx.utag_addr = .(nString(nd_root.GetUniqueTag()));
		Sandbox.ctx_map.push(ctx);
		let vars = Sandbox.GetVarsContainer(ctx.utag_addr, ctx);
		vars['<utag>'] = ctx.utag;
		.(nSymbol(callback_defs.join('')));
		.(nd_flowcode);
		return JSON.stringify({
			//node_to_value: Sandbox.node_to_value,
			errors: Sandbox.errors,
			ctx_map: Sandbox.ctx_map
		});
		//the root context destruction is currently skipped
	}), options);
	if (options.dump_errors) {
		for (let err of ret.errors) {
			let nd_loc = Node.GetNodeFromUniqueTag(err.addr);
			//origin tracking - ret.ctx_map
			let site_path = [];
			for (let utag = err.site; utag > 0; utag = ret.ctx_map[utag].utag_parent) {
				site_path.push(utag);
			}
			let origin_path = site_path.map(utag=>utag);
			if (err.origin_utag) {
				origin_path = [];
				for (let utag = err.origin_utag; utag > 0; utag = ret.ctx_map[utag].utag_parent) {
					origin_path.push(utag);
				}
			}
			let message_parts = [];while (site_path.length && origin_path.length && site_path[site_path.length - 1] == origin_path[origin_path.length - 1]) {
				let utag = site_path.pop();
				origin_path.pop();
				//only keep the last level
				let ctx = ret.ctx_map[utag];
				if (ctx.utag_clause != undefined) {
					message_parts.length = 0;
					let nd_here = Node.GetNodeFromUniqueTag(ctx.utag_addr);
					let msg = ctx.utag_clause;
					message_parts.push(nd_here.FormatFancyMessage(msg, options.colored ? MSG_COLORED : 0));
				}
			}
			while (origin_path.length) {
				let utag = origin_path.pop();
				let ctx = ret.ctx_map[utag];
				let nd_here = Node.GetNodeFromUniqueTag(ctx.utag_addr);
				if (ctx.utag_clause != undefined) {
					let msg = 'from ' + ctx.utag_clause;
					message_parts.push(nd_here.FormatFancyMessage(msg, options.colored ? MSG_COLORED : 0));
				}
			}
			if (err.origin_addr != undefined) {
				let nd_here = Node.GetNodeFromUniqueTag(err.origin_addr);
				message_parts.push(nd_here.FormatFancyMessage('originated from', options.colored ? MSG_COLORED : 0));
			} 
			while (site_path.length) {
				let utag = site_path.pop();
				let ctx = ret.ctx_map[utag];
				let nd_here = Node.GetNodeFromUniqueTag(ctx.utag_addr);
				let msg = 'to';
				if (ctx.utag_clause != undefined) {
					msg = 'to ' + ctx.utag_clause;
				}
				message_parts.push(nd_here.FormatFancyMessage(msg, options.colored ? MSG_COLORED : 0));
			}
			for (let utag = err.site; utag > 0; utag = ret.ctx_map[utag].utag_parent) {
				let ctx = ret.ctx_map[utag];
			}
			message_parts.push(nd_loc.FormatFancyMessage(err.msg.replace(/\{code\}/g, nd_loc.dump()), options.colored ? MSG_COLORED | MSG_WARNING : MSG_WARNING))
			console.write(message_parts.join(''));
		}
	}
	return ret;
}
