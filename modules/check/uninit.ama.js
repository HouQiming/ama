let g_builtin_names = new Set(['__half', 'half', 'float', 'double', 'bool', 'char', 'int8_t', 'uint8_t', 'short', 'int16_t', 'uint16_t', 'int', 'long', 'unsigned', 'int32_t', 'uint32_t', 'int64_t', 'uint64_t']);
module.exports = {
	default_value: {initialized: 0},
	templates: [
		{
			pattern: Node.MatchAny(N_REF, 'foo'),
			filter: match=> {
				let nd_ref = match.foo;
				if (nd_ref.Owning(N_PARAMETER_LIST)) {return 0;}
				if (nd_ref.Owner().node_class != N_FUNCTION) {return 0;}
				if (g_builtin_names.has(nd_ref.data)) {return 0;}
				if (nd_ref.flags & REF_DECLARED) {return 0;}
				return 1;
			},
			foo: (values, extra_args, vars, name)=> {
				if (values.filter(v=>v.as_function).length) {return;}
				//if (!values.filter(v=>v.addr_declared).length) {return;}
				let bad_values = values.filter(v=>!v.initialized);
				if (bad_values.length) {
					return {
						error: 'uninitialized variable `{code}`',
						value: bad_values[0]
					}
				}
			}
		},
		{
			//we can only tag initialization after checking
			pattern: nAssignment(Node.MatchAny('bar'), Node.MatchAny('foo')),
			foo: values=>values.map(v=>Sandbox.set(v, {initialized: 1}))
		}
	]
};
