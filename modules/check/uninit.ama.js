module.exports = {
	default_value: {initialized: 0},
	templates: [
		{
			pattern: Node.MatchAny(N_REF, 'foo'),
			filter: match=>{
				let nd_ref = match.foo;
				if (nd_ref.Owning(N_PARAMETER_LIST)) {return 0;}
				if (nd_ref.Owner().node_class != N_FUNCTION) {return 0;}
				return 1;
			},
			check: {
				foo: {
					addr_declared: {not_empty: 1,msg: 'uninitialized variable `{code}`'}
				}
			}
		},
		{
			pattern: .(.(Node.MatchAny('foo')) = .(Node.MatchAny('bar'))),
			set: {
				foo: {initialized: 1}
			}
		}
	]
};
