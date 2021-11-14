//DO NOT use ama features: this is a "chicken" file which gets called when formatting other JS files
'use strict'
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
	//if(!__global.__readdirSync){
	//	//old version migration
	//	const pipe=require('pipe');
	//	return pipe.runPiped(process.platform == 'win32' ? 'dir /s /b "'+dir+'\\*"' + ext : "find '" + dir+"'").stdout.split('\n').filter(fn=>fn);
	//}
	return RecursiveFileSearch([],dir);
}

fsext.SyncTimestamp=function(fn_src,fn_tar){
	//if(!__global.__SyncTimestamp){
	//	const pipe=require('pipe');
	//	pipe.run(['touch -r ', JSON.stringify(fn_src), ' ', JSON.stringify(fn_tar)].join(''));
	//	return;
	//}
	__global.__SyncTimestamp(fn_src,fn_tar);
}
