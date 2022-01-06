'use strict'
const depends = require('depends');
const classes = require('class');
const assert = require('assert');
let typing = module.exports;

typing.type_cache = new Map();
typing.def_cache = new Map();

typing.DropCache = function() {
	typing.type_cache = new Map();
	typing.def_cache = new Map();
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
	node_type: nPostfix(@(ama::Node), '*'),
	null_type: nPostfix(nRef('void'), '*'),
	non_type_function_keywords: new Set([
		'private', 'public', 'protected',
		'final', 'const', 'constexpr',
		'static', 'virtual', 'override',
		'__attribute__', '__declspec', '__cdecl', '__stdcall', '__fastcall',
		'extern', 'function', 'inline', '__inline', 'def', 'fn'
	]),
	non_type_postfixes: new Set(['++', '--']),
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
			} else if ( ch == 'e' || ch == 'E' ) {
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
			} else if ( n_L == 1 ) {
				type = n_u ? BasicType('uintptr_t') : BasicType('intptr_t');
			} else {
				type = n_u ? BasicType('unsigned') : BasicType('int');
			}
		} else if ( nd.data.endsWith("f") ) {
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
	let defs = typing.def_cache.get(nd_scope);
	if (!defs) {
		defs = new Map();
		//console.log('---')
		for (let ndi = nd_scope; ndi; ndi = ndi.PreorderNext(nd_scope)) {
			if (ndi.node_class == N_SCOPE && ndi != nd_scope || ndi.node_class == N_PARAMETER_LIST) {
				ndi = ndi.PreorderSkip();
				continue;
			}
			if (ndi.node_class == N_REF && (ndi.flags & REF_DECLARED)) {
				//console.log(ndi.data)
				if (ndi.p.node_class == N_KEYWORD_STATEMENT && defs.get(ndi.data)) {
					//let non-forward declarations override forward ones
					continue;
				}
				defs.set(ndi.data, ndi);
			}
		}
		if (nd_scope.p && (nd_scope.p.node_class == N_FUNCTION || nd_scope.p.node_class == N_PARAMETER_LIST)) {
			let nd_paramlist = nd_scope.p.c;
			if (nd_scope.p.node_class == N_FUNCTION) {
				nd_paramlist = nd_paramlist.s;
			}
			if (nd_paramlist.node_class == N_PARAMETER_LIST) {
				for (let ndi = nd_paramlist; ndi; ndi = ndi.PreorderNext(nd_paramlist)) {
					if (ndi.node_class == N_REF && (ndi.flags & REF_DECLARED)) {
						//console.log(ndi.data)
						defs.set(ndi.data, ndi);
					}
				}
			}
		}
		typing.def_cache.set(nd_scope, defs);
	}
	return defs;
};

typing.ListActiveScopes = function(nd_root) {
	return depends.ListAllDependency(nd_root, true);
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
	for (let nd_dep of typing.ListActiveScopes(nd_ref.Root())) {
		let nd_def = typing.GetDefs(nd_dep).get(nd_ref.data);
		if (nd_def) {
			if (want_all) {
				ret.push(nd_def);
			} else {
				return nd_def;
			}
		}
	}
	//COULDDO: `using` handling
	return want_all ? ret : undefined;
};

