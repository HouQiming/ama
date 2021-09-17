'use strict';
const fs=require('fs');
const path=require('path');
let cmake=module.exports;

let cmake_options={
	enable_hash_comment:1,
	parse_indent_as_scope:0,
	parse_c_forward_declarations:0,
	struct_can_be_type_prefix:0,
}

cmake.LoadCMakeFile=function(fn,template){
	let nd_root=ParseCode((fs.readFileSync(fn)||template||'').toString(),cmake_options);
	//if-endif pairing
	while(nd_root.c&&!nd_root.c.s&&nd_root.c.node_class===N_RAW&&!(nd_root.c.flags&0xffff)&&nd_root.c.c){
		nd_root.c.ReplaceWith(nd_root.c.c)
	}
	let if_stk=[];
	for(let ndi=nd_root.c;ndi;ndi=ndi.s){
		if(ndi.node_class===N_CALL){
			let name=ndi.GetName();
			if(name==='if'){
				if_stk.push(ndi);
			}else if(name==='endif'&&if_stk.length>0){
				let nd0=if_stk.pop();
				let nd_body=nd0.s;
				nd0.s=ndi.s;
				ndi.s=null;
				let nd_tmp=Node.GetPlaceHolder();
				nd0.ReplaceWith(nd_tmp);
				nd0.s=nd_body;
				ndi=nd_tmp.ReplaceWith(CreateNode(N_RAW,nd0));
			}else{
				//do nothing
			}
		}
	}
	return nd_root;
}

cmake.TokenizeCMakeArgs=function(nd_call){
	let ret=[];
	if(!nd_call.c.s){return ret;}
	function dfsTokenizeCMakeArgs(nd){
		if(nd.node_class===N_RAW){
			for(let ndi=nd.c;ndi;ndi=ndi.s){
				dfsTokenizeCMakeArgs(ndi);
			}
		}else if(nd.node_class===N_STRING||nd.node_class===N_REF||nd.node_class===N_NUMBER||nd.node_class===N_SYMBOL){
			ret.push(nd)
		}else{
			console.error('unrecognized cmake arg:',nd);
			ret.push(nd)
		}
	}
	dfsTokenizeCMakeArgs(nd_call.c.s);
	return ret;
}

cmake.FindTarget=function(nd_root,name){
	for(let nd_target of nd_root.FindAll(N_CALL,'add_executable').concat(nd_root.FindAll(N_CALL,'add_library'))){
		if(!nd_target.c.s){continue;}
		let args=cmake.TokenizeCMakeArgs(nd_target);
		if(args.length&&args[0].node_class===N_REF&&args[0].data===name){
			//found
			return nd_target;
		}
	}
	return null;
}

cmake.CreateTarget=function(nd_root,name,options){
	if(!options){
		options={};
	}
	let nd_target=cmake.FindTarget(nd_root,name);
	if(nd_target){return nd_target;}
	//create a new target
	let new_target=['\n'];
	let output_format=options.format||'exe';
	if(output_format==='exe'){
		new_target.push('add_executable(',target);
	}else if(output_format==='dll'){
		new_target.push('add_library(',target,' SHARED');
	}else if(output_format==='lib'){
		new_target.push('add_library(',target)
	}else{
		throw new Error('invalid output format '+output_format)
	}
	let files=options.files||[];
	for(let fn:files){
		new_target.push('\n  ',fn)
	}
	new_target.push('\n)')
	return nd_root.Insert(POS_BACK,ParseCode(new_target.join(''),cmake_options).Find(N_CALL,null))
}

//TODO: smart detection - also search for existing targets with the current file
//TODO: CMakeLists.txt searching, on-demand creation, make a filter
cmake.UpdateTargetWithSourceRoot=function(nd_root,nd_src_root){
	let name=path.parse(nd_src_root.data).name;
	let nd_target=cmake.FindTarget(nd_root,name);
	if(nd_target){return nd_target;}
	let format=undefined;
	//TODO: search pragma for format - nd_src_root
	//TODO: #pragma statement: they are important data holders... or naked string? works for JS and Python
	return cmake.CreateTarget(nd_root,name,{
		format:format,
		files:depends.ListAllDependency(nd_src_root,false)
	})
}
