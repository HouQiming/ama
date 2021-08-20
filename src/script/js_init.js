Node.then=function(f,...args){
	let ret=f.apply(null,[this].concat(args));
	if(ret===undefined){
		ret=this;
	}
	return ret;
}

__global.c_include_paths=['/usr/include','/usr/local/include']
