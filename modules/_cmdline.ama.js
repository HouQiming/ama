'use strict'
let _cmdline = module.exports;

_cmdline.help = function(argv) {
	console.log("  ama [-s script] [files]  = process [files] with [script]");
	for (let key in _cmdline) {
		if (_cmdline[key].usage) {
			console.log([
				'  ama --', key, _cmdline[key].usage,
			].join(''));
		}
	}
};
_cmdline.help.usage = '               = print this help';

/////////////////////////////////
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
	let stat = {};
	try {
		stat = fs.statSync(dir0);
	} catch (e) {};
	return stat.is_file ? path.dirname(dir0) : dir0;
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
	let stat = {};
	try {
		stat = fs.statSync(dir0);
	} catch (e) {};
	let is_file = !!stat.is_file;
	let dir = ComputeProjectDir(fn_init);
	let fn_sync_js = path.resolve(dir, 'script/sync.js');
	if (fs.existsSync(fn_sync_js)) {
		console.log('already initialized');
	} else {
		//generate list of features on the fly
		let feature_code = [];
		for (let fn of fsext.FindAllFiles(__dirname)) {
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
				let pnewline = s.indexOf('\n', p_filter);
				let brief = s.substr(p_filter + 7, pnewline - p_filter - 7).trim();
				feature_code.push(
					'////', brief, '\n',
					'//require(', JSON.stringify(fn_require), ')', name && name != 'Translate' ? '.' + name : '', ',\n'
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
		fs.writeFileSync(fn_sync_js, @(require('bisync')({
			dir_src: path.resolve(__dirname, '..'),
			processed_extensions: @(ParseCode(JSON.stringify(processed_extensions))),
			features: [
				@(nRaw(nAir().setCommentsBefore(feature_code.join(''))))
			]
		});).toSource() + '\n');
		console.log('created', fn_sync_js);
	}
};
_cmdline.init.usage = ' [path]        = set up a project or a file, defaults to ./';

_cmdline.build = function(argv) {
	//TODO: run sync.js then run cmake
};
_cmdline.build.usage = ' [path]       = build a project or a file, defaults to ./';
