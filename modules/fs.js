//DO NOT use ama features: this is a "chicken" file which gets called when formatting other JS files
'use strict'
let fs = module.exports;

let Buffer = {
	toString:__buffer_toString
};
Buffer.__proto__ = ArrayBuffer;

fs.readFileSync = function(fn) {
	let ret = __readFileSync(fn);
	if (ret) {
		ret.__proto__ = Buffer;
	}
	return ret;
}
fs.existsSync = __existsSync;
fs.writeFileSync = __writeFileSync;
fs.statSync = __statSync;

fs.Dirent=function(item){
	Object.assign(this,item);
}

fs.Dirent.prototype.isDirectory=function(){return this.is_dir;}
fs.Dirent.prototype.isFile=function(){return this.is_file;}

fs.readdirSync=function(dir,options){
	if(!options||options.withFileTypes!==true){
		throw new Error('{withFileTypes:true} is mandatory');
	}
	return JSON.parse(__readdirSync(dir)).map(item=>new fs.Dirent(item));
}
