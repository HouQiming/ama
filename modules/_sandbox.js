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
			this.MergePossibility(this.node_to_context_path,addr,ret);
		}
		return ret;
	},
	LazyClone:function(base){
		return Object.create(base);
	},
	Assign:function(ctx,name,value,addr){
		if(name){
			//TODO: function overloading, other value merging
			ctx[name]=value;
		}
		if(addr){
			this.MergePossibility(this.node_to_context_path,addr,value);
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
		//TODO
	},
	MergeContext:function(ctx,ctx_others){
		//TODO: vars, return, element
	},
	Call:function(ctx,addr,...values){
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
			this.MergePossibility(this.node_to_context_path,addr,this.LazyChild(ctx_f,'return'));
		}
		return this.node_to_context_path[addr];
	}
};
