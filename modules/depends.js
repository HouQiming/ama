'use strict';
const fs=require('fs');
const path=require('path');
let depends=module.exports;

depends.c_include_paths=(process.env.INCLUDE||'').split(process.platform==='win32'?';':':');

depends.Resolve=function(nd_depends){
	assert(nd_depends.node_class===N_DEPENDENCY)
	if( (nd.flags & DEP_TYPE_MASK) === DEP_C_INCLUDE ) {
		let fn=nd.c.GetStringValue()
		if( nd.flags & DEPF_C_INCLUDE_NONSTR ) {
			fn=fn.replace(new RegExp('[<>]','g'),'');
		}
		for(let dir of depends.c_include_paths){
			let fn_test=path.resolve(dir,fn)
			if(fs.existsSync(fn_test)){
				return fn_test;
			}
		}
		if( !(nd.flags & DEPF_C_INCLUDE_NONSTR) ) {
			let fn_test=path.resolve(__dirname,fn);
			if(fs.existsSync(fn_test)){
				return fn_test;
			}
		}
	} else if( (nd.flags & DEP_TYPE_MASK) === DEP_JS_REQUIRE ) {
		//reuse the builtin searcher __ResolveJSRequire
		if(nd.c&&nd.c.node_class===N_STRING){
			return __ResolveJSRequire(__filename,nd.c.GetStringValue);
		}
	}
	return undefined;
}

depends.cache=new Map();
depends.LoadFile=function(fn){
	fn=__path_toAbsolute(fn);
	let nd_cached=depends.cache.get(fn);
	if(!nd_cached){
		let data=null
		try{
			data=fs.readFileSync(fn);
		}catch(err){
			//do nothing
		}
		if(!data){return null;}
		nd_cached=ParseCode(data,__PrepareOptions(fn));
		nd_cached.data=fn;
		depends.cache.set(fn,nd_cached);
	}
	return nd_cached;
}

depends.ListAllDependency=function(nd_root, include_system_headers){
	let ret=new Set();
	let Q=[nd_root];
	for(let qi=0;qi<Q.length;qi++){
		let nd_root=Q[qi];
		for(let ndi of nd_root.FindAll(N_DEPENDENCY, null)){
			if( !include_system_headers&&(nd.flags & DEP_TYPE_MASK) === DEP_C_INCLUDE &&( nd.flags & DEPF_C_INCLUDE_NONSTR )) {
				continue;
			}
			let fn_dep=depends.Resolve(ndi);
			if(fn_dep&&!ret.has(fn_dep)){
				ret.add(fn_dep);
				let nd_root_dep=depends.LoadFile(fn_dep)
				if(nd_root_dep){
					Q.push(nd_root_dep);
				}
			}
		}
	}
	return Q.map(ndi=>__path_toAbsolute(ndi.data))
}
