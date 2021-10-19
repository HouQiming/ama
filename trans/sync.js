/*
@ama //*/
//starting the line with a quote will terminate the @ama
;'use strict';
const pipe=require('pipe');
const path=require('path');
const fs=require('fs');

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
	Node.gc();
}

function FixAMACPP(){
	function GetFileTime(fn){
		if(!fs.existsSync(fn)){return 0;}
		let stat=fs.statSync(fn);
		return stat.mtimeMs;
	}
	process.chdir(path.join(__dirname,'../src'));
	let script_ama2cpp=fs.readFileSync(path.join(__dirname,'from_ama.js')).toString();
	let script_cpp2ama=fs.readFileSync(path.join(__dirname,'to_ama.js')).toString();
	let all_cpp_files=new Set();
	for(let fn_rel_cpp of pipe.runPiped(process.platform==='win32'?'dir /s /b *.cpp':"find -iname '*.cpp'").stdout.split('\n')){
		if(!fn_rel_cpp){continue;}
		all_cpp_files.add(path.resolve(__dirname,'../src',fn_rel_cpp.replace(/\.ama\.cpp$/,'.cpp')));
	}
	for(let fn_rel_hpp of pipe.runPiped(process.platform==='win32'?'dir /s /b *.hpp':"find -iname '*.hpp'").stdout.split('\n')){
		if(!fn_rel_hpp){continue;}
		all_cpp_files.add(path.resolve(__dirname,'../src',fn_rel_hpp.replace(/\.ama\.hpp$/,'.hpp')));
	}
	let aba_test_jobs=[];
	for(let fn_cpp of all_cpp_files){
		Node.gc();
		let fn_ama_cpp=fn_cpp.replace(/\.cpp$/,'.ama.cpp').replace(/\.hpp$/,'.ama.hpp');
		let t_cpp=GetFileTime(fn_cpp);
		let t_ama=GetFileTime(fn_ama_cpp);
		if(t_ama<t_cpp){
			//cpp to ama
			ProcessAmaFile(fn_cpp,script_cpp2ama);
			aba_test_jobs.push([fn_ama_cpp,script_ama2cpp]);
			pipe.run(['touch -r ',JSON.stringify(fn_cpp),' ',JSON.stringify(fn_ama_cpp)].join(''));
			console.log('updated',fn_ama_cpp);
		}else if(t_cpp<t_ama){
			//ama to cpp
			ProcessAmaFile(fn_ama_cpp,script_ama2cpp);
			aba_test_jobs.push([fn_cpp,script_cpp2ama]);
			pipe.run(['touch -r ',JSON.stringify(fn_ama_cpp),' ',JSON.stringify(fn_cpp)].join(''));
			console.log('updated',fn_cpp);
		}
	}
}

__dirname=path.resolve(__dirname);
FixAMAJS();
FixAMACPP();
