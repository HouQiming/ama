'use strict'
//run this from ama
const fs=require('fs');
const path=require('path');
const depends=require('depends');

function MigrateProject(fn){
	fn=path.resolve(fn);
	let dir=path.dirname(fn);
	while(!fs.existsSync(path.join(dir,'.git'))){
		let dir_upper=path.dirname(dir);
		if(dir===dir_upper){
			throw new Error('failed to find git repo')
		}
		dir=dir_upper;
	}
	let prj_name=path.basename(dir);
	let dir_target=path.join(path.dirname(dir),'cpp_'+prj_name);
	console.log('Migrating',dir,'to',dir_target,'based on',path.relative(dir,fn));
	////////////
	let nd_root=depends.LoadFile(fn);
	let script=['#!/bin/sh\n'];
	__system(['cd ',JSON.stringify(dir),' ; git ls-files > /tmp/files.txt'].join(''));
	let made_dirs=new Set()
	for(let fn of fs.readFileSync('/tmp/files.txt').toString().split('\n')){
		if(!fn){continue;}
		let ext=path.extname(fn);
		if(ext!=='.jc'&&ext!=='.jch'){
			//copy all non-jc
			let dir_to_make=path.dirname(path.join(dir_target,fn));
			if(!made_dirs.has(dir_to_make)){
				script.push('mkdir -p ',JSON.stringify(dir_to_make),'\n');
				made_dirs.add(dir_to_make);
			}
			script.push('cp ',JSON.stringify(path.join(dir,fn)),' ',JSON.stringify(path.join(dir_target,fn)),'\n')
		}
	}
	for(let fn of depends.ListAllDependency(nd_root,false)){
		//copy .cpp or .hpp for jc, and replace file names
		let fn_rel=path.relative(dir,fn);
		if(fn_rel.startsWith('.')){continue;}
		if(path.isAbsolute(fn_rel)){continue;}
		let ext=path.extname(fn_rel);
		if(ext==='.jc'||ext==='.jch'){
			let dir_to_make=path.dirname(path.join(dir_target,fn_rel));
			if(!made_dirs.has(dir_to_make)){
				script.push('mkdir -p ',JSON.stringify(dir_to_make),'\n');
				made_dirs.add(dir_to_make);
			}
			let fn_cpp=fn_rel+(ext==='.jc'?'.cpp':'.hpp');
			let fn_just_cpp=(ext==='.jc'?fn_rel.substr(0,fn_rel.length-3)+'.cpp':fn_rel.substr(0,fn_rel.length-4)+'.hpp')
			script.push("sed 's%\\.jc\\.cpp%.cpp%g;s%\\.jch\\.hpp%.hpp%g' ",JSON.stringify(path.join(dir,fn_cpp)),' > ',JSON.stringify(path.join(dir_target,fn_just_cpp)),'\n')
		}
	}
	fs.writeFileSync('/tmp/migrate.sh',script.join(''));
}

function main(){
	MigrateProject(path.join(__dirname,'../src/entry/ama.jc'));
	MigrateProject(path.join(__dirname,'../../laas/src/core/net0822.jc'))
}

module.exports=main;
