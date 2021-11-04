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

let g_ptn_lazychild=.(Sandbox.LazyChild(.(Node.MatchAny('ctx')),.(Node.MatchAny(N_STRING,'name')),.(Node.MatchAny(N_STRING,'addr'))));
function dfsGenerate(nd, options) {
	if(options.hook){
		let nd_ret=options.hook(nd);
		if(nd_ret){
			return nd_ret;
		}
	}
	//just treat scopes and air as dummy values
	//if (nd.node_class == N_SCOPE || nd.node_class == N_FILE) {
	//	let children = [];
	//	for (let ndi = nd.c; ndi; ndi = ndi.s) {
	//		children.push(dfsGenerate(ndi, options));
	//	}
	//	return nScope.apply(null, children);
	//}
	//if (nd.node_class == N_AIR) {
	//	return nAir();
	//}
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
			default_params.push(defGenFlow(ndi,options));
		}
		//translate the body
		let nd_body = nd.LastChild();
		return .((function(){
			let f = function(ctx_outer, params) {
				let ctx = Sandbox.LazyChild(ctx_outer, .(nString(nd.GetUniqueTag())), .(nString(nd.GetUniqueTag())));
				let vars = Sandbox.LazyChild(ctx, 'vars');
				Sandbox.AssignMany(vars, .(ParseCode(JSON.stringify(param_names))), params);
				.(dfsGenerate(nd_body, options))/*no `;`*/
				return ctx;
			}
			f(ctx, .(nRaw.apply(null, default_params).setFlags(0x5D5B/*[]*/)));
			let value={as_function: f};
			.(nd.GetName()?.(Sandbox.Assign(vars, .(nString(nd.GetName())), value);):nAir())/*no `;`*/
			return value;
		})());
	}
	if (nd.node_class == N_CLASS) {
		//fields, base class, resolve methods using or-on-Assign
		//all those are handled under nd_body
		let nd_body = nd.LastChild();
		return .((function(){
			let c = function(ctx_outer) {
				let ctx = Sandbox.LazyChild(ctx_outer, .(nString(nd.GetUniqueTag())), .(nString(nd.GetUniqueTag())));
				let vars = Sandbox.LazyChild(ctx, 'vars');
				.(dfsGenerate(nd_body, options))/*no `;`*/
				return ctx;
			}
			//COULDDO: route to the proper constructor
			return Sandbox.Assign(vars, .(nString(nd.GetName())), {as_function: c});
		})());
	}
	if(nd.node_class==N_REF){
		//TODO: name resolution
		let nd_ret=.(Sandbox.LazyChild(vars,.(nString(nd.GetName())),.(nString(nd.GetUniqueTag()))));
		if(nd.flags&REF_DECLARED){
			//TODO: set declared type and declared node - typing.ComputeDeclaredType? separate code for the N_RAW case?
			//we will need types for certain checks
			//but the code here should be capable of independently resolving them
			//represent type with node addr? as raw expression and try to resolve the name?
		}
		return nd_ret;
	}
	if(nd.node_class==N_DOT){
		//TODO: name resolution for air dot
		return .(Sandbox.LazyChild(Sandbox.LazyChild(.(dfsGenerate(nd.c,options)),'vars'),.(nString(nd.GetName())),.(nString(nd.GetUniqueTag()))));
	}
	if(nd.node_class==N_ITEM){
		//generate subscript values without using them: redundant args
		let subscripts=[];
		for(let ndi=nd.c.s;ndi;ndi=ndi.s){
			subscripts.push(dfsGenerate(ndi,options));
		}
		return .(Sandbox.LazyChild(.(dfsGenerate(nd.c,options)),'element',.(nString(nd.GetUniqueTag())),.(subscripts)));
	}
	if (nd.node_class == N_ASSIGNMENT) {
		let nd_lhs=dfsGenerate(nd.c,options);
		let nd_rhs=dfsGenerate(nd.c.s,options);
		let match=nd_lhs.Match(g_ptn_lazychild);
		if(match){
			match.ctx.Unlink();
			match.name.Unlink();
			return .(Sandbox.Assign(.(match.ctx),.(match.name),.(nd_rhs),.(match.addr)))
		}
		//we don't understand it...
		return nScope(nd_lhs,nd_rhs);
	}
	if (nd.node_class == N_CALL) {
		let children=[.(Sandbox.Call),.(ctx),nString(nd.GetUniqueTag())];
		for(let ndi=nd.c;ndi;ndi=ndi.s){
			children.push(dfsGenerate(ndi,options));
		}
		return nCall.apply(null,children);
	}
	if (nd.node_class == N_SCOPED_STATEMENT) {
		if (nd.data == 'if') {
			let nd_cond=dfsGenerate(nd.c,options);
			let nd_then=dfsGenerate(nd.c.s,options);
			let nd_else=nd.c.s.s?dfsGenerate(nd.c.s.s,options):nScope();
			//fork / join, treat the condition normally
			return .((function(ctx_if){
				let ctx=ctx_if;
				let ctx_then=Sandbox.LazyClone(ctx_if,.(nd_cond));
				{
					let ctx=ctx_then;
					let vars = Sandbox.LazyChild(ctx, 'vars');
					.(nd_then)/*no `;`*/
				}
				let ctx_else=Sandbox.LazyClone(ctx_if);
				{
					let ctx=ctx_else;
					let vars = Sandbox.LazyChild(ctx, 'vars');
					.(nd_else)/*no `;`*/
				}
				Sandbox.MergeContext(ctx_if,[ctx_then,ctx_else]);
			})(ctx))
		}
		if(nd.GetCFGRole()==CFG_LOOP){
			let children=[];
			let nd_loop_body=undefined;
			for(let ndi=nd.c;ndi;ndi=ndi.s){
				if(ndi.node_class==N_SCOPE&&!nd_loop_body){
					nd_loop_body=ndi;
				}else{
					children.push(dfsGenerate(ndi,options));
				}
			}
			//loops: change to for-twice
			return .((function(ctx_loop){
				{
					let ctx=ctx_loop;
					.(nScope.apply(null,children));
				}
				let iterations=[ctx_loop];
				for(let i=0;i<2;i++){
					let ctx=Sandbox.LazyClone(iterations[iterations.length-1]);
					let vars = Sandbox.LazyChild(ctx, 'vars');
					.(dfsGenerate(nd_loop_body,options))/*no `;`*/
					iterations.push(ctx);
				}
				Sandbox.MergeContext(ctx_loop,iterations);
			})(ctx))
		}
	}
	if (nd.node_class == N_KEYWORD_STATEMENT) {
		if (nd.data == 'return') {
			let nd_value=dfsGenerate(nd.c,options);
			return .(Sandbox.Assign(ctx,'return',.(nd_value),.(nString(nd.GetUniqueTag()))))
		}
	}
	//it's just {} so no point recording, but we need {}
	//it's pointless to record node-level information at run time then associate it back with a node
	let children=[.(Sandbox.DummyValue),nString(nd.GetUniqueTag())];
	for(let ndi=nd.c;ndi;ndi=ndi.s){
		children.push(dfsGenerate(ndi,options));
	}
	return nCall.apply(null,children);
}

omnichecker.RunGeneratedCode = function(nd_generated, options) {
	//pre-run the runtime lib in a sandboxed QuickJS runtime, passing back result as JSON
	let ret = __RunInSandbox(['function(){\n', (options || {}).sandboxed_code || '', nd_generated.toSource(), '\n//\\""\\\'\'\\``*/\n}'].join(''));
	return ret && JSON.parse(ret);
}

//don't over-generalize: dedicated dataflow generator first
omnichecker.GenerateChecker = function(nd_root, options) {
	options=Object.create(options||null);
	let nd_flowcode = dfsGenerate(nd_root, options);
	let df_tree = omnichecker.RunGeneratedCode(.({
		Sandbox.node_to_context_path = Object.create(null);
		let ctx = Object.create(null);
		let vars = Sandbox.LazyChild(ctx, 'vars');
		.(nd_flowcode)/*no `;`*/
		return JSON.stringify(ctx);
	}), options);
	//TODO: parse df_tree: the node addrs
}
