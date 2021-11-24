'use strict';
//This module is automatically executed by ama::JSRunInSandbox at most once
//It's in a sandbox and does not have access to ANY native function
//@ama ParseCurrentFile().Save()
__global.Sandbox={
	AlreadyMemorized:new Error('already memorized'),
	console:console,
	Init:function(){
		this.default_value={};
		this.ctx_map={};
		this.reach_map=new Map();
		this.explore_map=new Map();
		this.return_map=new Map();
		this.errors=[];
		this.error_dedup=new Set();
		this.destructors=[];
		this.queue=[];
		for(let key in this){
			let value=this[key];
			if(typeof(value)==='function'){
				//globalize all methods
				__global[key]=value.bind(this);
			}
		}
		__global.console=this.console;
	},
	Declare:function(ctx,name,addr){
		if(!ctx[name]){
			ctx[name]={value:{},addr:addr};
		}
		return ctx[name];
	},
	Assign:function(ctx,lvalue,rvalue,addr){
		if(rvalue){
			//merge them
			__global.MergeValue(ctx,lvalue.value,rvalue);
		}
		return rvalue;
	},
	Dot:function(lvalue,addr,name){
		return this.Declare(lvalue,'.'+name,addr);
	},
	Item:function(lvalue,addr/*,subscript*/){
		return this.Declare(lvalue,'[]',addr);
	},
	DummyValue:function(ctx,addr/*,...ignored*/){
		return Object.create(this.default_value);
	},
	GetScopeByTag:function(ctx,addr,addr_return){
		let ret=this.ctx_map[addr];
		if(!ret){
			ret={'[parent]':ctx,'[age]':1,'[utag]':addr,'[return]':this.return_map.get(addr_return)};
			this.ctx_map[addr]=ret;
		}
		return ret;
	},
	BumpAge:function(ctx){
		for(let ctxi=ctx;ctxi;ctxi=ctxi['[parent]']){
			ctxi['[age]']+=1;
		}
	},
	Reach:function(ctx,addr){
		//TODO: record CFG path for error reporting
		let reached_age=this.reach_map.get(addr)|0;
		if(reached_age!==ctx['[age]']){
			this.reach_map.set(addr,ctx['[age]']);
		}else if(ctx.still_exploring){
			//explore-more queueing: in-context unfinished-ness tagging: inside Reach
			ctx.still_exploring=0;
		}else{
			//we're done
			throw this.AlreadyMemorized;
		}
	},
	Explore:function(ctx_func,ctx,addr,n_branches/*,...dummy_args*/){
		let mod=n_branches+1;
		let st=this.explore_map.get(addr)|0;
		let age_expected=st/mod|0;
		if(age_expected!==ctx['[age]']){
			st=ctx['[age]']*mod;
		}
		let ret=st%mod;
		if(ret===n_branches){
			//we're done
			throw this.AlreadyMemorized;
		}
		if(ret+1<n_branches){
			//explore-more queueing - ctx_func
			ctx_func.still_exploring=1;
		}
		this.explore_map.set(addr,st+1);
		return ret;
	},
	EndScope:function(ctx){
		if(!this.destructors.length){return;}
		for(let desc of this.EnumVars(ctx.vars)){
			for(let dtor of this.destructors){
				this.FilterErrors(ctx,desc.addr,dtor(desc.vars[desc.name],desc.vars,desc.name));
			}
		}
	},
	QueueCall:function(f,params){
		let obj_return=this.return_map.get(f.utag);
		if(!obj_return){
			obj_return={value:{},caller_ctx:new Set()};
			this.return_map.set(f.utag,obj_return);
		}
		this.queue.push({f:f,params:params});
		return {f:f,obj_return:obj_return};
	},
	Call:function(ctx,value_func,params){
		let value_ret=Object.create(this.default_value);
		for(let f of (value_func.f||[])){
			let desc=QueueCall(f,params);
			desc.obj_return.caller_ctx.add(ctx);
			__global.MergeValue(ctx,value_ret,desc.obj_return.value);
		}
		return value_ret;
	},
	Return:function(ctx,rvalue,addr){
		let age0=ctx['[age]'];
		this.Assign(ctx,ctx['[return]'].value,rvalue,addr);
		if(age0<ctx['[age]']){
			//return value updated, notify all callers
			for(let ctxi of ctx['[return]'].caller_ctx){
				BumpAge(ctxi);
			}
		}
		return rvalue;
	},
	//TODO: error reporting, on-the-spot backtracking
};
