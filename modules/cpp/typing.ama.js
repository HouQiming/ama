'use strict'
//@ama ParseCurrentFile().then(require("jcs").TranslateJCS)
const depends = require('depends');
const classes = require('class');
const assert = require('assert');
let typing = module.exports;

typing.type_cache = new Map();

typing.DropCache = function() {
	typing.type_cache = new Map();
};

function BasicType(name) {
	let type = typing.type_cache.get(name);
	if (!type) {
		type = nRef(name);
		typing.type_cache.set(name, type);
	}
	return type;
}

typing.options = {
	string_type: nPostfix(nRef('char'), '*'),
	regexp_type: nRef('RegExp'),
	bool_type: nRef('int'),
	node_type: nPostfix(.(ama::Node), '*'),
	null_type: nPostfix(nRef('void'), '*'),
	non_type_function_keywords: new Set([
		'private', 'public', 'protected',
		'final', 'const', 'constexpr',
		'static', 'virtual', 'override',
		'__attribute__', '__declspec', '__cdecl', '__stdcall', '__fastcall',
		'extern', 'function', 'inline', '__inline', 'def', 'fn'
	]),
	nulls: new Set(['NULL', 'nullptr']),
	ComputeNumberType: function(nd) {
		let type = undefined;
		let is_int = 1;
		let n_L = 0;
		let n_u = 0;
		//the code was C++
		for (let i = 0; i < nd.data.length; i++) {
			let ch = nd.data[i];
			if ( ch == 'p' || ch == 'P' || ch == '.' ) {
				is_int = 0;
			} else if( ch == 'e' || ch == 'E' ) {
				if ( !nd.data.startsWith('0x') ) {
					is_int = 0;
				}
			}
			if ( ch == 'L' ) {
				n_L += 1;
			}
			if ( ch == 'u' ) {
				n_u += 1;
			}
		}
		if ( is_int ) {
			if ( n_L >= 2 ) {
				type = n_u ? BasicType('uint64_t') : BasicType('int64_t');
			} else if( n_L == 1 ) {
				type = n_u ? BasicType('uintptr_t') : BasicType('intptr_t');
			} else {
				type = n_u ? BasicType('unsigned') : BasicType('int');
			}
		} else if( nd.data.endsWith("f") ) {
			type = BasicType('float');
		} else {
			type = BasicType('double');
		}
		return type;
	}
};
typing.rules = [];

let g_int_types = ['bool', 'char', 'int8_t', 'uint8_t', 'short', 'int16_t', 'uint16_t', 'int', 'long', 'unsigned', 'int32_t', 'uint32_t', 'int64_t', 'uint64_t'];
let g_float_types = ['__half', 'half', 'float', 'double'];

function ComputeOperandTypePriority(type) {
	if (!type) {return 0;}
	if (type.node_class != N_REF) {return 255;}
	for (let i = 0; i < g_int_types.length; i++) {
		if (g_int_types[i] == type.data) {
			return 64 + i;
		}
	}
	for (let i = 0; i < g_float_types.length; i++) {
		if (g_float_types[i] == type.data) {
			return 128 + i;
		}
	}
	return 192;
}

typing.GetDefs = function(nd_scope) {
	let defs = typing.type_cache.get(nd_scope);
	if (!defs) {
		defs = new Map();
		//console.log('---')
		for (let ndi = nd_scope; ndi; ndi = ndi.PreorderNext(nd_scope)) {
			if (ndi.node_class == N_SCOPE && ndi != nd_scope || ndi.node_class == N_PARAMETER_LIST) {
				ndi = ndi.PreorderLastInside();
				continue;
			}
			if (ndi.node_class == N_REF && (ndi.flags & REF_DECLARED)) {
				//console.log(ndi.data)
				defs.set(ndi.data, ndi);
			}
		}
		if (nd_scope.p && nd_scope.p.node_class == N_FUNCTION) {
			let nd_paramlist = nd_scope.p.c.s;
			for (let ndi = nd_paramlist; ndi; ndi = ndi.PreorderNext(nd_paramlist)) {
				if (ndi.node_class == N_REF && (ndi.flags & REF_DECLARED)) {
					//console.log(ndi.data)
					defs.set(ndi.data, ndi);
				}
			}
		}
		typing.type_cache.set(nd_scope, defs);
	}
	return defs;
};

