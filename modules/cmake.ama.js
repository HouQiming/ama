'use strict'
//@ama ParseCurrentFile().then(require("jcs").TranslateJCS)
const fs = require('fs');
const path = require('path');
const depends = require('depends');
let cmake = module.exports;

//as a new language support module, cmake.jcs extends Node
//a small number will be interpreted as the opening char
const CMAKE_CHANGED = 65536;

let cmake_options = {
	enable_hash_comment: 1,
	parse_indent_as_scope: 0,
	parse_c_forward_declarations: 0,
	parse_js_regexp: 0,
	struct_can_be_type_prefix: 0,
	keywords_scoped_statement: '',
	//////////
	tab_indent: 2,//2 for auto
	auto_space: 0,
}

cmake.LoadCMakeFile = function(fn, template) {
	let flags = 0;
	let nd_root = ParseCode((fs.readFileSync(fn) || (flags = CMAKE_CHANGED,typeof(template) == 'function' ? template() : template) || '').toString(), cmake_options);
	//if-endif pairing
	while (nd_root.c && !nd_root.c.s && nd_root.c.node_class == N_RAW && !(nd_root.c.flags & 0xffff) && nd_root.c.c) {
		nd_root.c.ReplaceWith(nd_root.c.c)
	}
	nd_root.flags |= flags;
	nd_root.data = fn;
	let if_stk = [];
	for (let ndi = nd_root.c; ndi; ndi = ndi.s) {
		if (ndi.node_class == N_CALL) {
			let name = ndi.GetName();
			if (name == 'if') {
				if_stk.push(ndi);
			} else if(name == 'endif' && if_stk.length > 0) {
				let nd0 = if_stk.pop();
				let nd_tmp = Node.GetPlaceHolder();
				nd0.ReplaceUpto(ndi, nd_tmp)
				ndi = nd_tmp.ReplaceWith(CreateNode(N_RAW, nd0));
			} else {
				//do nothing
			}
		}
	}
	return nd_root;
}

Node.TokenizeCMakeArgs = function() {
	let nd_call = this;
	let ret = [];
	if (!nd_call.c.s) {return ret;}
	let merge_group = [];
	function FlushMergeGroup() {
		//unquoted args - merge and convert. quoting individual names in target is fine
		if (merge_group.length == 1 && merge_group[0].node_class == N_REF) {
			//leave single identifiers alone
			ret.push(merge_group[0])
			merge_group.length = 0;
		} else {
			let n0 = ret.length;
			for (let str of merge_group.map(nd=>nd.toSource()).join('').trim().split(' ')) {
				ret.push(nString(str).setCommentsBefore(' '))
			}
			merge_group[0].ReplaceUpto(merge_group[merge_group.length - 1], nScope.apply(null, ret.slice(n0)).c)
			merge_group.length = 0;
		}
	}
	function dfsTokenizeCMakeArgs(nd) {
		if (merge_group.length && nd.comments_before.length > 0) {
			FlushMergeGroup();
		}
		if (nd.node_class == N_RAW && !(nd.flags & 0xffff)) {
			for (let ndi = nd.c; ndi; ndi = ndi.s) {
				dfsTokenizeCMakeArgs(ndi);
			}
		} else if(nd.node_class == N_STRING) {
			if (merge_group.length) {
				FlushMergeGroup();
			}
			ret.push(nd);
		} else if(nd.node_class == N_REF || nd.node_class == N_NUMBER || nd.node_class == N_SYMBOL) {
			merge_group.push(nd);
		} else {
			//console.error('unrecognized cmake arg:', nd);
			//ret.push(nd)
			merge_group.push(nd);
		}
	}
	for (let ndi = nd_call.c.s; ndi; ndi = ndi.s) {
		dfsTokenizeCMakeArgs(ndi);
	}
	if (merge_group.length) {
		FlushMergeGroup();
	}
	return ret;
}

Node.CMakeFindTarget = function(name) {
	let nd_root = this;
	for (let nd_target of nd_root.FindAll(N_CALL, 'add_executable').concat(nd_root.FindAll(N_CALL, 'add_library'))) {
		if (!nd_target.c.s) {continue;}
		let args = nd_target.TokenizeCMakeArgs();
		if (args.length && args[0].node_class == N_REF && args[0].data == name) {
			//found
			return nd_target;
		}
	}
	return null;
}

