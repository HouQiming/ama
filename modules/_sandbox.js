//this module is automatically executed by ama::JSRunInSandbox at most once
//it's in a sandbox and does not have access to ANY native function
//@ama ParseCurrentFile().Save()
//convention: sandbox objects should be dumb, the shouldn't have methods
__global.Sandbox={
	default_value:{},
	errors:[],
	ctx_map:[],
	log:console.log,
	error:console.error,
	LazyChild:function(parent,name,addr){
		let ret=parent[name];
		if(ret===undefined){
			ret=Object.create(null);
			parent[name]=ret;
		}
		if(addr){
			this.MergePossibility(this.node_to_value,addr,ret);
		}
		return ret;
	},
	LazyCloneScope:function(base,addr,clause){
		let ret=Object.create(base);
		ret.utag=this.ctx_map.length;
		ret.utag_parent=base.utag;
		ret.utag_addr=addr;
		ret.utag_clause=clause;
		this.ctx_map.push(ret);
		return ret;
	},
	LazyChildScope:function(ctx,addr){
		let ret=this.LazyChild(ctx,'children')[addr];
		if(ret===undefined){
			ret=Object.create(ctx);
			ret.vars=Object.create(ctx.vars);
			ret.utag=this.ctx_map.length;
			ret.utag_parent=ctx.utag;
			ret.utag_addr=addr;
			this.ctx_map.push(ret);
			ctx.children[addr]=ret;
			this.node_to_value[addr]={ctx_utag:ret.utag};
		}
		return ret;
	},
	Declare:function(parent,name,addr){
		//it's always a new value
		let ret={addr_declared:addr};
		parent[name]=ret;
		this.MergePossibility(this.node_to_value,addr,ret);
		return ret;
	},
	Assign:function(ctx,name,value,addr){
		ctx[name]=value;
		if(addr){
			this.MergePossibility(this.node_to_value,addr,value);
		}
		return value;
	},
	AssignMany:function(ctx,names,values){
		let lg_min=Math.min(names.length,values.length);
		for(let i=0;i<lg_min;i++){
			if(names[i]){
				this.Assign(ctx,names[i],values[i]);
			}
		}
		return ctx;
	},
	FunctionValue:function(addr,f){
		let ret=Object.create(this.default_value);
		ret.addr_declared=addr;
		ret.as_function=f;
		return ret;
	},
	ClassValue:function(addr,f){
		let ret=f();
		ret.addr_declared=addr;
		ret.as_function=f;
		return ret;
	},
	DummyValue:function(addr){
		return Object.create(this.default_value);
	},
	MergePossibility:function(ctx,name,value){
		if(!value){return;}
		let value0=ctx[name];
		if(value0===undefined){
			ctx[name]=value;
			return value;
		}
		if(!Array.isArray(value0)){
			value0=[value0];
			ctx[name]=value0;
		}
		//collapse them later
		value0.push(value);
	},
	MergeContext:function(ctx,ctx_others){
		if(ctx===ctx_others){return;}
		if(ctx_others.vars){
			let vars=this.LazyChild(ctx,'vars');
			for(let name in ctx_others.vars){
				this.MergePossibility(vars,name,ctx_others.vars[name]);
			}
		}
		this.MergePossibility(ctx,'return',ctx_others['return']);
		this.MergePossibility(ctx,'element',ctx_others['element']);
		if(ctx_others.children){
			let children=this.LazyChild(ctx,'children');
			for(let addr in ctx_others.children){
				this.MergeContext(
					this.LazyChildScope(ctx,addr),
					ctx_others.children[addr]
				);
			}
		}
		//TODO: child contexts
	},
	Call:function(ctx,addr,values){
		//expandable-later, function-indexible list of calls
		let value_func=values[0];
		//COULDDO: better function resolution
		let funcs=value_func.as_function;
		if(!f){
			funcs=[];
		}else if(!Array.isArray(funcs)){
			funcs=[funcs];
		}
		let params=values.slice(1);
		for(let f of funcs){
			let ctx_f=f(params);
			this.MergePossibility(this.node_to_value,addr,this.LazyChild(ctx_f,'return'));
		}
		return this.node_to_value[addr];
	},
	SetProperties:function(value,ppts){
		Object.assign(value,ppts);
		return value;
	},
	CheckProperties:function(ctx,value,addr,all_ppts){
		for(let ppt of all_ppts){
			for(let key in ppt){
				let vals=value[key];
				if(!Array.isArray(vals)){
					if(vals===undefined){
						vals=[];
					}else{
						vals=[vals];
					}
				}
				let expected=ppt[key];
				let failed=0;
				if(expected.not!==undefined){
					failed=vals.indexOf(expected.not)>=0;
				}else if(expected.must_be!==undefined){
					failed=(vals.filter(val=>val!==expected.must_be).length>0||vals.length===0);
				}else if(expected.not_empty){
					failed=(vals.length===0);
				}else if(expected.empty){
					failed=(vals.length>0);
				}
				if(expected.action){
					if(!failed){
						if(expected.action==='skip'){
							break;
						}
					}
				}else if(failed){
					this.errors.push({
						msg:expected.msg,
						property:key,
						origin:ctx.utag,
						addr:addr
					})
				}
			}
		}
		return value;
	},
};
