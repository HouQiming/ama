//work in progress
'use strict';
let omnichecker = module.exports;
const assert = require('assert');
const ParseCFor = require('cpp/c_for');

function FindDef(nd_defroot) {
	for (let ndj = nd_defroot; ndj; ndj = ndj.PreorderNext(nd_defroot)) {
		if (ndj.node_class == N_REF && (ndj.flags & REF_DECLARED)) {
			return ndj;
		}
	}
	return undefined;
}

omnichecker.CreateTestingCode = function CreateTestingCode(nd_root, options) {
	if (!options) {
		options = {};
	}
	options = Object.assign(Object.create({
		language: 'js',
		templates: [],
		properties: [],
		destructors: [],
		enable_warnings: 1,
		enable_shim: 1,
	}), options);
	assert(options.language == 'js');
	//setup a default property for function possibilities
	options.properties.unshift({
		name: 'f',
		type: 'array'
	})
	let templates_by_class = __node_class_names.map(name => []);
	for (let t of options.templates) {
		if (t.pattern.node_class == N_NODEOF) {
			if (t.pattern.c.node_class == N_CALL) {
				templates_by_class[__global[t.pattern.c.GetName()]].push(t);
			} else {
				throw new Error('invalid template ' + JSON.stringify(t))
			}
		} else {
			templates_by_class[t.pattern.node_class].push(t);
		}
	}
	let t_progress = new Map();
	let all_ppts = new Set(options.properties.map(ppt => ppt.name));
	function dfsTranslate(nd, is_lvalue) {
		let utag = nd.GetUniqueTag();
		let templates = templates_by_class[nd.node_class];
		let helper = {
			match: undefined,
			translate: dfsTranslate,
			set: function(name, value, nd_value) {
				if (!nd_value) {
					nd_value = this.translate(this.match.nd);
				}
				if (!all_ppts.has(name)) {throw new ReferenceError('undeclared property ' + JSON.stringify(name));}
				return @(@(nRef('set_' + name))(ctx, @(nd_value), @(ParseCode(JSON.stringify(value))), @(nString(this.match.nd.GetUniqueTag()))));
			}
		};
		for (let i = t_progress.get(utag) | 0; i < templates.length; i++) {
			let match = nd.match(t.pattern);
			if (!match) {continue;}
			//allow self-recursion
			t_progress.set(utag, i + 1);
			//on-generation callback: {action:(match,helper)=>helper.set(name,value,helper.translate(match.nd))}
			//calling translate again on match.nd will skip the current hook
			helper.match = match;
			return t.action(match, helper);
		}
		if (nd.node_class == N_FUNCTION || nd.node_class == N_CLASS) {
			//a function, a context
			let nd_params = undefined;
			let params_default = [];
			if (nd.node_class == N_FUNCTION) {
				let params = [];
				let id = 0;
				for (let nd_param of nd.c.s.children) {
					if (nd_param.node_class == N_ASSIGNMENT) {
						let nd_def = FindDef(nd_param.c);
						if (nd_def) {
							let name = nd_def.GetName();
							params.push(@(
								Declare(ctx, @(nString(name)), @(nString(nd_def.GetUniqueTag())));
								if (params.length < @(nNumber(id.toString()))) {
									Assign(ctx, ctx[@(nString(name))], params[@(nNumber(id.toString()))]);
								}
							));
						}
						params_default.push(dfsTranslate(nd_param.c.s));
					} else {
						//default value: 
						params_default.push(@(DummyValue(ctx, @(nString(utag)))));
					}
					id += 1;
				}
				nd_params = nScope.apply(null, params);
			} else {
				//TODO: inherit base class contexts
				//we're not *that* dependent on classes in JS mode
				nd_params = nAir();
			}
			let nd_default_params = nRaw.apply(null, params_default).setFlags(0x5d5b/*[]*/);
			let utag_body = nd.LastChild().GetUniqueTag();
			//QueueCall declares the return value object
			let nd_func = @(QueueCall(Object.assign(function(params) {
				let _ctx_outer = ctx;
				try {
					let ctx = GetScopeByTag(_ctx_outer, @(nString(utag_body)), @(nString(utag)));
					let ctx_func = ctx;
					if (params) {
						@(nd_params);
					}
					//reach memorization - it throws
					Reach(ctx, @(nString(utag_body)));
					@(dfsTranslate(nd.LastChild()));
				} finally () {
					EndScope(ctx);
				}
			}, {utag: @(nString(utag))}), @(nd_default_params)).f);
			let nd_def = nd.node_class == N_FUNCTION ? FindDef(nd.c) : nd.c.s;
			//TODO: C++ out-of-line definition
			if (nd_def) {
				let name = nd_def.GetName();
				nd_func = @(
					Declare(ctx, @(nString(name)), @(nString(nd_def.GetUniqueTag())));
					Assign(ctx, ctx[@(nString(name))], {f: [{value: @(nd_func),addr: @(nString(utag))}]}, @(nString(utag)));
				);
			}
			return nd_func;
		}
		if (nd.node_class == N_SCOPED_STATEMENT) {
			//COULDDO: condition hooks
			if (nd.data == 'if') {
				//actual if, but with exploration - record age-based exploration states
				//store exploration status by addr
				//CFG contexts need to age their parents
				let nd_if = @(if (Explore(ctx_func, ctx, @(nString(utag)), 2, @(dfsTranslate(nd.c))) === 0) {
					Reach(ctx, @(nString(nd.c.s.GetUniqueTag())));
					@(dfsTranslate(nd.c.s));
				});
				let nd_else = nd.c.s.s;
				if (nd_else) {
					assert(nd_else.data == 'else');
					nd_if.Insert(POS_BACK, nExtensionClause('else', nAir(), @({
						Reach(ctx, @(nString(nd_else.c.s.GetUniqueTag())));
						@(dfsTranslate(nd_else.c.s));
					})));
				}
				return nd_if;
			} else if (nd.GetCFGRole() == CFG_LOOP) {
				let nd_init = undefined;
				let nd_cond_begin = undefined;
				let nd_cond_end = undefined;
				let nd_iter_begin = undefined;
				let nd_iter_end = undefined;
				if (nd.data == 'for') {
					let desc = ParseCFor(nd);
					if (desc) {
						//init / cond / iter
						nd_init = dfsTranslate(desc.init);
						nd_cond_begin = dfsTranslate(desc.cond);
						nd_iter_end = dfsTranslate(desc.iter);
					} else {
						//assume range for
						nd_iter_begin = dfsTranslate(nd.c);
					}
				} else if (nd.data == 'do') {
					let nd_while = nd.c.s.s;
					if (nd_while) {
						nd_cond_end = dfsTranslate(nd_while.c);
					}
				} else if (nd.data == 'while') {
					nd_cond_begin = dfsTranslate(nd.c);
				} else {
					if (options.enable_warnings) {
						console.error('unimplemented loop statement:', nd.data);
					}
					if (nd.c.node_class != N_AIR) {
						nd_cond_begin = dfsTranslate(nd.c);
					}
				}
				assert(!nd_cond_begin || !nd_cond_end);
				if (!nd_init) {nd_init = nAir();}
				//gadget for break / continue interception
				//2 indicates break, 1 indicates continue, 0 indicates nothing
				//ncb: nothing-continue-break
				let nd_body = dfsTranslate(nd.c.s);
				if (nd_iter_begin) {
					nd_body = nScope(nd_iter_begin, nd_body);
				}
				nd_body = @({
					let ncbgadget = 0;
					let ncbgadget_i = 0;
					for (ncbgadget = 2; ncbgadget_i++ == 0; ncbgadget = 1) {
						@(nd_body);
						ncbgadget = 0;
					}
					//continue is equivalent to do-nothing here
					if (ncbgadget == 2) {break;}
					@(nd_iter_end || nAir());
				});
				if (nd.c.s.node_class != N_SCOPE) {
					nd_body.Insert(POS_FRONT, @(Reach(ctx, @(nString(nd.c.s.GetUniqueTag())));));
				}
				if (nd_cond_begin) {
					nd_body = @(while (Explore(ctx_func, ctx, @(nString(utag)), 2, @(nd_cond_begin)) === 0) @(nd_body));
				} else if (nd_cond_end) {
					nd_body = @(do {@(nd_body)} while (Explore(ctx_func, ctx, @(nString(utag)), 2, @(nd_cond_end)) === 0));
				} else {
					//TODO: explore cond: make sure it terminates once values stop changing
					nd_body = @(for (; ;) @(nd_body))
				}
				if (nd_init) {
					nd_body = @({
						let _ctx_outer = ctx;
						try {
							let ctx = GetScopeByTag(_ctx_outer, @(nString(utag)));
							@(nd_init);
							@(nd_body);
						} finally () {
							EndScope(ctx);
						}
					});
				}
				return nd_body;
			} else {
				if (options.enable_warnings) {
					console.error('unimplemented statement:', nd.data);
				}
			}
		}
		if (nd.node_class == N_KEYWORD_STATEMENT) {
			if (nd.data == 'break' || nd.data == 'continue') {
				return nKeywordStatement(nd.data);
			} else if (nd.data == 'return') {
				//the return value of Assign should be ignored
				let nd_value = dfsTranslate(nd.c);
				return @(return Return(ctx, @(nd_value), @(nString(utag))););
			} else {
				if (options.enable_warnings) {
					console.error('unimplemented statement:', nd.data);
				}
			}
		}
		if (nd.node_class == N_CALL) {
			return @(Call(ctx, @(dfsTranslate(nd.c)), @(nScope.apply(null, nd.children.slice(1).map(ndi=>dfsTranslate(ndi))).c))); 
		}
		if (nd.node_class == N_SCOPE) {
			return @({
				let _ctx_outer = ctx;
				try {
					let ctx = GetScopeByTag(_ctx_outer, @(nString(utag)));
					@(nScope.apply(null, nd.children.map(ndi=>dfsTranslate(ndi))).c)/*no ;*/
				} finally () {
					EndScope(ctx);
				}
			});
		}
		if (nd.node_class == N_ASSIGNMENT) {
			return @(Assign(ctx, @(dfsTranslate(nd.c, 1)), @(dfsTranslate(nd.c.s)), @(nString(utag))));
		}
		if (nd.node_class == N_DOT) {
			let nd_ret = @(Dot(@(dfsTranslate(nd.c, 1)), @(nString(utag)), @(nString(nd.data))));
			if (!is_lvalue) {
				nd_ret = nd_ret.dot('value');
			}
			return nd_ret;
		}
		if (nd.node_class == N_ITEM) {
			let nd_ret = @(Item(
				@(dfsTranslate(nd.c, 1)), @(nString(utag)),
				@(nScope.apply(null, nd.children.slice(1).map(ndi=>dfsTranslate(ndi))).c)
			));
			if (!is_lvalue) {
				nd_ret = nd_ret.dot('value');
			}
			return nd_ret;
		}
		//COULDDO: && / ||
		if (nd.node_class == N_REF) {
			let nd_ret = undefined;
			if (nd.flags & REF_DECLARED) {
				nd_ret = @(Declare(ctx, @(nString(nd.data)), @(nString(utag))));
			} else {
				nd_ret = @(ctx[@(nString(nd.data))]);
			}
			if (!is_lvalue) {
				nd_ret = nd_ret.dot('value');
			}
			return nd_ret;
		}
		let is_lvalue_child = 0;
		if (nd.node_class == N_PREFIX && (nd.data == '&' || nd.data == '&&')) {
			is_lvalue_child = 1;
		}
		return @(DummyValue(ctx, @(nString(utag)), @(nScope.apply(null, nd.children.map(ndi=>dfsTranslate(ndi, is_lvalue_child))).c)));
	}
	let nd_gen = dfsTranslate(nd_root);
	if (options.enable_shim) {
		let code_init = [@(Init();)];
		let big_merger = [];
		for (let ppt of options.properties) {
			if (ppt.default_value != undefined) {
				code_init.push(@(Sandbox.default_value[@(nString(ppt.name))] = @(ParseCode(JSON.stringify(ppt.default_value)));));
			}
			//create per-property setter / merger: set_foo, merge_foo
			let set_foo = 'set_' + ppt.name;
			let merge_foo = 'merge_' + ppt.name;
			let type = (ppt.type || 'array');
			if (type == 'array') {
				code_init.push(@(
					function @(nRef(set_foo))(ctx, tar, value, addr) {
						let cands = tar[@(nString(ppt.name))];
						if (!cands) {
							cands = [];
							tar[@(nString(ppt.name))] = cands;
						}
						let found = 0;
						for (let i = 0; i < cands.length; i++) {
							if (cands[i].value === value) {
								found = 1;
								break;
							}
						}
						if (!found) {
							cands.push({
								value: value,
								source: addr,
							});
							BumpAge(ctx);
						}
						return tar;
					}
					function @(nRef(merge_foo))(ctx, tar, src) {
						let cands = tar[@(nString(ppt.name))];
						if (!cands) {
							cands = [];
							tar[@(nString(ppt.name))] = cands;
						}
						let dedup = new Set(cands.map(item=>item.value));
						let changed = 0;
						for (let item of src) {
							if (dedup.has(item.value)) {continue;}
							cands.push(item);
							changed = 1;
						}
						if (changed) {BumpAge(ctx);}
						return tar;
					}
				));
			} else if (type == 'max') {
				code_init.push(@(
					function @(nRef(set_foo))(ctx, tar, value, addr) {
						let cur = tar[@(nString(ppt.name))];
						if (cur === undefined) {
							cur = {
								value: Sandbox.default_value[@(nString(ppt.name))] || 0,
								addr: undefined
							};
							tar[@(nString(ppt.name))] = cur;
						}
						if (cur.value < value) {
							cur.value = value;
							cur.addr = addr;
							BumpAge(ctx);
						}
						return tar;
					}
					function @(nRef(merge_foo))(ctx, tar, src) {
						return @(nRef(set_foo))(ctx, tar, src.value, src.addr)
					}
				));
			} else {
				throw new Error('invalid property type: ' + type)
			}
			big_merger.push(@(if (src[@(nRef(ppt.name))] != undefined) {{@(nRef(merge_foo))(ctx, tar, src[@(nRef(ppt.name))]));}});
		}
		code_init.push(@(
			function MergeValue(ctx, tar, src) {
				@(nScope.apply(null, big_merger));
			}
		));
		for (let dtor of options.destructors) {
			//TODO
		}
		nd_gen = @(
			Sandbox.Init();
			@(nScope.apply(null, code_init).c)/*no `;`*/
			@(nd_gen);
			while (!Sandbox.queue.empty()) {
				let item = Sandbox.queue.shift();
				try {
					item.f(item.params);
				} catch (e) {
					if (e === Sandbox.AlreadyMemorized) {
						continue;
					} else {
						throw e;
					}
				}
				//re-queue if still exploring
				Sandbox.queue.push(item);
			});
		//TODO: wrap up - return the errors
	}
	for (let nd_binop of nd_gen.FindAll(N_BINOP)) {
		if (nd_binop.data == '==') {nd_binop.data = '===';} else
		if (nd_binop.data == '!=') {nd_binop.data = '!==';}
	}
	return nd_gen;
};