//only the type system cares about declarations, put it here
//we are past auto_decl, we can focus on top-level names for N_REF
typing.LookupSymbol = function(nd_ref, want_all) {
	//cache defs in typing.type_cache: type cache gets invalidated by certain code changes anyway
	if (nd_ref.flags & REF_DECLARED) {
		if (want_all) {
			return [nd_ref];
		} else {
			return nd_ref;
		}
	}
	let ret = [];
	//look up local scopes
	for (let ndi = nd_ref; ndi; ndi = ndi.p) {
		if (ndi.node_class == N_SCOPE || ndi.node_class == N_FILE) {
			let nd_def = typing.GetDefs(ndi).get(nd_ref.data);
			if (nd_def) {
				if (want_all) {
					ret.push(nd_def);
				} else {
					return nd_def;
				}
			}
		}
	}
	//not found: we have to load dependencies
	for (let nd_dep of depends.ListAllDependency(nd_ref.Root(), true).map(fn=>depends.LoadFile(fn))) {
		let nd_def = typing.GetDefs(nd_dep).get(nd_ref.data);
		if (nd_def) {
			if (want_all) {
				ret.push(nd_def);
			} else {
				return nd_def;
			}
		}
	}
	return want_all ? ret : undefined;
};

typing.ComputeDeclaredType = function(nd_def) {
	//COULDDO: non-C++ forms of declaration
	//COULDDO: handle destructuring and other weird forms
	let modifiers = [];
	let type = undefined;
	let nd_owner = nd_def.Owner();
	if (nd_owner && (nd_owner.node_class == N_CLASS && nd_owner.c.s.isAncestorOf(nd_def) ||
	nd_owner.node_class == N_FUNCTION && nd_owner.c.isAncestorOf(nd_def))) {
		//class / function
		return nd_owner;
	}
	for (let ndi = nd_def.p; ndi; ndi = ndi.p) {
		if (ndi.node_class == N_ITEM || ndi.node_class == N_PREFIX || ndi.node_class == N_POSTFIX) {
			modifiers.push(ndi);
		} else if(ndi.node_class == N_RAW) {
			//just pick the last expr
			for (let ndj = ndi.c; ndj; ndj = ndj.s) {
				if (ndj.isAncestorOf(nd_def)) {break;}
				type = typing.ComputeType(ndj);
			}
			break;
		} else if(ndi.node_class == N_CLASS || ndi.node_class == N_FUNCTION) {
			type = ndi;
			break;
		} else if(ndi.node_class == N_SEMICOLON || ndi.node_class == N_SCOPE) {
			break;
		}
	}
	if (type && modifiers.length) {
		//C-style modifiers
		//COULDDO: we could have gotten the priorities wrong
		type = type.Clone();
		while (modifiers.length) {
			let nd_modifier = modifiers.pop();
			if (nd_modifier.node_class == N_ITEM) {
				let type_new = nd_modifier.Clone();
				type_new.c.ReplaceWith(type);
				type = type_new;
			} else if(nd_modifier.node_class == N_PREFIX) {
				type = nPrefix(nd_modifier.data, type);
			} else if(nd_modifier.node_class == N_POSTFIX) {
				type = nPostfix(type, nd_modifier.data);
			}
		}
	}
	return type;
};

function isNamespace(nd_class) {
	for (let ndi = nd_class; ndi; ndi = ndi.p) {
		if (ndi.node_class == N_FUNCTION) {return 0;}
		if (ndi.node_class == N_CLASS && ndi.data != 'namespace') {return 0;}
	}
	return 1;
}

