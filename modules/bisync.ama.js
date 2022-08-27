//Bidirectional synchronization system. Usage with default option values:
//```Javascript
//require('bisync')({
//    dir_src: path.resolve(__dirname, '../src'),
//    middle_extension: '.ama',
//    processed_extensions: ['.cpp','.hpp','.cu'],
//    filters: [],
//})
//```
//The module will search for all files with `processed_extensions` in `dir_src`.
//It will apply filters specified in `filters` to files with `middle_extension` (e.g. `foo.ama.cpp`) to generate the corresponding non-ama file (e.g. `foo.cpp`).
//When available, it will also apply the inverse version of the filters to generate ama files from non-ama files.
//Between each pair of ama and non-ama files, `bisync` will always synchronize the the newer file's content to its older counterpart.
//
//See [the filters section](#-filters) for possible filters.
'use strict';
const pipe = require('pipe');
const path = require('path');
const fs = require('fs');
const fsext = require('fsext');

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
	/////////
	let script_ama2cpp = '';
	let script_cpp2ama = '';
	let p_backup = __global.default_pipeline;
	let p_forward = __global.default_pipeline.map(a => a);
	let p_inverse = __global.default_pipeline.map(a => a);
	if (options.filters) {
		let p_inverse_rev = [];
		for (let item of options.filters) {
			if (typeof(item) == 'string') {
				item = __global.__GetFilterByName(item);
			}
			p_forward.push(item);
			if (item.setup) {
				p_forward.unshift(item.setup);
			}
			if (item.inverse) {
				p_inverse_rev.push(item.inverse);
				if (item.setup) {
					p_inverse.unshift(item.setup);
				}
			}
		}
		for (let item of p_inverse_rev.reverse()) {
			p_inverse.push(item);
		}
		//we cannot edit the default pipeline: it's used in depends
		script_ama2cpp = '__pipeline=GetPipelineFromFilename(__filename,__global.forward_pipeline);';
		script_cpp2ama = '__pipeline=GetPipelineFromFilename(__filename,__global.inverse_pipeline);/*ignore per-file scripts*/return;';
		__global.inverse_pipeline = p_inverse;
		__global.forward_pipeline = p_forward;
	} else {
		script_ama2cpp = options.script_forward || fs.readFileSync(path.join(__dirname, 'cpp/from_ama.js')).toString();
		script_cpp2ama = options.script_backward || fs.readFileSync(path.join(__dirname, 'cpp/to_ama.js')).toString();
	}
	let all_cpp_files = new Set();
	let mext_dot = middle_extension + '.';
	let exts = options.processed_extensions || ['.cpp', '.hpp', '.cu'];
	let all_files = fsext.FindAllFiles(dir_src);
	for (let ext of exts) {
		//for (let fn_rel_cpp of pipe.runPiped(process.platform == 'win32' ? 'dir /s /b *' + ext : "find -iname '*" + ext + "'").stdout.split('\n')) 
		for (let fn_rel_cpp of all_files) {
			//if (!fn_rel_cpp) {continue;}
			if (path.extname(fn_rel_cpp) != ext) {continue;}
			//COULDDO: ignore patterns
			if (fn_rel_cpp.indexOf('/build/') >= 0 || fn_rel_cpp.indexOf('\\build\\') >= 0) {continue;}
			let fn_final = path.resolve(dir_src, fn_rel_cpp.replace(mext_dot, '.'));
			//full name already contains an extra dot: we can't handle such files
			if (path.basename(fn_final).match(/\..*/).toString() != path.extname(fn_final)) {continue;}
			all_cpp_files.add(fn_final);
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
		p_inverse.push({change_ext: middle_extension + parts.ext}, 'Save');
		p_forward.push({change_ext: parts.ext}, 'Save');
		if (t_ama < t_cpp || process.aba || process[rebuild_ama]) {
			//cpp to ama
			if (process.dry_run || ProcessAmaFile(fn_cpp, script_cpp2ama) == 1) {
				if (!process.dry_run) {fsext.SyncTimestamp(fn_cpp, fn_ama_cpp);}
				console.log(process.dry_run ? 'will update' : 'updated', fn_ama_cpp);
			}
		}
		if (t_cpp < t_ama || process.aba || process[rebuild_cpp]) {
			//ama to cpp
			if (process.dry_run || ProcessAmaFile(fn_ama_cpp, script_ama2cpp) == 1) {
				if (!process.dry_run) {fsext.SyncTimestamp(fn_ama_cpp, fn_cpp);}
				console.log(process.dry_run ? 'will update' : 'updated', fn_cpp);
			}
		}
		p_forward.pop();p_forward.pop();
		p_inverse.pop();p_inverse.pop();
	}
};
