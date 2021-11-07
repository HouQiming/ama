module.exports = {
	templates: [
		{
			pattern: Node.MatchAny(N_REF, 'foo'),
			check: {
				foo: {
					addr_declared: {not_empty: 1,msg: 'undeclared identifier `{code}`'}
				}
			}
		}
	]
};
