#!/usr/bin/env ama
'use strict';
const pipe=require('pipe');
const path=require('path');
const fs=require('fs');
const fsext=require('fsext');
const bisync=require('bisync');
const jsism=require('cpp/jsism');

function FixAMAJS(){
	//minimize the `require` here: we are *modifying* the js files which we'll probably `require` later
	const auto_paren=require('auto_paren');
	const auto_semicolon=require('auto_semicolon');
	process.chdir(path.join(__dirname,'../modules'));
	//for(let fn_rel_amajs of pipe.runPiped(process.platform==='win32'?'dir /s /b *.ama.js':"find -iname '*.ama.js'").stdout.split('\n'))
	for(let fn_rel_amajs of fsext.FindAllFiles(path.join(__dirname,'../modules'))){
		//if(!fn_rel_amajs){continue;}
		if(!fn_rel_amajs.endsWith('.ama.js')){continue;}
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
bisync({
	dir_src:path.resolve(__dirname,'../src'),
	features:[
		'StripRedundantPrefixSpace',
		require('auto_semicolon'),
		require('cpp/sane_for'),
		require('auto_paren'),
		require('cpp/sane_types').FixArrayTypes,
		require('cpp/auto_decl'),
		require('cpp/typing').DeduceAuto,
		require('cpp/auto_dot'),
		require('cpp/cpp_indent'),
		'Save',
		///////////////
		require('cpp/line_sync'),
		require('cpp/short_types'),
		require('cpp/sane_types'),
		require('cpp/sane_init'),
		require('cpp/sane_export'),
		require('cpp/move_operator'),
		require('cpp/unified_null'),
		require('cpp/unified_null'),
		jsism.EnableJSLambdaSyntax,
		jsism.EnableJSON,
		jsism.EnableConsole,
		jsism.EnableSingleQuotedStrings,
		require('cpp/auto_header'),
		require('cpp/asset'),
		require('cpp/gentag').GeneratedCode
	]
});
require(path.join(__dirname,'docgen.ama.js'))();