Node.CMakeCreateTarget = function(name, options) {
	let nd_root = this;
	if (!options) {
		options = {};
	}
	//create a new target
	let new_target = ['\n'];
	let output_format = options.format;
	if (output_format == 'exe') {
		new_target.push('add_executable(', name);
	} else if(output_format == 'dll') {
		new_target.push('add_library(', name, ' SHARED');
	} else if(output_format == 'lib') {
		new_target.push('add_library(', name)
	} else {
		throw new Error('invalid output format ' + output_format)
	}
	let files = options.files || [];
	for (let fn of files) {
		new_target.push('\n  ', JSON.stringify(fn))
	}
	new_target.push('\n)');
	return nd_root.Insert(POS_BACK, ParseCode(new_target.join(''), cmake_options).Find(N_CALL, null));
}

function TryRelative(dir_cmake, src_name_abs) {
	let src_name_rel = path.relative(dir_cmake, src_name_abs);
	if (path.isAbsolute(src_name_rel) || src_name_rel.length > src_name_abs.length) {
		src_name_rel = src_name_abs;
	}
	if (src_name_abs.startsWith(__std_module_dir + '/')) {
		src_name_rel = '${AMA_MODULES}' + src_name_abs.substr(__std_module_dir.length);
	} else if(src_name_rel.startsWith(__std_module_dir_global + '/')) {
		src_name_rel = '${AMA_MODULES_GLOBAL}' + src_name_abs.substr(__std_module_dir_global.length);
	}
	return src_name_rel;
}

