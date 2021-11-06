//this module is automatically executed by ama::JSRunInSandbox at most once
//it's in a sandbox and does not have access to ANY native function
//@ama ParseCurrentFile().Save()
//convention: sandbox objects should be dumb, the shouldn't have methods
__global.Sandbox={
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
	LazyClone:function(base){
		return Object.create(base);
	},
	LazyChildScope:function(parent,addr){
		let ret=parent[addr];
		if(ret===undefined){
			ret=Object.create(parent);
			parent[name]=ret;
		}
		this.node_to_value[addr]=ret;
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
		if(name){
			//TODO: function overloading, other value merging
			ctx[name]=value;
		}
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
	DummyValue:function(addr){
		return {};
	},
	MergePossibility:function(ctx,name,value){
		if(!value){return;}
		let value0=ctx[name];
		if(!value0){
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
		if(ctx_others.vars){
			let vars=this.LazyChild(ctx,'vars');
			for(let name in ctx_others.vars){
				this.MergePossibility(vars,name,ctx_others.vars[name]);
			}
		}
		this.MergePossibility(ctx,'return',ctx_others['return']);
		this.MergePossibility(ctx,'element',ctx_others['element']);
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
			let ctx_f=f(ctx,params);
			this.MergePossibility(this.node_to_value,addr,this.LazyChild(ctx_f,'return'));
		}
		return this.node_to_value[addr];
	}
};
