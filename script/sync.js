#!/usr/bin/env ama
//starting the line with a quote will terminate the @ama
//'use strict' is the default setting anyway
const pipe=require('pipe');
const path=require('path');
const fs=require('fs');
const bisync=require('bisync')

function FixAMAJS(){
	//minimize the `require` here: we are *modifying* the js files which we'll probably `require` later
	const auto_paren=require('auto_paren');
	const auto_semicolon=require('auto_semicolon');
	process.chdir(path.join(__dirname,'../modules'));
	for(let fn_rel_amajs of pipe.runPiped(process.platform==='win32'?'dir /s /b *.ama.js':"find -iname '*.ama.js'").stdout.split('\n')){
		if(!fn_rel_amajs){continue;}
		let fn=path.resolve(__dirname,'../modules',fn_rel_amajs);
		let nd_root=ParseCode(fs.readFileSync(fn));
		if(nd_root){
			nd_root.data=fn;
			for(let nd of nd_root.FindAll(N_BINOP,'!==')){
				nd.data='!=';
			}
			for(let nd of nd_root.FindAll(N_BINOP,'===')){
				nd.data='==';
			}
			nd_root.then(auto_paren).then(auto_semicolon).Save();
		}
	}
	//Node.gc();
}

FixAMAJS();
bisync({dir_src:path.resolve(__dirname,'../src')});
