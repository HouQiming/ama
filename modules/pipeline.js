'use strict';
let pipeline = module.exports;
pipeline.GetFunctionByName = function(name) {
	if(name==='ParseSimplePairing'){
		return ParseSimplePairing;
	}
	if(Node[name]){
		return function(nd,options){
			return nd[name](options);
		}
	}
	throw new Error('unknown function ' + JSON.stringify(name));
};

pipeline.Run = function(p, input) {
	//p is an array of functions / function names / option objects
	//use functions to update options functionally
	let options = Object.create(null);
	for (let i = 0; i < p.length; i++) {
		let item = p[i];
		if (typeof(item) === 'string') {
			item = pipeline.GetFunctionByName(item);
		}
		if (typeof(item) === 'function') {
			let ret = item(input, options);
			if (ret != undefined) {
				input = ret;
			}
		} else if (typeof(item) === 'object') {
			Object.assign(options, item);
		} else {
			throw new Error('invalid pipeline item [' + i.toString() + ']')
		}
	}
	return input;
};

//TODO: inversion: cannot reverse the basic parsing part - need a separator
