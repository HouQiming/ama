'use strict';
//command line --foo command resolver
let _cmdline = module.exports;

_cmdline.help = function(argv) {
	console.log([
		"  ama [options] files          = process files with options",
		"      -f filter                = add a filter",
		"      -o path                  = write output to path",
		"      -s script                = run the script",
	].join('\n'));
	for (let key in _cmdline) {
		if (_cmdline[key].usage) {
			console.log([
				'  ama --', key, _cmdline[key].usage,
			].join(''));
		}
	}
	console.log('\nList of filters:');
	const path = require('path');
	const fs = require('fs');
	const fsext = require('fsext');
	const depends = require('depends');
	for (let fn of fsext.FindAllFiles(__dirname).sort()) {
		if (!fn.endsWith('.js')) {continue;}
		let nd_root = depends.LoadFile(fn);
		if (!nd_root) {continue;}
		for (let nd_func of nd_root.FindAll(N_FUNCTION)) {
			let s = nd_func.ParentStatement().comments_before;
			let p_filter = s.indexOf('#filter');
			if (p_filter < 0) {continue;}
			let fn_require = path.relative(__dirname, fn).replace(/[.].*/, '');
			let name = nd_func.data;
			if (!name && nd_func.p.node_class == N_ASSIGNMENT) {
				name = nd_func.p.c.GetName();
			}
			if (name == 'Translate') {name = '';}
			if (name == 'exports') {name = '';}
			if (name) {
				fn_require = fn_require + '.' + name;
			}
			let pnewline = s.indexOf('\n', p_filter);
			let brief = s.substr(p_filter + 7, pnewline - p_filter - 7).trim();
			console.log(['  ', fn_require, ' '.repeat(Math.max(37 - fn_require.length, 1)), brief].join(''));
		}
	}
};
_cmdline.help.usage = '                   = print this help';

/////////////////////////////////
function isFile(path) {
	const fs = require('fs');
	let stat = {};
	try {
		stat = fs.statSync(path);
	} catch (e) {};
	return !!stat.is_file;
}

function ComputeProjectDir(dir0) {
	const path = require('path');
	const fs = require('fs');
	const fsext = require('fsext');
	let dir = dir0;
	for (; ;) {
		if (fs.existsSync(path.join(dir, '.git'))) {
			return dir;
		}
		if (fs.existsSync(path.join(dir, '.svn'))) {
			return dir;
		}
		let dir_next = path.dirname(dir);
		if (dir_next == dir) {break;}
		dir = dir_next;
	}
	return isFile(dir0) ? path.dirname(dir0) : dir0;
}

let g_exts = {
	'.cpp': ['.cpp', '.hpp', '.cu'],
	'.hpp': ['.cpp', '.hpp', '.cu'],
	'.cu': ['.cpp', '.hpp', '.cu'],
	'.cc': ['.cc', '.hh'],
	'.hh': ['.cc', '.hh'],
	'.cxx': ['.cxx', '.hxx'],
	'.hxx': ['.cxx', '.hxx'],
	'.c': ['.c', '.h'],
	'.h': ['.c', '.h'],
	'.m': ['.m', '.c', '.h'],
	'.mm': ['.mm', '.cpp', '.hpp', '.cu'],
};
_cmdline.init = function(argv) {
	const path = require('path');
	const depends = require('depends');
	const fsext = require('fsext');
	const fs = require('fs');
	const pipe = require('pipe');
	let fn_init = argv[2] || '.';
	let is_file = isFile(fn_init);
	let dir = ComputeProjectDir(fn_init);
	let fn_sync_js = path.resolve(dir, 'script/sync.js');
	if (fs.existsSync(fn_sync_js)) {
		console.log('already initialized');
	} else {
		//generate list of features on the fly
		let feature_code = [];
		for (let fn of fsext.FindAllFiles(__dirname).sort()) {
			if (!fn.endsWith('.js')) {continue;}
			let nd_root = depends.LoadFile(fn);
			if (!nd_root) {continue;}
			for (let nd_func of nd_root.FindAll(N_FUNCTION)) {
				let s = nd_func.ParentStatement().comments_before;
				let p_filter = s.indexOf('#filter');
				if (p_filter < 0) {continue;}
				let fn_require = path.relative(__dirname, fn).replace(/[.].*/, '');
				let name = nd_func.data;
				if (!name && nd_func.p.node_class == N_ASSIGNMENT) {
					name = nd_func.p.c.GetName();
				}
				if (name == 'Translate') {name = '';}
				if (name == 'exports') {name = '';}
				let pnewline = s.indexOf('\n', p_filter);
				let brief = s.substr(p_filter + 7, pnewline - p_filter - 7).trim();
				let s_prefix = '//';
				if (s.indexOf('#default on') >= 0) {
					s_prefix = '';
				}
				feature_code.push(
					s_prefix, 'require(', JSON.stringify(fn_require), ')', name ? '.' + name : '', ', //', brief, '\n'
				);
			}
		}
		let processed_extensions = ['.cpp', '.hpp', '.cu'];
		if (is_file && path.extname(fn_init)) {
			let new_exts = g_exts[path.extname(fn_init)];
			if (new_exts) {
				processed_extensions = new_exts;
			} else {
				processed_extensions = [path.extname(fn_init)];
			}
		}
		//generate sync.js
		if (feature_code.length) {feature_code.pop();}
		let dir_sync_js = path.dirname(fn_sync_js);
		pipe.run((__platform == 'win32' ? 'md ' : 'mkdir -p ') + dir_sync_js);
		fs.writeFileSync(fn_sync_js, '#!/usr/bin/env ama\n' + @(require('bisync')({
			dir_src: require('path').resolve(__dirname, '..'),
			processed_extensions: @(ParseCode(JSON.stringify(processed_extensions))),
			features: [
				@(nRaw(nAir().setCommentsBefore(feature_code.join(''))))
			]
		});).toSource() + '\n');
		console.log('created', fn_sync_js);
	}
	let ret = ProcessAmaFile(fn_sync_js, '');
	if (ret <= 0) {
		throw new Error('but failed to run it');
	}
};
_cmdline.init.usage = ' [dir]             = set up a project dir, defaults to ./';

