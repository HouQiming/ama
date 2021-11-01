//this module is automatically executed by ama::JSRunInSandbox at most once
//it's in a sandbox and does not have access to ANY native function
//@ama ParseCurrentFile().Save()
//convention: sandbox objects should be dumb, the shouldn't have methods
__global.Sandbox={
	LazyChild:function(parent,name){
		let ret=parent[name];
		if(ret===undefined){
			ret=Object.create(null);
			parent[name]=ret;
		}
		return ret;
	},
	LazyClone:function(base){
		return Object.create(base);
	},
	Assign:function(ctx,name,value){
		if(name){
			//TODO: function overloading
			ctx[name]=value;
		}
	},
	AssignMany:function(ctx,names,values){
		let lg_min=Math.min(names.length,values.length);
		for(let i=0;i<lg_min;i++){
			if(names[i]){
				this.Assign(ctx,names[i],values[i]);
			}
		}
		return ctx;
	}
};
