let g_builtin_names = new Set(['__half', 'half', 'float', 'double', 'bool', 'char', 'int8_t', 'uint8_t', 'short', 'int16_t', 'uint16_t', 'int', 'long', 'unsigned', 'int32_t', 'uint32_t', 'int64_t', 'uint64_t']);
module.exports = {
	default_value: {initialized: 0},
	templates: [
		{
			pattern: Node.MatchAny(N_REF, 'foo'),
			filter: match=>{
				let nd_ref = match.foo;
				let nd_asgn = nd_ref.Owning(N_ASSIGNMENT);
				if (nd_asgn && nd_asgn.c.isAncestorOf(nd_ref)) {return 1;}
				return 0;
			},
			set: {
				foo: {initialized: 1}
			}
		},
		{
			pattern: Node.MatchAny(N_REF, 'foo'),
			filter: match=>{
				let nd_ref = match.foo;
				if (nd_ref.Owning(N_PARAMETER_LIST)) {return 0;}
				if (nd_ref.Owner().node_class != N_FUNCTION) {return 0;}
				if (g_builtin_names.has(nd_ref.data)) {return 0;}
				if (nd_ref.flags & REF_DECLARED) {return 0;}
				return 1;
			},
			check: {
				foo: {
					as_function: {not_empty: 1,action: 'skip'},
					addr_declared: {empty: 1,action: 'skip'},
					initialized: {must_be: 1,msg: 'uninitialized variable `{code}`'}
				}
			}
		}
	]
};
