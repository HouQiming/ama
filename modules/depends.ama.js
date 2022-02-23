'use strict';
const fs = require('fs');
const path = require('path');
const assert = require('assert');
let depends = module.exports;

depends.oracle = function nullOracle(name, referrer) {
	return /*full path or undefined*/;
};
depends.c_include_paths = (process.env.INCLUDE || '').split(process.platform == 'win32' ? ';' : ':').filter(s => s);

depends.Resolve = function(nd) {
	if (Node.ParseDependency) {
		assert(nd.node_class == N_DEPENDENCY);
		if ((nd.flags & DEP_TYPE_MASK) == DEP_C_INCLUDE) {
			let fn = nd.c.GetStringValue();
			if (nd.flags & DEPF_C_INCLUDE_NONSTR) {
				fn = fn.replace(new RegExp('[<>]', 'g'), '');
			}
			if (!(nd.flags & DEPF_C_INCLUDE_NONSTR)) {
				let fn_test = path.resolve(path.dirname(nd.Root().data), fn);
				if (fs.existsSync(fn_test)) {
					return fn_test;
				}
			}
			for (let dir of depends.c_include_paths) {
				let fn_test = path.resolve(dir, fn);
				if (fs.existsSync(fn_test)) {
					return fn_test;
				}
			}
			return depends.oracle(fn, nd.Root().data);
		} else if ((nd.flags & DEP_TYPE_MASK) == DEP_JS_REQUIRE) {
			//reuse the builtin searcher __ResolveJSRequire
			if (nd.c && nd.c.node_class == N_STRING) {
				return __ResolveJSRequire(__filename, nd.c.GetStringValue()) || depends.oracle(nd.c.GetStringValue(), nd.Root().data);
			}
		}
		return undefined;
	} else {
		if (nd.isStatement('#include') && nd.c && nd.c.node_class == N_STRING) {
			let fn = nd.c.GetStringValue();
			if (nd.c.flags & STRING_C_STD_INCLUDE) {
				fn = fn.replace(new RegExp('[<>]', 'g'), '');
			}
			if (!(nd.c.flags & STRING_C_STD_INCLUDE)) {
				let fn_test = path.resolve(path.dirname(nd.Root().data), fn);
				if (fs.existsSync(fn_test)) {
					return fn_test;
				}
			}
			for (let dir of depends.c_include_paths) {
				let fn_test = path.resolve(dir, fn);
				if (fs.existsSync(fn_test)) {
					return fn_test;
				}
			}
			return depends.oracle(fn, nd.Root().data);
		}
		if (nd.node_class == N_CALL && nd.c.isRef('require') && nd.c.s && !nd.c.s.s && nd.c.s.node_class == N_STRING) {
			return __ResolveJSRequire(__filename, nd.c.s.GetStringValue()) || depends.oracle(nd.c.s.GetStringValue(), nd.Root().data);
		}
		if (nd.node_class == N_IMPORT) {
			let nd_import = nd.c;
			let nd_from = nd.c.s;
			//ES6 / Python / Java: generally not file-based
			if (nd_from.node_class == N_STRING) {
				//assume ES6
				return __ResolveJSRequire(__filename, nd_from.GetStringValue()) || depends.oracle(nd.c.s.GetStringValue(), nd.Root().data);
			}
			return undefined; 
		}
	}
};

depends.cache = new Map();

depends.LoadFile = function(fn, options) {
	fn = __path_toAbsolute(fn);
	let nd_cached = depends.cache.get(fn);
	if (!nd_cached) {
		let data = null;
		try {
			data = fs.readFileSync(fn);
		} catch (err) {
			//do nothing
		};
		if (!data) {return undefined;}
		if (path.basename(fn).toLowerCase() == 'cmakelists.txt') {
			nd_cached = require('cmake').LoadCMakeFile(fn);
		} else {
			nd_cached = ParseCode(data, options || __global.GetPipelineFromFilename(fn));
		}
		nd_cached.data = fn;
		depends.cache.set(fn, nd_cached);
	}
	return nd_cached;
};

let nd_add_template = @{#pragma add(@(Node.MatchAny(N_STRING, 'kind')), @(Node.MatchAny(N_STRING, 'name')))};
depends.dependency_cache = [new Map(), new Map()];
depends.ListAllDependency = function(nd_root, include_system_headers) {
	let cache = depends.dependency_cache[0 | !!include_system_headers];
	if (cache.get(nd_root)) {
		return cache.get(nd_root);
	}
	let ret = new Set();
	let Q = [nd_root];
	for (let qi = 0; qi < Q.length; qi++) {
		let nd_root = Q[qi];
		if (Node.ParseDependency) {
			for (let ndi of nd_root.FindAll(N_DEPENDENCY, null)) {
				if (!include_system_headers && (ndi.flags & DEP_TYPE_MASK) == DEP_C_INCLUDE && ( ndi.flags & DEPF_C_INCLUDE_NONSTR )) {
					continue;
				}
				let fn_dep = depends.Resolve(ndi);
				if (fn_dep && !ret.has(fn_dep)) {
					ret.add(fn_dep);
					let nd_root_dep = depends.LoadFile(fn_dep);
					if (nd_root_dep) {
						Q.push(nd_root_dep);
					}
				}
			}
		} else {
			for (let ndi of nd_root.FindAll(N_KSTMT, '#include').concat(nd_root.FindAll(N_CALL, 'require')).concat(nd_root.FindAll(N_IMPORT))) {
				if (!include_system_headers && ndi.isStatement('#include') && ndi.c && ndi.c.node_class == N_STRING && (ndi.c.flags & STRING_C_STD_INCLUDE)) {
					continue;
				}
				let fn_dep = depends.Resolve(ndi);
				if (fn_dep && !ret.has(fn_dep)) {
					ret.add(fn_dep);
					let nd_root_dep = depends.LoadFile(fn_dep);
					if (nd_root_dep) {
						Q.push(nd_root_dep);
					}
				}
			}
		}
		for (let match of nd_root.MatchAll(nd_add_template)) {
			if (!match.kind.GetStringValue().endsWith('_files')) {continue;}
			let fn_dep = path.resolve(path.dirname(nd_root.data), match.name.GetStringValue());
			if (fs.existsSync(fn_dep) ) {
				if (!ret.has(fn_dep)) {
					ret.add(fn_dep);
					let ext = path.extname(fn_dep);
					if (ext != '.a' && ext != '.so' && ext != '.dll') {
						let nd_root_dep = depends.LoadFile(fn_dep);
						if (nd_root_dep) {
							Q.push(nd_root_dep);
						}
					}
				}
			} else {
				console.error('unable to find', fn_dep);
			}
		}
	}
	//let qret = Q.map(ndi=>__path_toAbsolute(ndi.data));
	cache.set(nd_root, Q);
	return Q;
};

depends.ListLoadedFiles = function() {
	let ret = [];
	for (let name in __require_cache) {
		ret.push(path.resolve(name));
	}
	depends.cache.forEach((nd_root, name) => {
		ret.push(path.resolve(name));
	});
	return ret;
};

depends.DropCache = function() {
	depends.cache = new Map();
	depends.dependency_cache = [new Map(), new Map()];
};

/*
#filter Dump dependency
*/
depends.Dump = function(nd_root) {
	console.log(JSON.stringify(depends.ListAllDependency(nd_root).map(nd => nd.data), null, 1));
};
