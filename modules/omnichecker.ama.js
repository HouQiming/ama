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

omnichecker.CreateTestingCode=function CreateTestingCode(nd_root,options){
	if(!options){
		options={};
	}
	options=Object.assign(Object.create({
		language:'js',
		enable_warnings:1,
		templates:[]
	}),options);
	assert((options.language)=='js');
	let templates_by_class=__node_class_names.map(name=>[]);
	for(let t of options.templates){
		templates_by_class[t.nd.node_class].push(t);
	}
	function dfsTranslate(nd){
		let utag=nd.GetUniqueTag();
		for(let t of templates_by_class[nd.node_class]){
			//TODO: hooks, sets and checks
			//TODO: we need several types of hooks: get value, update value, check value, arbitrary check
		}
		if(nd.node_class==N_FUNCTION||nd.node_class==N_CLASS){
			//a function, a context
			let nd_params=undefined;
			let params_default=[];
			if(nd.node_class==N_FUNCTION){
				let params=[];
				let id=0;
				for(let nd_param of nd.c.s.children){
					if(nd_param.node_class==N_ASSIGNMENT){
						let nd_def=FindDef(nd_param.c);
						if(nd_def){
							let name=nd_def.GetName();
							params.push(@(
								Sandbox.Declare(ctx,@(nString(name)),@(nString(nd_def.GetUniqueTag())));
								if(params.length<@(nNumber(id.toString()))){
									Sandbox.Assign(ctx,@(nString(name)),params[@(nNumber(id.toString()))]);
								}
							))
						}
						params_default.push(dfsTranslate(nd_param.c.s));
					}else{
						//default value: 
						params_default.push(@(Sandbox.DummyValue(ctx,@(nString(utag)))));
					}
					id+=1;
				}
				nd_params=nScope.apply(null,params);
			}else{
				//TODO: inherit base class contexts
				//we're not *that* dependent on classes in JS mode
				nd_params=nAir();
			}
			let nd_default_params=nRaw.apply(null,params_default).setFlags(0x5d5b/*[]*/);
			let utag_body=nd.LastChild().GetUniqueTag();
			//TODO: declare return value when needed
			//QueueCall should return the function object
			let nd_func=@(Sandbox.QueueCall(function(params){
				let _ctx_outer=ctx;
				try{
					let ctx=Sandbox.GetScopeByTag(_ctx_outer,@(nString(utag_body)));
					@(nd_params);
					//reach memorization - it throws
					Sandbox.Reach(ctx,@(nString(utag_body)));
					@(dfsTranslate(nd.LastChild()));
				}finally(){
					Sandbox.EndScope(ctx);
				}
			},@(nd_default_params)));
			//.enclose('[]')
			let nd_def=nd.node_class==N_FUNCTION?FindDef(nd.c):nd.c.s;
			if(nd_def){
				let name=nd_def.GetName();
				nd_func=@(
					Sandbox.Declare(ctx,@(nString(name)),@(nString(nd_def.GetUniqueTag())));
					Sandbox.Assign(ctx,@(nString(name)),@(nd_func));
				);
			}
			return nd_func;
		}
		if(nd.node_class==N_SCOPED_STATEMENT){
			//TODO: reach memorization
			if(nd.data=='if'){
				//TODO: actual if, but exploration - record age-based exploration states
				//store exploration status by addr
				//CFG contexts need to age their parents
				let nd_if=@(if(Sandbox.Explore(ctx,@(nString(utag)),2,@(dfsTranslate(nd.c)))===0){
					Sandbox.Reach(ctx,@(nString(nd.c.s.GetUniqueTag())));
					@(dfsTranslate(nd.c.s));
				});
				let nd_else=nd.c.s.s;
				if(nd_else){
					//TODO: else
				}
			}else if(nd.data=='for'){
				//TODO: init / cond / update...
			}else if(nd.data=='while'){
				//TODO
			}else{
				if(options.enable_warnings){
					console.error('unimplemented statement:',nd.data)
				}
			}
		}
		if(nd.node_class==N_KEYWORD_STATEMENT){
			if(nd.data=='break'||nd.data=='continue'){
				return nKeywordStatement(nd.data);
			}else if(nd.data=='return'){
				//TODO: assume Reach created at the target
				//TODO: assign return value
			}else{
				if(options.enable_warnings){
					console.error('unimplemented statement:',nd.data)
				}
			}
		}
		if(nd.node_class==N_CALL){
			//TODO: QueueCall, register return value dependency
			//in JS, the dependency will be rough: age the context 
		}
		if(nd.node_class==N_SCOPE){
			//TODO: new context, EndScope
		}
		//COULDDO: && / ||
		//TODO: generic assignments / declarations should always happen: they could carry states
		//but they need to age the context, they'll never be normal assignments
		//DummyValue is the way to go
	}
	let nd_gen=dfsTranslate(nd_root);
	//TODO: reset and hook merge / dtor binding
	//TODO: return final node
};
