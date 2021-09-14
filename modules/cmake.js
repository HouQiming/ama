'use strict'
let cmake=module.exports;

cmake.LoadCMakeFile=function(fn,template){
	return ParseCode((fs.readFileSync(fn)||template||'').toString(),{
		enable_hash_comment:1,
		parse_indent_as_scope:0,
		parse_c_forward_declarations:0,
		struct_can_be_type_prefix:0,
	});
}

cmake.CreateTarget=function(nd_root,name,options){
	if(!options){
		options={};
	}
	for(let nd_target of nd_root.FindAll(N_CALL,'add_executable').concat(nd_root.FindAll(N_CALL,'add_library'))){
		if(!nd_target.c.s){continue;}
		//TODO: nd_target
	}
}
