'use strict'
//run this from ama
const fs=require('fs');
const path=require('path');
const assert=require('assert');
const depends=require('depends');
const cmake=require('cmake');

function MigrateProject(fn,patches){
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
	let made_dirs=new Set();
	let cmakelists=[];
	let base_to_prj=new Map();
	for(let fn of fs.readFileSync('/tmp/files.txt').toString().split('\n')){
		if(!fn){continue;}
		let ext=path.extname(fn);
		if(ext!=='.jc'&&ext!=='.jch'||fn.startsWith('test/')){
			//copy all non-jc
			let dir_to_make=path.dirname(path.join(dir_target,fn));
			if(!made_dirs.has(dir_to_make)){
				script.push('mkdir -p ',JSON.stringify(dir_to_make),'\n');
				made_dirs.add(dir_to_make);
			}
			script.push('cp ',JSON.stringify(path.join(dir,fn)),' ',JSON.stringify(path.join(dir_target,fn)),'\n');
			if(path.basename(fn)==='CMakeLists.txt'){
				cmakelists.push(path.join(dir_target,fn));
			}
		}
		base_to_prj.set(path.basename(fn),fn);
	}
	let cpps_to_translate=[];
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
			script.push("sed '/#line/d;s%\\.jc\\.cpp%.cpp%g;s%\\.jch\\.hpp%.hpp%g' ",JSON.stringify(path.join(dir,fn_cpp)),' > ',JSON.stringify(path.join(dir_target,fn_just_cpp)),'\n');
			cpps_to_translate.push(path.join(dir_target,fn_just_cpp));
		}
	}
	script.push("sed '/#line/d;s%\\.jc\\.cpp%.cpp%g;s%\\.jch\\.hpp%.hpp%g' ",JSON.stringify(path.join(dir,'src/entry/ama.jc.cpp')),' > ',JSON.stringify(path.join(dir_target,'src/entry/ama.cpp')),'\n');
	cpps_to_translate.push(path.join(dir_target,'src/entry/ama.cpp'));
	let fn_script='/tmp/migrate_'+prj_name+'.sh';
	fs.writeFileSync(fn_script,script.join(''));
	__system('sh '+fn_script)
	////////
	//CMakeLists fixing - after the script
	script=['#!/bin/sh\n'];
	let required_jc_libs=[];
	for(let fn of cmakelists){
		let dir_cmake=path.dirname(fn);
		let deleted_targets=new Set();
		let nd_cmake=cmake.LoadCMakeFile(fn);
		for(let nd of nd_cmake.FindAll(N_CALL,'add_custom_command')){
			nd.Unlink();
		}
		let keep_cmakelists=0;
		for(let nd_exe of nd_cmake.FindAll(N_CALL,'add_executable').concat(nd_cmake.FindAll(N_CALL,'add_library'))){
			let args=nd_exe.TokenizeCMakeArgs();
			if(!args[0].isRef('amal')&&!args[0].isRef('ama')){
				deleted_targets.add(args[0].GetName())
				nd_exe.Unlink();
				continue;
			}
			keep_cmakelists=1;
			for(let ndi of args){
				if(ndi.node_class===N_STRING){
					let fn_src=ndi.GetStringValue();
					if(fn_src.startsWith('${JC_LIB}/')){
						//set in-project files to project path
						let fn_prj=base_to_prj.get(path.basename(fn_src));
						if(fn_prj){
							fn_src=path.relative(dir_cmake,path.join(dir,fn_prj));
						}
					}
					if(fn_src.startsWith('${JC_LIB}/')){
						//we need the raw .jc.cpp / .jch.hpp names here
						required_jc_libs.push(fn_src.substr(10));
					}else{
						assert(!fn_src.startsWith('$'));
						//path adjustion for move to root
						fn_src=path.relative(dir_target,path.resolve(dir_cmake,fn_src))
					}
					if(fn_src.endsWith('.jc.cpp')){
						fn_src=fn_src.replace(/\.jc\./,'.')
					}else if(fn_src.endsWith('.jch.hpp')){
						fn_src=fn_src.replace(/\.jch\./,'.')
					}
					ndi.data=fn_src;
				}
			}
		}
		for(let nd_property of nd_cmake.FindAll(N_CALL,'set_property').concat(nd_cmake.FindAll(N_CALL,'add_custom_target'))){
			let keep=0;
			for(let nd_tgt of nd_property.FindAll(N_REF,null)){
				if(nd_tgt.data==='amal'||nd_tgt.data==='ama'){
					keep=1;
					break;
				}
				if(nd_tgt.data==='APPEND'){
					break;
				}
			}
			if(!keep){
				nd_property.Unlink();
			}
		}
		//drop the JC-specific shenanigans
		for(let nd_if of nd_cmake.FindAll(N_CALL,'if')){
			if(nd_if.p.node_class!==N_RAW){continue;}
			if(nd_if.Find(N_REF,'NOT')){
				nd_if.p.Unlink();
			}
		}
		for(let nd of nd_cmake.FindAll(N_CALL,'check_cxx_source_compiles').concat(nd_cmake.FindAll(N_CALL,'include'))){
			nd.Unlink();
		}
		for(let nd of nd_cmake.FindAll(N_REF,'COND_JC_OS_JC_OS_WINDOWS_614D399C').concat(nd_cmake.FindAll(N_REF,'COND_DEFINED_WIN32__FD28C5A6'))){
			nd.ReplaceWith(nRef('WIN32'));
		}
		for(let nd of nd_cmake.FindAll(N_REF,'COND__JC_OS_JC_OS_WINDOWS__1DB9171C').concat(nd_cmake.FindAll(N_REF,'COND__DEFINED_WIN32__A7217375'))){
			nd.ReplaceWith(nRaw(nRef('NOT'),nRef('WIN32').setCommentsBefore(' ')));
		}
		//set JC_LIB to the local copy
		nd_cmake.Find(N_CALL,'project').Insert(POS_AFTER,ParseCode(
			'\nset(JC_LIB "${CMAKE_CURRENT_SOURCE_DIR}/jc_lib")'
		).Find(N_CALL,null));
		nd_cmake.Save(cmake.options);
		if(keep_cmakelists){
			script.push('mv ',JSON.stringify(fn),' ',JSON.stringify(path.join(dir_target,'CMakeLists.txt')),'\n');
		}else{
			script.push('rm ',JSON.stringify(fn),'\n');
		}
	}
	//recursively expand required_jc_libs
	let dir_jc_lib=path.resolve(__dirname,'../../jc3/lib');
	//language-level dependencies
	required_jc_libs.push('jc_string_search.h','jc_polyfill.h','jc_array_algorithm.h')
	let required_jc_libs_set=new Set([required_jc_libs]);
	for(let fn of depends.ListAllDependency(nd_root,false)){
		let fn_rel=path.relative(dir_jc_lib,fn);
		if(!fn_rel.startsWith('.')&&!path.isAbsolute(fn_rel)&&!required_jc_libs_set.has(fn_rel)){
			required_jc_libs_set.add(fn_rel);
			required_jc_libs.push(fn_rel);
		}
	}
	for(let fn_jc_lib of required_jc_libs){
		let nd_root_jc_lib=depends.LoadFile(path.join(dir_jc_lib,fn_jc_lib));
		for(let fn of depends.ListAllDependency(nd_root_jc_lib,false)){
			let fn_rel=path.relative(dir_jc_lib,fn);
			if(!fn_rel.startsWith('.')&&!path.isAbsolute(fn_rel)&&!required_jc_libs_set.has(fn_rel)){
				required_jc_libs_set.add(fn_rel);
				required_jc_libs.push(fn_rel);
			}
		}
	}
	//create the JC_LIB local copy
	let do_not_copy=new Set([
		'jc_array.h',
		'jc_string_search.h',
		'jc_polyfill.h',
		'jc_array_algorithm.h'
	]);
	for(let fn of required_jc_libs){
		if(do_not_copy.has(path.basename(fn))){continue;}
		let fn_target=path.resolve(dir_target,'jc_lib',fn.replace('.jc.','.').replace('.jch.','.'));
		let dir_to_make=path.dirname(fn_target);
		if(!made_dirs.has(dir_to_make)){
			script.push('mkdir -p ',JSON.stringify(dir_to_make),'\n');
			made_dirs.add(dir_to_make);
		}
		script.push("sed '/#line/d;s%\\.jc\\.cpp%.cpp%g;s%\\.jch\\.hpp%.hpp%g' ",JSON.stringify(path.join(dir_jc_lib,fn)),' > ',JSON.stringify(fn_target),'\n');
		cpps_to_translate.push(fn_target);
	}
	//patch it
	for(let key in patches){
		let patch=patches[key];
		let fn=path.resolve(dir_target,key);
		if(typeof(patch)==='string'){
			fs.writeFileSync(fn,patch);
		}else if(typeof(patch)==='object'&&patch.append){
			fs.writeFileSync(fn,fs.readFileSync(fn).toString()+('\n'+patch.append));
		}else if(typeof(patch)==='object'&&patch.from){
			script.push('mv ',JSON.stringify(path.resolve(dir_target,patch.from)),' ',JSON.stringify(fn),'\n')
		}else if(typeof(patch)==='object'&&patch.rm){
			script.push('rm ',JSON.stringify(fn),'\n');
		}else{
			console.log('invalid patch for',key)
		}
	}
	//run script
	fn_script='/tmp/migrate_'+prj_name+'_jc_lib.sh';
	fs.writeFileSync(fn_script,script.join(''));
	__system('sh '+fn_script);
	//back-translate cpps to .ama.cpp
	let inv_sane_types=require('cpp/sane_types').inverse;
	let inv_sane_init=require('cpp/sane_init').inverse;
	let jc_things=new Set();
	let mmdots=new Set([
		'data','push','pop',
		'startsWith','endsWith','indexOf','lastIndexOf',
		'subarray','sortby','fill','concat','unique',
		'map_get','map_set'
	]);
	let obsolete_headers=new Set([
		'jc_string_search.h',
		'jc_polyfill.h',
		'jc_array_algorithm.h'
	]);
	for(let fn_cpp of cpps_to_translate){
		let nd_root=depends.LoadFile(fn_cpp);
		if(!nd_root){continue;}
		//replace JC::foo methods with the new ---> mechanism
		for(let nd_dot of nd_root.FindAll(N_DOT,null)){
			if(!nd_dot.c.isRef('JC')){continue;}
			let name=nd_dot.data;
			if(mmdots.has(name)&&nd_dot.p.node_class===N_CALL&&nd_dot.p.c===nd_dot&&nd_dot.s){
				let nd_call=nd_dot.p;
				let nd_obj=nd_call.c.s;
				let nd_args=nd_obj.BreakSibling();
				nd_obj.BreakSelf();
				if(!(nd_obj.node_class===N_CALL&&nd_obj.GetName()==='push')&&name!=='data'){
					nd_obj=nPostfix(nd_obj,'--');
				}
				if(name.startsWith('map')){name=name.substr(4);}
				if(nd_args){nd_args.setCommentsBefore('');}
				let nd_new_call=nd_call.ReplaceWith(nd_obj.dot(name).setFlags(name==='data'?0:DOT_PTR).call(nd_args));
				if(name==='push'&&nd_new_call.p.node_class===N_DOT&&nd_new_call.p.p.node_class===N_CALL&&nd_new_call.p.p.GetName()==='push'){
					let nd_parent_call=nd_new_call.p.p;
					nd_parent_call.ReplaceWith(nd_new_call.BreakSelf());
					nd_new_call.Insert(POS_BACK,nd_parent_call.c.s);
				}
				continue;
			}
			jc_things.add(nd_dot.toSource().trim());
		}
		//replace JC_LIB files with in-project alternatives
		for(let nd_dep of nd_root.FindAll(N_DEPENDENCY,null)){
			if(nd_dep.c.GetStringValue()==='json.hpp'){
				nd_dep.c.ReplaceWith(nString(
					path.relative(path.dirname(fn_cpp),path.join(dir_target,'modules/cpp/json/json.h'))
				));
				continue;
			}
			let fn=depends.Resolve(nd_dep);
			if(fn&&fn.startsWith(dir_jc_lib+'/')){
				let fn_base=path.basename(fn);
				if(obsolete_headers.has(fn_base)){
					nd_dep.Unlink();
					continue;
				}
				let fn_prj=base_to_prj.get(fn_base);
				if(fn_prj){
					nd_dep.c.ReplaceWith(nString(
						path.relative(path.dirname(fn_cpp),path.join(dir_target,fn_prj))
					));
					continue;
				}
				console.log(fn_base,'not found');
			}
		}
		nd_root.Save();
	}
	jc_things.forEach(src=>{
		console.log('TODO: untranslate',src)
	});
	//default_options.is_migrating=1;
	ProcessAmaFile(path.resolve(dir_target,'trans/sync.js'));
}

function main(){
	let dir_jc_lib=path.resolve(__dirname,'../../jc3/lib');
	depends.c_include_paths.push(dir_jc_lib);
	MigrateProject(path.join(__dirname,'../src/entry/amal.jc'),{
		'src/script/jsgen.ama.cpp':[
			"//@ama require('./jsgen.js')('ama',ParseCurrentFile()).Save('.cpp');\n",
			'#include "./jsenv.hpp"\n',
			'#pragma gen_begin(js_bindings)\n'
		].join(''),
		'src/script/jsgen.cpp':{rm:1}
	});
	//MigrateProject(path.join(__dirname,'../../laas/src/core/net0822.jc'),{})
}

module.exports=main;