typing.LookupNamespacesByNames = function(nd_root, names) {
	let all_scopes = [];
	for (let nd_dep of [nd_root].concat(depends.ListAllDependency(nd_root, true).map(fn=>depends.LoadFile(fn)))) {
		let scopes = [nd_dep];
		for (let i = names.length - 1; i >= 0; i--) {
			let new_scopes = [];
			for (let nd_scope of scopes) {
				for (let nd_class of nd_scope.FindAllWithin(BOUNDARY_FUNCTION | BOUNDARY_CLASS, N_CLASS, names[i])) {
					if (nd_class.data == 'namespace' && nd_class.LastChild().node_class == N_SCOPE) {
						new_scopes.push(nd_class.LastChild());
					}
				}
			}
			scopes = new_scopes;
			if (!scopes.length) {break;}
		}
		for (let nd_scope of scopes) {
			all_scopes.push(nd_scope);
		}
	}
	return all_scopes;
};

///returns a binding of all template parameters
typing.MatchTemplateType = function(type_in, type_template, names) {
	let is_param = new Set(names);
	let nd_pattern = type_template.Clone();
	for (let ndi = nd_pattern; ndi; ndi = ndi.PreorderNext(nd_pattern)) {
		if (ndi.node_class == N_REF && is_param.has(ndi.data)) {
			let was_pattern = (nd_pattern == ndi);
			ndi = ndi.ReplaceWith(Node.MatchAny(ndi.data));
			if (was_pattern) {
				nd_pattern = ndi;
			}
			ndi = ndi.PreorderLastInside();
		}
	}
	return type_in.Match(nd_pattern);
};

typing.ComputeReturnType = function(type_func) {
	let type = undefined;
	let nd_after = type_func.c.s.s;
	let nd_before = type_func.c;
	if (nd_after.node_class == N_LABELED && nd_after.c.node_class == N_AIR) {
		//air :, a natural extension of the JS syntax, function():foo{}
		type = typing.ComputeType(nd_after.c.s);
	} else if(nd_after.node_class == N_RAW && nd_after.c && nd_after.c.isSymbol('->') && nd_after.c.s) {
		//air ->, C++ lambda return type
		type = typing.ComputeType(nd_after.c.s);
	} else if(nd_before.node_class == N_RAW) {
		//we could have `static __attribute__(foo) void __attribute__(bar) __cdecl f`
		//go back to front and filter out the calling conventions
		for (let ndi = nd_before.LastChild(); ndi; ndi = ndi.Prev()) {
			if (ndi.node_class == N_REF && typing.options.non_type_function_keywords.has(ndi.data)) {
				if (type) {break;} else {continue;}
			}
			if (ndi.node_class == N_CALL && ndi.c.node_class == N_REF && ndi.node_class == N_CALL && ndi.c.node_class == N_REF && typing.options.non_type_function_keywords.has(ndi.c.data)) {
				if (type) {break;} else {continue;}
			}
			type = ndi;
			if (type.node_class != N_REF) {
				break;
			}
		}
	}
	return type;
};