_cmdline.build = function(argv) {
	const path = require('path');
	const depends = require('depends');
	const fsext = require('fsext');
	const fs = require('fs');
	const pipe = require('pipe');
	const cmake = require('cmake');
	if (argv[2] && argv[2].startsWith('--')) {
		argv.splice(2, 0, '.');
	}
	let fn_init = argv[2] || '.';
	let dir = ComputeProjectDir(fn_init);
	let fn_sync_js = path.resolve(dir, 'script/sync.js');
	if (fs.existsSync(fn_sync_js)) {
		let ret = ProcessAmaFile(fn_sync_js, '');
		if (ret <= 0) {
			throw new Error('failed to synchronize ama files');
		}
	} else if (isFile(fn_init)) {
		let ret = ProcessAmaFile(fn_init, '__pipeline.push(require("cmake").AutoCreate);');
		if (ret <= 0) {
			throw new Error('failed to synchronize ama files');
		}
	}
	let options = {};
	options.cmakelist_path = path.resolve(dir, 'CMakeLists.txt');
	if (!fs.existsSync(options.cmakelist_path)) {
		for (let fn of fsext.FindAllFiles(dir)) {
			if (path.basename(fn).toLowerCase() == 'cmakelists.txt') {
				options.cmakelist_path = fn;
				break;
			}
		}
	}
	if (!options.cmakelist_path) {
		throw new Error('cannot find CMakeLists.txt');
	}
	if (isFile(fn_init)) {
		options.target = path.parse(fn_init).name;
	} else {
		//run the first executable
		let nd_cmake = require('cmake').LoadCMakeFile(options.cmakelist_path);
		let nd_target = nd_cmake.Find(N_CALL, 'add_executable');
		if (nd_target) {
			let nd_name = nd_target.c.s.Find(N_REF);
			if (nd_name) {
				options.target = nd_name.data;
			}
		}
	}
	let extra_args = [];
	options.extra_args = extra_args;
	for (let i = 3; i < argv.length; i++) {
		if (argv[i] == '--clean-first') {
			options.rebuild = 1;
		} else if (argv[i] == '--type') {
			options.build = argv[i + 1];
			i += 1;
		} else if (argv[i] == '--run') {
			options.run = [];
			extra_args = options.run;
		} else {
			extra_args.push(argv[i]);
		}
	}
	cmake.Build(options);
};
_cmdline.build.usage = [
	' [path] [options] = build a project dir or file, defaults to ./, options:',
	'      --type CMAKE_BUILD_TYPE  = specify build type, e.g., Debug / RelWithDebInfo',
	'      --clean-first            = rebuild from scratch',
	'      --run [args]             = run the result after build'
].join('\n');
