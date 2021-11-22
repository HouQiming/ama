//Extra file system utility.
//
//Provided methods:
//- `fsext.FindAllFiles(dir)`: recursively find files in dir and return an array of absolute paths
//- `fsext.SyncTimestamp(fn_src, fn_tar)`: make the timestamps of `fn_src` and `fn_tar` identical
'use strict'
//DO NOT use ama features: this is a "chicken" file which gets called when formatting other JS files
let fsext = module.exports;
const fs=require('fs');
const path=require('path');

function RecursiveFileSearch(ret,dir){
	for(let ent of fs.readdirSync(dir,{withFileTypes:true})){
		if(ent.name==='.'||ent.name==='..'){continue;}
		ent.name=path.join(dir,ent.name);
		if(ent.isFile()){
			ret.push(ent.name);
		}else if(ent.isDirectory()){
			RecursiveFileSearch(ret,ent.name);
		}
	}
	return ret;
}

fsext.FindAllFiles=function(dir){
	return RecursiveFileSearch([],dir);
}

//bidirectional synchronization will only stop when the timestamps are exactly equal
//so we need SyncTimestamp to achieve that
fsext.SyncTimestamp=function(fn_src,fn_tar){
	__global.__SyncTimestamp(fn_src,fn_tar);
}