//let nd_output_format_template = ParseCode('#pragma add("output_format",.(N_STRING(format)))\n').Find(N_KEYWORD_STATEMENT, '#pragma')
let nd_output_format_template = .{#pragma add("output_format", .(Node.MatchAny(N_STRING, 'format')))};
Node.CMakeUpdateTargetWithSourceRoot = function(nd_src_root) {
	let src_name_abs = path.resolve(nd_src_root.data);
	let dir_cmake = path.dirname(path.resolve(this.data));
	let our_files = depends.ListAllDependency(nd_src_root, false)
	let our_files_filtered = [];
	let src_name_rel = TryRelative(dir_cmake, src_name_abs)
	let pama = src_name_rel.lastIndexOf('.ama');
	if (pama >= 0) {
		src_name_rel = src_name_rel.substr(0, pama) + src_name_rel.substr(pama + 4);
	}
	for (let fn of our_files) {
		if (path.extname(fn).startsWith('.h') && fn != src_name_abs) {
			//exclude all headers except the current file
			continue;
		}
		//use relative dirs whenever possible
		let fn_rel = TryRelative(dir_cmake, fn);
		let pama = fn_rel.lastIndexOf('.ama');
		if (pama >= 0) {
			fn_rel = fn_rel.substr(0, pama) + fn_rel.substr(pama + 4);
		}
		our_files_filtered.push(fn_rel)
	}
	//search for all targets containing src_name_rel, then add dependent files to them
	//COULDDO: also add to 'set' commands
	let found_any = 0;
	for (let nd_target of this.FindAll(N_CALL, 'add_executable').concat(this.FindAll(N_CALL, 'add_library'))) {
		if (!nd_target.c.s) {continue;}
		let args = nd_target.TokenizeCMakeArgs();
		let p_files = 1;
		if (args.length >= 2 && args[1].GetName() == 'SHARED') {
			p_files = 2;
		}
		let files = new Set(args.slice(p_files).map(nd=>nd.GetName()));
		if (!files.has(src_name_rel)) {continue;}
		found_any = 1;
		let new_files = our_files_filtered.filter(fn=>!files.has(fn));
		if (new_files.length) {
			//append the new files
			for (let fn of new_files) {
				this.flags |= CMAKE_CHANGED;
				nd_target.Insert(POS_BACK, nString(fn).setCommentsBefore(' '))
			}
		}
	}
	if (found_any) {
		return;
	}
	let name = nd_src_root.data.replace(/[.].*/, '');
	let format = undefined;
	for (let nd_format of nd_src_root.FindAll(N_KEYWORD_STATEMENT, '#pragma')) {
		let match = nd_format.Match(nd_output_format_template)
		if (match) {
			format = match.format.GetStringValue();
			break;
		}
	}
	if (!format) {
		if (nd_src_root.Find(N_FUNCTION, 'main')) {
			format = 'exe';
		}
	}
	if (format) {
		this.flags |= CMAKE_CHANGED;
		this.CMakeCreateTarget(name, {
			format: format,
			files: our_files_filtered
		})
	}
}

Node.CMakeInsertAMACommand = function(options) {
	let dir_cmake = path.dirname(path.resolve(this.data));
	let fn_src = TryRelative(dir_cmake, options.source_file);
	let fn_out = TryRelative(dir_cmake, options.output);
	for (let nd_call of this.FindAll(N_CALL, 'add_custom_command')) {
		let args = nd_call.TokenizeCMakeArgs();
		for (let i = 0; i + 1 < args.length; i++) {
			if (args[i].GetName() == 'OUTPUT' && args[i + 1].GetName() == fn_out) {
				//we already have a command
				return nd_call;
			}
		}
	}
	this.flags |= CMAKE_CHANGED;
	let cmd = [
		'\nadd_custom_command(\n',
		'  OUTPUT ',JSON.stringify(fn_out),'\n',
		'  COMMAND ',options.command.replace(/\$\{SOURCE_FILE\}/g, JSON.stringify(fn_src)),'\n',
		'  MAIN_DEPENDENCY ',options.main_dependency.map(fn=>JSON.stringify(TryRelative(dir_cmake, fn))).join(' '),'\n',
		'  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}\n',
		')\n'
	].join('');
	return this.Insert(POS_BACK, ParseCode(cmd, cmake_options).Find(N_CALL, null))
}

Node.UpdateCMakeLists = function(fn_cmake) {
	if (!fn_cmake) {
		fn_cmake = __global.__cmakelist;
	}
	if (!fn_cmake) {
		let dir = path.dirname(fn_cmake);
		let dir_git = undefined;
		for (; ;) {
			let fn_test = path.join(dir, 'CMakeLists.txt');
			if (fs.existsSync(path.join(dir, '.git'))) {
				dir_git = dir;
			}
			if (fs.existsSync(fn_test)) {
				fn_cmake = fn_test;
				break;
			}
			if (dir_git) {break;}
		}
		if (dir_git && !fn_cmake) {
			fn_cmake=path.join(dir_git, 'CMakeLists.txt')
		}
	}
	let nd_cmake = cmake.LoadCMakeFile(fn_cmake, ()=>[
		'cmake_minimum_required (VERSION 3.0)\n',
		'project(',path.basename(path.dirname(path.resolve(fn_cmake))).replace(/[^0-9a-zA-Z]+/g, '_'),')\n',
		'if(NOT AMA)\n',
		'  find_program(AMA ama)\n',
		'endif()\n',
		'if(NOT AMA_MODULES)\n',
		'  if(DEFINED ENV{AMA_MODULES})\n',
		'    set(AMA_MODULES "$ENV{AMA_MODULES}")\n',
		'  else()\n',
		'    set(AMA_MODULES "$ENV{HOME}/.ama_modules")\n',
		'  endif()\n',
		'endif()\n',
		'if(NOT AMA_MODULES_GLOBAL)\n',
		'  if(DEFINED ENV{AMA_MODULES_GLOBAL})\n',
		'    set(AMA_MODULES_GLOBAL "$ENV{AMA_MODULES_GLOBAL}")\n',
		'  else()\n',
		'    set(AMA_MODULES_GLOBAL "/usr/share/ama_modules")\n',
		'  endif()\n',
		'endif()\n',
	].join(''))   
	//insert files
	nd_cmake.CMakeUpdateTargetWithSourceRoot(this)
	//insert the ama command for self
	//about with-ama dependencies: ama once manually to pull, we don't want to crawl dependent files for @ama
	//by default, remove the last '.ama' from the current name
	//if the file name doesn't contain '.ama', just don't generate the ama command
	let fn_output = this.data;
	let pama = fn_output.lastIndexOf('.ama');
	if (pama >= 0) {
		fn_output = fn_output.substr(0, pama) + fn_output.substr(pama + 4);
		nd_cmake.CMakeInsertAMACommand({
			output: fn_output,
			source_file: this.data,
			command: '${AMA} -s "__global.__cmakelist=\'${CMAKE_CURRENT_LIST_FILE}\'" ${SOURCE_FILE}',
			main_dependency: [path.resolve(this.data)].concat(depends.ListLoadedFiles())
		});
		if (nd_cmake.flags & CMAKE_CHANGED) {
			nd_cmake.Save(cmake_options);
		}
		///////////////////////
		this.Save({name:fn_output});
	}
}