typing.ComputeType = function(nd_expr) {
	let type = typing.type_cache.get(nd_expr);
	if (type) {return type;}
	for (let i = typing.rules.length - 1; i >= 0; i--) {
		type = typing.rules[i](nd_expr);
		if (type) {
			typing.type_cache.set(nd_expr, type);
			return type;
		}
	}
	switch (nd_expr.node_class) {
	case N_RAW:
	case N_SYMBOL:
	case N_AIR: {
		//we don't really understand these, so we can't compute a type
		//and it's not worth caching
		return undefined;
	}
	case N_FILE:
	case N_SCOPE:
	case N_SCOPED_STATEMENT:
	case N_EXTENSION_CLAUSE:
	case N_KEYWORD_STATEMENT:
	case N_PARAMETER_LIST:
	case N_CALL_CUDA_KERNEL:
	case N_LABELED:
	case N_SEMICOLON:
	case N_DEPENDENCY: {
		//don't have a type in C++, but could generalize a bit to return something
		//COULDDO: generalize
		return undefined;
	}
	case N_FUNCTION:
	case N_CLASS: {
		//they are self-representing, don't cache
		return nd_expr;
	}
	case N_PAREN: {
		type = typing.ComputeType(nd_expr.c);
		break;
	}
	case N_STRING: {
		return typing.options.string_type;
	}
	case N_JS_REGEXP: {
		return typing.options.regexp_type;
	}
	case N_NODEOF: {
		return typing.options.node_type;
	}
	case N_NUMBER: {
		type = typing.options.ComputeNumberType(nd_expr);
		break;
	}
	case N_POSTFIX:
	case N_PREFIX: {
		//COULDDO: try to find overloaded operators
		//here we cheat and return the operand
		type = typing.ComputeType(nd_expr.c);
		break;
	}
	case N_CONDITIONAL:
	case N_BINOP: {
		if (nd_expr.data == '||' || nd_expr.data == '&&') {
			if (typing.options.bool_type) {
				return typing.options.bool_type;
			}
		}
		//COULDDO: try to find overloaded operators
		//here we cheat and return one of the operands
		let nd_a = nd_expr.node_class == N_CONDITIONAL ? nd_expr.c.s : nd_expr.c;
		let type_a = typing.ComputeType(nd_a);
		let type_b = typing.ComputeType(nd_a.s);
		type = ComputeOperandTypePriority(type_b) > ComputeOperandTypePriority(type_a) ? type_b : type_a;
		break;
	}
	case N_ASSIGNMENT: {
		type = typing.ComputeType(nd_expr.c);
		break;
	}
	case N_REF: {
		//find the declaration
		let nd_def = typing.LookupSymbol(nd_expr, false);
		if (!nd_def) {
			//assume self-representing
			if (typing.options.nulls.has(nd_expr.data)) {
				type = typing.null_type;
			} else {
				type = nd_expr;
			}
		} else {
			type = typing.ComputeDeclaredType(nd_def);
		}
		break;
	}
	case N_DOT: {
		//try to look up the "primary" type first
		//COULDDO: substitute template parameters
		let type_obj = typing.ComputeType(nd_expr.c);
		while (type_obj && type_obj.node_class == N_CALL_TEMPLATE) {
			type_obj = typing.ComputeType(type_obj.c);
		}
		if (type_obj.node_class == N_CLASS) {
			let nd_def = typing.GetDefs(type_obj.LastChild()).get(nd_expr.data);
			if (nd_def) {
				type = typing.ComputeDeclaredType(nd_def);
				if (type) {break;}
			}
		}
		//it got ugly: we need to search ALL same-named namespaces
		//don't merge namespaces internally: each file "sees" a different version of the same namespace
		if (isNamespace(type_obj)) {
			let names = [];
			for (let ndi = type_obj; ndi; ndi = ndi.p) {
				if (ndi.node_class == N_CLASS) {names.push(ndi.GetName());}
			}
			for (let nd_scope of typing.LookupNamespacesByNames(nd_expr.Root(), names)) {
				let nd_def = typing.GetDefs(nd_scope.LastChild()).get(nd_expr.data);
				if (nd_def) {
					type = typing.ComputeDeclaredType(nd_def);
					if (type) {break;}
				}
			}
		}
		//assume self-representative when we fail to find the name
		if (!type) {type = nd_expr;}
		break;
	}
	case N_CALL: {
		//COULDDO: substitute template parameters
		//COULDDO: try to resolve overloading
		let type_func = typing.ComputeType(nd_expr.c);
		while (type_func && type_func.node_class == N_CALL_TEMPLATE) {
			type_func = typing.ComputeType(type_func.c);
		}
		if (type_func && type_func.node_class == N_FUNCTION) {
			type = typing.ComputeReturnType(type_func);
		}
		break;
	}
	case N_CALL_TEMPLATE: {
		let type_template = typing.ComputeType(nd_expr.c);
		if (type_template && type_template.node_class == N_CLASS) {
			//we are self-representing
			type = nd_expr;
		} else {
			//we are just a function, let the outer call worry about the return type
			type = type_template;
		}
		break;
	}
	case N_ITEM: {
		//we should do this before sane_types: we want the untranslated *sane* types
		let type_obj = typing.ComputeType(nd_expr.c);
		//recognize array and pointer
		if (type_obj && type_obj.node_class == N_ITEM) {
			type = type_obj.c;
			break;
		}
		if (type_obj && type_obj.node_class == N_PREFIX && type_obj.data == '*') {
			type = type_obj.c;
			break;
		}
		//recognize other hard-coded templates in hooks: don't put them here
		break;  
	}
	}
	typing.type_cache.set(nd_expr, type);
	//console.log(nd_expr.toSource(), type);
	return type;
};

