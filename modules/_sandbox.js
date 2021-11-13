//this module is automatically executed by ama::JSRunInSandbox at most once
//it's in a sandbox and does not have access to ANY native function
//@ama ParseCurrentFile().Save()
//convention: sandbox objects should be dumb, the shouldn't have methods
__global.Sandbox={
	log:console.log,
	error:console.error,
	Reset:function(){
		this.default_value={};
		this.errors=[];
		this.ctx_map=[];
		this.error_dedup=new Set();
		this.destructors=[];
	},
	LazyChild:function(parent,name,addr){
		let ret=parent[name];
		if(ret===undefined){
			ret=Object.create(null);
			parent[name]=ret;
		}
		return ret;
	},
	ReadName:function(parent,name,addr){
		let ret=parent[name];
		if(ret===undefined){
			return [];
		}
		return ret;
	},
	GetVarsContainer:function(addr,parent){
		if(!parent.vars){
			if(parent.utag===undefined){
				parent.utag=this.ctx_map.length;
				parent.utag_addr=addr;
				parent.utag_clause='members';
				this.ctx_map.push(parent);
			}
			parent.vars={'<utag>':parent.utag,'<addrs>':Object.create(null)};
		}
		return parent.vars;
	},
	LazyCloneScope:function(base,addr,clause){
		let ret=Object.create(base);
		ret.utag=this.ctx_map.length;
		ret.utag_parent=base.utag;
		ret.utag_addr=addr;
		ret.utag_clause=clause;
		ret.vars=Object.create(base.vars||null);
		ret.vars['<utag>']=ret.utag;
		ret.vars['<addrs>']=Object.create(null);
		ret.children=undefined;
		this.ctx_map.push(ret);
		return ret;
	},
	LazyChildScope:function(ctx,addr){
		let ret=this.LazyChild(ctx,'children')[addr];
		if(ret===undefined){
			ret=Object.create(ctx);
			ret.utag=this.ctx_map.length;
			ret.utag_parent=ctx.utag;
			ret.utag_addr=addr;
			ret.vars=Object.create(ctx.vars||null);
			ret.vars['<utag>']=ret.utag;
			ret.children=undefined;
			this.ctx_map.push(ret);
			ctx.children[addr]=ret;
		}
		return ret;
	},
	Declare:function(ctx,name,addr,value){
		//it's always a new value
		if(!value){
			value=this.DummyValue(ctx,addr);
		}
		value.addr_declared=addr;
		ctx.vars[name]=value;
		ctx.vars['<addrs>'][name]=addr;
		return value;
	},
	Assign:function(vars,name,value,addr){
		//if(name in vars){
		//	while(vars&&!Object.prototype.hasOwnProperty.call(vars,name)){
		//		vars=vars.__proto__;
		//	}
		//}
		vars[name]=value;
		if(addr&&!vars['<addrs>'][name]){
			vars['<addrs>'][name]=addr;
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
	FunctionValue:function(ctx,addr,f){
		let ret=this.DummyValue(ctx,addr);
		ret.addr_declared=addr;
		ret.as_function=f;
		return ret;
	},
	ClassValue:function(ctx,addr,f){
		let ret=f();
		ret.ctx_utag=ctx.utag;
		ret.addr_declared=addr;
		ret.as_function=f;
		return ret;
	},
	DummyValue:function(ctx,addr){
		let ret=Object.create(this.default_value);
		ret.ctx_utag=ctx.utag;
		ret.addr_origin=addr;
		return ret;
	},
	MergePossibility:function(ctx,name,value){
		if(!value){return;}
		let value0=ctx[name];
		if(value0===value){return;}
		if(value0===undefined){
			ctx[name]=value;
			return value;
		}
		if(!Array.isArray(value0)){
			value0=[value0];
			ctx[name]=value0;
		}
		//collapse them later
		if(Array.isArray(value)){
			for(let v of value){
				value0.push(v);
			}
		}else{
			value0.push(value);
		}
		//this.log(name,value0,value);
	},
	MergeContext:function(ctx,all_other_ctxs){
		for(let ctx_other of all_other_ctxs){
			if(ctx===ctx_other){continue;}
			let vars=ctx.vars;
			for(let name in ctx_other.vars){
				if(name.startsWith('<')){continue;}
				this.MergePossibility(vars,name,ctx_other.vars[name]);
			}
			this.MergePossibility(ctx,'return',ctx_other['return']);
			//this.MergePossibility(ctx,'element',ctx_other['element']);
			if(ctx_other.children){
				let children=this.LazyChild(ctx,'children');
				for(let addr in ctx_other.children){
					this.MergeContext(
						this.LazyChildScope(ctx,addr),
						[ctx_other.children[addr]]
					);
				}
			}
		}
	},
	Call:function(ctx,addr,values){
		//expandable-later, function-indexible list of calls
		let value_func=values[0];
		//COULDDO: better function resolution
		let funcs=value_func.as_function;
		if(!funcs){
			funcs=[];
		}else if(!Array.isArray(funcs)){
			funcs=[funcs];
		}
		let params=values.slice(1);
		let ret={};
		for(let f of funcs){
			let ctx_f=f(params);
			this.MergePossibility(ret,'return',this.LazyChild(ctx_f,'return'));
		}
		return ret.return;
	},
	ArrayWrap:function(value){
		return Array.isArray(value)?value:(value==undefined?[]:[value]);
	},
	FilterErrors:function(ctx,addr,ret){
		if(!ret){return ret;}
		if(ret.error!==undefined){
			let err=ret;
			let msg=err.error;
			let offending_value=err.value;
			let key=[addr,msg].join('_');
			if(!this.error_dedup.has(key)){
				this.error_dedup.add(key);
				this.errors.push({
					msg:msg,
					property:key,
					origin:offending_value&&offending_value.ctx_utag,
					origin_addr:offending_value&&offending_value.addr_origin,
					site:ctx.utag,
					addr:addr
				});
			}
			return;
		}
		if(Array.isArray(ret)){
			for(let err of ret.filter(err=>err.error!==undefined)){
				this.FilterErrors(ctx,addr,err)
			}
			return ret.filter(err=>err.error===undefined);
		}
		return ret
	},
	RunHooks:function(value,extra_args,ref_vars,ref_name,ctx,addr,...cbs){
		let value_a=Array.isArray(value)?value:(value===undefined?[]:[value]);
		for(let cb of cbs){
			let ret=this.FilterErrors(ctx,addr,cb(value_a,extra_args,ref_vars,ref_name,ctx,addr));
			if(Array.isArray(ret)){
				value_a=ret;
				if(value_a.length===0){
					value=undefined;
				}else if(value_a.length===1){
					value=value_a[0];
				}else{
					value=value_a;
				}
			} 
		}
		return value;
	},
	set:function(obj0,obj1){
		return Object.assign(Object.create(obj0),obj1);
	},
	EnumVars:function(vars,ret){
		if(!ret){ret=[];}
		for(let name in vars){
			if(name.startsWith('<')){continue;}
			ret.push({vars:vars,name:name,addr:vars['<addrs>'][name]});
			for(let value of this.ArrayWrap(vars[name])){
				if(!value.as_function&&value.vars){
					this.EnumVars(value.vars,ret);
				}
			}
		}
		return ret;
	},
	FindValues:function(vars,filter){
		let ret=[];
		for(let desc of this.EnumVars(vars)){
			for(let value of this.ArrayWrap(desc.vars[desc.name])){
				if(filter(value)){ret.push(value);}
			}
		}
		return ret;
	},
	EndScope:function(ctx){
		if(!this.destructors.length){return;}
		for(let desc of this.EnumVars(ctx.vars)){
			for(let dtor of this.destructors){
				this.FilterErrors(ctx,desc.addr,dtor(desc.vars[desc.name],desc.vars,desc.name));
			}
		}
	}
};
