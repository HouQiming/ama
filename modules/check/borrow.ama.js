module.exports = {
	templates: [
		{
			pattern: .(&.(Node.MatchAny('foo'))),
			nd: (values, vars, name)=>{
				//console.log(vars['<utag>'], name)
				return values.map(v=>Sandbox.set(v, {
					ref_vars_utag: vars['<utag>'],
					ref_name: name,
					ref_mutable: 1,
				}))
			}
		}
		//TODO: assign-to-const: reset ref_mutable
	],
	destructors: [
		(values, vars, name)=> {
			let utag = vars['<utag>'];
			let utag_parent = Sandbox.ctx_map[utag].utag_parent;
			if (utag_parent == undefined) {return;}
			let ctx_parent = Sandbox.ctx_map[utag_parent];
			let dangling_pointers = Sandbox.FindValues(ctx_parent.vars, value=>{
				return value.ref_vars_utag == utag && value.ref_name == name;
			});
			if (dangling_pointers.length) {
				return dangling_pointers.map(value_ptr=>{
					return {
						error: 'dangling pointer to `{code}`',
						value: value_ptr
					}
				});
			}
		}
	]
};
