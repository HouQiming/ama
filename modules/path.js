//DO NOT use ama features: this is a "chicken" file which gets called when formatting other JS files
'use strict'
let path = module.exports;

path.join = function(...args) {
	return __path_normalize(args.join('/'));
}

path.resolve = function(...args) {
	let ret=[];
	for(let fn of args){
		if(__path_isAbsolute(fn)){ret.length=0;}
		ret.push(fn);
	}
	return __path_toAbsolute(ret.join('/'));
}

path.parse = function(s) {
	return JSON.parse(__path_parse(s));
}

path.dirname = function(s) {
	return path.parse(s).dir;
}

path.basename = function(s) {
	return path.parse(s).base;
}

path.extname = function(s) {
	return path.parse(s).ext;
}

path.isAbsolute = __path_isAbsolute;
path.relative = __path_relative;