typing.ComputeDeclaredType = function(nd_def) {
	//COULDDO: non-C++ forms of declaration
	//COULDDO: handle destructuring and other weird forms
	if (nd_def.p && nd_def.p.node_class == N_KEYWORD_STATEMENT && nd_def.p.c == nd_def) {
		//forward declaration: self-representing
		return nd_def;
	}
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
		} else if (ndi.node_class == N_RAW) {
			//just pick the last expr
			for (let ndj = ndi.c; ndj; ndj = ndj.s) {
				if (ndj.isAncestorOf(nd_def)) {break;}
				type = typing.ComputeType(ndj);
			}
			break;
		} else if (ndi.node_class == N_CLASS || ndi.node_class == N_FUNCTION) {
			type = ndi;
			break;
		} else if (ndi.node_class == N_SEMICOLON || ndi.node_class == N_SCOPE) {
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
			} else if (nd_modifier.node_class == N_PREFIX) {
				type = nPrefix(nd_modifier.data, type);
			} else if (nd_modifier.node_class == N_POSTFIX) {
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

typing.LookupClassesByNames = function(nd_root, names, options) {
	let all_scopes = [];
	for (let nd_dep of options.include_dependency ? depends.ListAllDependency(nd_root, true) : [nd_root]) {
		let scopes = [nd_dep];
		for (let i = names.length - 1; i >= 0; i--) {
			let new_scopes = [];
			for (let nd_scope of scopes) {
				for (let nd_class of nd_scope.FindAllWithin(BOUNDARY_FUNCTION | BOUNDARY_CLASS, N_CLASS, names[i])) {
					if ((!options.must_be || nd_class.data == options.must_be) && nd_class.LastChild().node_class == N_SCOPE) {
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
			ndi = ndi.PreorderSkip();
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
	} else if (nd_after.node_class == N_RAW && nd_after.c && nd_after.c.isSymbol('->') && nd_after.c.s) {
		//air ->, C++ lambda return type
		type = typing.ComputeType(nd_after.c.s);
	} else if (nd_before.node_class == N_RAW) {
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

typing.LookupDottedName = function(nd_site, name, nd_class, want_all) {
	let ret = [];
	let nd_def = undefined;
	if (nd_class) {
		//base class: should use ParseClass
		//nd_def = typing.GetDefs(nd_class.LastChild()).get(name);
		let class_defs = typing.def_cache.get(nd_class);
		if (!class_defs) {
			class_defs = new Map();
			typing.def_cache.set(nd_class, class_defs);
			let desc = nd_class.ParseClass();
			for (let ppt of desc.properties) {
				let nd_def = ppt.node;
				if (!nd_def) {continue;}
				if (nd_def.node_class == N_CLASS) {
					nd_def = nd_def.c.s;
				} else if (nd_def.node_class == N_FUNCTION) {
					let nd_func = nd_def;
					nd_def = undefined;
					for (let ndi = nd_func; ndi; ndi = ndi.PreorderNext(nd_func)) {
						if (ndi.node_class == N_SCOPE || ndi.node_class == N_PARAMETER_LIST) {
							ndi = ndi.PreorderSkip();
							continue;
						}
						if (ndi.node_class == N_REF && (ndi.flags & REF_DECLARED)) {
							nd_def = ndi;
							break;
						}
					}
				}
				if (nd_def && nd_def.node_class == N_REF && (nd_def.flags & REF_DECLARED)) {
					class_defs.set(ppt.name, nd_def);
				}
			}
		}
		nd_def = class_defs.get(name);
		if (nd_def && want_all) {
			ret.push(nd_def);
		}
	}
	if ((!nd_def || want_all) && isNamespace(nd_class)) {
		//it got ugly: we need to search ALL same-named namespaces
		//don't merge namespaces internally: each file "sees" a different version of the same namespace
		let names = [];
		for (let ndi = nd_class; ndi; ndi = ndi.p) {
			if (ndi.node_class == N_CLASS) {names.push(ndi.GetName());}
		}
		for (let nd_scope of typing.LookupClassesByNames(nd_site.Root(), names, {must_be: 'namespace',include_dependency: 1})) {
			nd_def = typing.GetDefs(nd_scope).get(name);
			if (nd_def) {
				if (want_all) {
					ret.push(nd_def);
				} else {
					break;
				}
			}
		}
	}
	return want_all ? ret : nd_def;
};

typing.TryGettingClass = function(type_obj) {
	if (type_obj && type_obj.node_class == N_POSTFIX && type_obj.data == '*') {
		type_obj = typing.ComputeType(type_obj.c);
	} else if (type_obj && type_obj.node_class == N_CALL_TEMPLATE && (type_obj.GetName() == 'unique_ptr' || type_obj.GetName() == 'shared_ptr') && type_obj.c.s) {
		type_obj = typing.ComputeType(type_obj.c.s);
	}
	//we could enter an infinite loop here for:
	//`const Multilib &Multilib;`
	let dedup = new Set();
	let count = 0;
	while (type_obj && count < 1024 && !dedup.has(type_obj) && type_obj.node_class == N_POSTFIX && (type_obj.data == '&' || type_obj.data == 'const' || type_obj.data == 'volatile')) {
		dedup.add(type_obj);
		type_obj = typing.ComputeType(type_obj.c);
		count++;
	}
	count = 0;
	while (type_obj && count < 1024 && !dedup.has(type_obj) && type_obj.node_class == N_CALL_TEMPLATE) {
		dedup.add(type_obj);
		type_obj = typing.ComputeType(type_obj.c);
		count++;
	}
	return type_obj;
};

typing.PointerType = function(type) {
	if (!type) {return type;}
	return nPostfix(typing.AccessTypeAt(type, type), '*');
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
	//avoid infinite recursion
	typing.type_cache.set(nd_expr, nd_expr);
	switch (nd_expr.node_class) {
	case N_RAW: {
		//QoL: return declared type on parent N_RAW
		for (let ndi = nd_expr.c; ndi; ndi = ndi.s) {
			if (ndi.node_class == N_REF && (ndi.flags & REF_DECLARED)) {
				type = typing.ComputeDeclaredType(ndi);
				break;
			}
		}
		break;
	}
	case N_SYMBOL:
	case N_AIR: {
		//we don't really understand these, so we can't compute a type
		//and it's not worth caching
		type = undefined;
		break;
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
		type = undefined;
		break;
	}
	case N_FUNCTION:
	case N_CLASS: {
		//they are self-representing, don't cache
		type = nd_expr;
		break;
	}
	case N_PAREN: {
		type = typing.ComputeType(nd_expr.c);
		break;
	}
	case N_STRING: {
		type = typing.options.string_type;
		break;
	}
	case N_JS_REGEXP: {
		type = typing.options.regexp_type;
		break;
	}
	case N_NODEOF: {
		type = typing.options.node_type;
		break;
	}
	case N_NUMBER: {
		type = typing.options.ComputeNumberType(nd_expr);
		break;
	}
	case N_POSTFIX: {
		if (typing.options.non_type_postfixes.has(nd_expr.data)) {
			//COULDDO: try to find overloaded operators
			//here we cheat and return the operand
			type = typing.ComputeType(nd_expr.c);
			break;
		} else {
			//we are self-representative
			type = nd_expr;
			break;
		}
	}
	case N_PREFIX: {
		//COULDDO: try to find overloaded operators
		//here we cheat and return the operand
		type = typing.ComputeType(nd_expr.c);
		if (nd_expr.data == '&') {
			type = typing.PointerType(type);
		}
		break;
	}
	case N_CONDITIONAL:
	case N_BINOP: {
		if (nd_expr.data == '||' || nd_expr.data == '&&') {
			if (typing.options.bool_type) {
				type = typing.options.bool_type;
				break;
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
		if (nd_expr.data == 'this') {
			let nd_owner = nd_expr.Owner();
			if (nd_owner.node_class == N_FUNCTION) {
				let nd_name = nd_owner.GetFunctionNameNode();
				if (nd_name && nd_name.node_class == N_DOT) {
					type = typing.ComputeType(nd_name.c);
				} else {
					let nd_owner_owner = nd_owner.Owner();
					if (nd_owner_owner.node_class == N_CLASS && nd_owner_owner.data != 'namespace') {
						type = nd_owner_owner;
					}
				}
			}
			if (type) {
				type = typing.PointerType(type);
				break;
			}
		}
		let all_defs = typing.LookupSymbol(nd_expr, true);
		for (let nd_def of all_defs) {
			let type_def = typing.ComputeDeclaredType(nd_def);
			if (!type_def) {continue;}
			//try to find a non-self-representing def
			type = type_def;
			if (type != nd_def) {
				break;
			}
		}
		if (!type) {
			//assume self-representing
			if (typing.options.nulls.has(nd_expr.data)) {
				type = typing.null_type;
			} else if (nd_expr.p && nd_expr.p.node_class == N_ASSIGNMENT && nd_expr == nd_expr.p.c.s) {
				//immediate r-value of an assignment, unlikely to be self-representing
			} else {
				type = nd_expr;
			}
		}
		break;
	}
	case N_DOT: {
		//try to look up the "primary" type first
		//COULDDO: substitute template parameters
		let type_obj = typing.TryGettingClass(typing.ComputeType(nd_expr.c));
		if (type_obj && type_obj.node_class == N_CLASS) {
			let all_defs = typing.LookupDottedName(nd_expr, nd_expr.data, type_obj, true);
			for (let nd_def of all_defs) {
				let type_def = typing.ComputeDeclaredType(nd_def);
				if (!type_def) {continue;}
				//try to find a non-self-representing def
				type = type_def;
				if (type != nd_def) {
					break;
				}
			}
		}
		//assume self-representing type name when we fail to find the def
		if (!type && nd_expr.flags == DOT_CLASS) {
			type = nd_expr;
		}
		break;
	}
	case N_CALL: {
		//COULDDO: substitute template parameters
		//COULDDO: try to resolve overloading
		let type_func = typing.ComputeType(nd_expr.c);
		let dedup = new Set();
		let count = 0;
		while (type_func && count < 1024 && !dedup.has(type_func) && type_func.node_class == N_CALL_TEMPLATE) {
			dedup.add(type_func);
			type_func = typing.ComputeType(type_func.c);
			count += 1;
		}
		if (type_func && type_func.node_class == N_FUNCTION) {
			type = typing.ComputeReturnType(type_func);
		}
		break;
	}
	case N_CALL_TEMPLATE: {
		let type_template = typing.ComputeType(nd_expr.c);
		if (type_template && type_template.node_class == N_FUNCTION) {
			//we are just a function, let the outer call worry about the return type
			type = type_template;
		} else {
			//we are self-representing
			type = nd_expr;
		}
		break;
	}
	case N_ITEM: {
		if (!nd_expr.c.s || nd_expr.c.s.node_class == N_SYMBOL) {
			//self-representing array type
			type = nd_expr;
			break;
		}
		//we should do this before sane_types: we want the untranslated *sane* types
		let type_obj = typing.ComputeType(nd_expr.c);
		//recognize array and pointer
		if (type_obj) {
			if (type_obj.node_class == N_ITEM) {
				type = type_obj.c;
				break;
			}
			if (type_obj.node_class == N_PREFIX && type_obj.data == '*') {
				type = type_obj.c;
				break;
			}
			if (type_obj.node_class == N_CALL_TEMPLATE && type_obj.GetName() == 'vector' && type_obj.c.s) {
				type = typing.ComputeType(type_obj.c.s);
			}
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
			ret = ret.dot(names.pop()).setFlags(DOT_CLASS);
		}
		typing.type_cache.set(ret, type);
		return ret;
	} else if (type.node_class == N_CALL_TEMPLATE) {
		let ret = type.Clone();
		ret.c.ReplaceWith(typing.AccessTypeAt(ret.c, nd_site));
		typing.type_cache.set(ret, type);
		return ret;
	}
	//COULDDO: N_REF case
	let ret = type.Clone();
	typing.type_cache.set(ret, type);
	return ret.setCommentsBefore('').setCommentsAfter('');
};

typing.DeduceAutoTypedDef = function(nd_def) {
	assert(nd_def.flags & REF_DECLARED);
	//COULDDO: more precise scope
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

/*
#filter Replace C++ `auto` with the deduced type whenever possible
This filter is intended for source feedback, i.e., save the deduction result to a file later.
The deduction is backed by ama's simple typing engine so the result may not be available or correct.
*/
typing.DeduceAuto = function(nd_root) {
	for (let nd_def of nd_root.FindAll(N_REF)) {
		if (!(nd_def.flags & REF_DECLARED) || nd_def.data == 'auto') {continue;}
		if (!(nd_def.p && nd_def.p.node_class == N_RAW && nd_def.p.Find(N_REF, 'auto'))) {continue;}
		typing.DeduceAutoTypedDef(nd_def);
	}
	typing.DropCache();
};
