let g_builtin_names = new Set(['__half', 'half', 'float', 'double', 'bool', 'char', 'int8_t', 'uint8_t', 'short', 'int16_t', 'uint16_t', 'int', 'long', 'unsigned', 'int32_t', 'uint32_t', 'int64_t', 'uint64_t']);
module.exports = {
	templates: [
		{
			pattern: Node.MatchAny(N_REF, 'foo'),
			filter: match=>{
				let nd_ref = match.foo;
				if (g_builtin_names.has(nd_ref.data)) {return 0;}
				if (nd_ref.flags & REF_DECLARED) {return 0;}
				return 1;
			},
			check: {
				foo: {
					as_function: {not_empty: 1,action: 'skip'},
					addr_declared: {not_empty: 1,msg: 'undeclared identifier `{code}`'}
				}
			}
		}
	]
};
