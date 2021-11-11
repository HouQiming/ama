//the bidirectional synchronization workflow
'use strict';
const pipe = require('pipe');
const path = require('path');
const fs = require('fs');

/**
@param options Options and default values:
{
	dir_src: path.resolve('./src'),
	middle_extension: '.ama',
	processed_extensions:['.cpp','.hpp']
	script_forward: fs.readFileSync(path.join(__dirname,'cpp/from_ama.js')).toString(),
	script_backward: fs.readFileSync(path.join(__dirname,'cpp/to_ama.js')).toString(),
}
*/
module.exports = function Bisync(options) {
	if (!options) {options = {};}
	function GetFileTime(fn) {
		if (!fs.existsSync(fn)) {return 0;}
		let stat = fs.statSync(fn);
		return stat.mtimeMs;
	}
	let middle_extension = options.middle_extension || '.ama';
	let dir_src = options.dir_src || path.resolve('./src');
	process.chdir(dir_src);
	let script_ama2cpp = options.script_forward || fs.readFileSync(path.join(__dirname, 'cpp/from_ama.js')).toString();
	let script_cpp2ama = options.script_backward || fs.readFileSync(path.join(__dirname, 'cpp/to_ama.js')).toString();
	let all_cpp_files = new Set();
	let mext_dot = middle_extension + '.';
	let exts = options.processed_extensions || ['.cpp', '.hpp', '.cu'];
	for (let ext of exts) {
		for (let fn_rel_cpp of pipe.runPiped(process.platform == 'win32' ? 'dir /s /b *' + ext : "find -iname '*" + ext + "'").stdout.split('\n')) {
			if (!fn_rel_cpp) {continue;}
			all_cpp_files.add(path.resolve(dir_src, fn_rel_cpp.replace(mext_dot, '.')));
		}
	}
	let rebuild_ama = 'rebuild_' + middle_extension.replace(/\./g, '');
	let rebuild_cpp = 'rebuild_' + exts[0].replace(/\./g, '');
	for (let fn_cpp of all_cpp_files) {
		//Node.gc();
		let parts = path.parse(fn_cpp);
		let fn_ama_cpp = path.join(parts.dir, parts.name + middle_extension + parts.ext);
		let t_cpp = GetFileTime(fn_cpp);
		let t_ama = GetFileTime(fn_ama_cpp);
		if (t_ama < t_cpp || process.aba || process[rebuild_ama]) {
			//cpp to ama
			if (process.dry_run || ProcessAmaFile(fn_cpp, script_cpp2ama) == 1) {
				if (!process.dry_run) {pipe.run(['touch -r ', JSON.stringify(fn_cpp), ' ', JSON.stringify(fn_ama_cpp)].join(''));}
				console.log(process.dry_run ? 'will update' : 'updated', fn_ama_cpp);
			}
		}
		if (t_cpp < t_ama || process.aba || process[rebuild_cpp]) {
			//ama to cpp
			if (process.dry_run || ProcessAmaFile(fn_ama_cpp, script_ama2cpp) == 1) {
				if (!process.dry_run) {pipe.run(['touch -r ', JSON.stringify(fn_ama_cpp), ' ', JSON.stringify(fn_cpp)].join(''));}
				console.log(process.dry_run ? 'will update' : 'updated', fn_cpp);
			}
		}
	}
}