typing.AccessTypeAt = function(type, nd_site) {
	if (type.node_class == N_CLASS) {
		let names = [];
		for (let ndi = type; ndi; ndi = ndi.p) {
			if (ndi.node_class == N_CLASS) {
				names.push(ndi.GetName());
			}
		}
		let ret = nRef(names.pop());
		while (names.length) {
			ret = ret.dot(names.pop());
		}
		return ret;
	} else if(type.node_class == N_CALL_TEMPLATE) {
		let ret = type.Clone();
		ret.c.ReplaceWith(typing.AccessTypeAt(ret.c, nd_site))
		return ret;
	}
	//COULDDO: N_REF case
	return type.Clone();
};

typing.DeduceAutoTypedDef = function(nd_def) {
	assert(nd_def.flags & REF_DECLARED);
	let nd_scope = nd_def.Owning(N_SCOPE) || nd_def.Root();
	let nd_owner = nd_def.Owner();
	//don't deduce fields
	//find all its assignments
	let type_template = typing.ComputeDeclaredType(nd_def);
	let nd_auto = type_template && type_template.Find(N_REF, 'auto');
	if (!nd_auto) {return;}
	let has_null = 0;
	let cands = [];
	for (let nd_ref of nd_scope.FindAll(N_REF, nd_def.data)) {
		if (typing.LookupSymbol(nd_ref) != nd_def) {continue;}
		let nd_stmt = nd_ref.ParentStatement();
		while (nd_stmt.node_class == N_SEMICOLON) {
			nd_stmt = nd_stmt.c;
		}
		if (nd_stmt.node_class == N_ASSIGNMENT && nd_stmt.c.isAncestorOf(nd_ref)) {
			let nd_value = nd_stmt.c.s;
			if (nd_value.node_class == N_REF && typing.options.nulls.has(nd_value.data)) {
				has_null = 1;
			} else {
				let type_value = typing.ComputeType(nd_value);
				if (type_value) {
					let match = typing.MatchTemplateType(type_value, type_template, ['auto']);
					if (match) {
						cands.push(match.auto);
					}
				}
			}
		}
	}
	if (has_null && !cands.length) {
		let match = typing.MatchTemplateType(typing.options.null_type, type_template, ['auto']);
		if (match) {
			cands.push(match.auto);
		}
	}
	if (cands.length > 0) {
		//try promoting
		let type = cands[0];
		let best = ComputeOperandTypePriority(type);
		for (let i = 1; i < cands.length; i++) {
			let score = ComputeOperandTypePriority(cands[i]);
			if (best < score) {
				best = score;
				type = cands[i];
			}
		}
		nd_auto.ReplaceWith(typing.AccessTypeAt(type, nd_auto));
	}
};

typing.DeduceAuto = function(nd_root) {
	for (let nd_def of nd_root.FindAll(N_REF, null)) {
		if (!(nd_def.flags & REF_DECLARED) || nd_def.data == 'auto') {continue;}
		if (!(nd_def.p && nd_def.p.node_class == N_RAW && nd_def.p.Find(N_REF, 'auto'))) {continue;}
		typing.DeduceAutoTypedDef(nd_def);
	}
	typing.DropCache();
};