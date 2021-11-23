'use strict';
//This module is automatically executed by ama::JSRunInSandbox at most once
//It's in a sandbox and does not have access to ANY native function
//@ama ParseCurrentFile().Save()
//convention: sandbox objects should be dumb, the shouldn't have methods
__global.Sandbox={
	Reset:function(){
		this.default_value={};
		this.errors=[];
		this.ctx_map=[];
		this.error_dedup=new Set();
		this.destructors=[];
	},
	Declare:function(ctx,name,addr){
		ctx[name]=!?;
		return value;
	},
	Assign:function(ctx,name,value,addr){
		//TODO: should do a merge, need custom merges
		ctx[name]=value;
		return value;
	},
	DummyValue:function(ctx,addr){
		let ret=Object.create(this.default_value);
		ret.ctx_utag=ctx.utag;
		ret.addr_origin=addr;
		return ret;
	},
	GetScopeByTag:function(ctx,addr){
		//let ctx={'[parent]':_ctx_outer,'[age]':0};
		//TODO
	},
	Reach:function(ctx,addr){
		//TODO: throw on memorization
	},
	Explore:function(ctx,addr,branches/*,...dummy_args*/){
		//TODO
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
