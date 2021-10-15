'use strict'
//@ama ParseCurrentFile().then(require("jcs").TranslateJCS)
let jsism = module.exports;

let console_method_to_options = {
	log: {
		separator: ' ',
		tail: '\n',
		ostream: 'std::cout',
		std_stream: 'stdout',
	},
	error: {
		separator: ' ',
		tail: '\n',
		ostream: 'std::cerr',
		std_stream: 'stderr',
	},
	/////////////
	//extensions
	write: {
		separator: '',
		tail: '',
		ostream: 'std::cout',
		std_stream: 'stdout',
	},
	writeError: {
		separator: '',
		tail: '',
		ostream: 'std::cerr',
		std_stream: 'stderr',
	},
	format: {
		separator: '',
		tail: '',
		ostream: 'std::ostringstream()',
		std_stream: '/* error: not supported */',
		just_format: 1,
	},
};
jsism.EnableConsole = function(nd_root, options) {
	options = options || {};
	let backend = options.backend || 'iostream';
	let console_uses = nd_root.FindAll(N_CALL, null).filter(nd_call=>{
		return nd_call.c.node_class == N_DOT && nd_call.c.c.node_class == N_REF && nd_call.c.c.data == 'console' && console_method_to_options[nd_call.c.data];
	});
	if (!console_uses.length) {
		return;
	}
	let need_iomanip = 0;
	let need_sstream = 0;
	let need_ios = 0;
	function ReplaceWithStdFormat(nd_call) {
		let format_parts = [];
		let options = console_method_to_options[nd_call.c.data];
		let args = [nRef('std').dot('format').setFlags(DOT_CLASS), /*filled later*/null];
		for (let ndi = nd_call.c.s; ndi; ndi = ndi.s) {
			if (format_parts.length && options.separator) {
				format_parts.push(options.separator);
			}
			let nd_value = ndi
			let fmt = '';
			if (nd_value.node_class == N_STRING) {
				format_parts.push(nd_value.GetStringValue())
				continue
			;}
			if (nd_value.isMethodCall('padStart') && nd_value.c.s && nd_value.c.s.node_class == N_NUMBER) {
				fmt = ':>' + nd_value.c.s.data;
				nd_value = nd_value.c.c;
				if (nd_value.isMethodCall('toString') && !nd_value.c.s) {
					nd_value = nd_value.c.c;
				}
			} else if(nd_value.isMethodCall('padEnd') && nd_value.c.s && nd_value.c.s.node_class == N_NUMBER) {
				fmt = ':<' + nd_value.c.s.data;
				nd_value = nd_value.c.c;
				if (nd_value.isMethodCall('toString') && !nd_value.c.s) {
					nd_value = nd_value.c.c;
				}
			}
			if (nd_value.isMethodCall('toExponential') && !nd_value.c.s) {
				if (!fmt) {fmt = ':';}
				fmt = fmt + 'e';
				nd_value = nd_value.c.c;
			} else if(nd_value.isMethodCall('toFixed') && nd_value.c.s && nd_value.c.s.node_class == N_NUMBER) {
				if (!fmt) {fmt = ':';}
				fmt = fmt + '.' + nd_value.c.s.data + 'f';
				nd_value = nd_value.c.c;
			}
			format_parts.push('{' + fmt + '}')
			args.push(nd_value);
		}
		if (options.tail) {
			format_parts.push(options.tail);
		}
		args[1] = nString(format_parts.join(''));
		let nd_format = nCall.apply(null, args);
		if (!options.just_format) {
			if (options.std_stream == 'stdout') {
				nd_format = nCall(nRef('printf'), nString('%s'), nCall(nd_format.dot('c_str')));  
			} else {
				nd_format = nCall(nRef('fprintf'), nRef(options.std_stream), nString('%s'), nCall(nd_format.dot('c_str')));  
			}
		}
		nd_format.comments_before = nd_call.c.c.comments_before
		nd_call.ReplaceWith(nd_format);
	}
	function ReplaceWithIOStream(nd_call) {
		let options = console_method_to_options[nd_call.c.data];
		let nd_stream = ParseCode(options.ostream).c;
		let is_first = 1;
		for (let ndi = nd_call.c.s; ndi;) {
			let ndi_next = ndi.s;
			if (is_first) {
				is_first = 0;
			} else if(options.separator) {
				nd_stream = nBinop(nd_stream, '<<', nString(options.separator));  
			}
			let nd_value = ndi
			if (nd_value.isMethodCall('padStart') && nd_value.c.s && nd_value.c.s.node_class == N_NUMBER) {
				need_iomanip = 1;
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('right').setFlags(DOT_CLASS))  ;
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('setw').setFlags(DOT_CLASS).call(nd_value.c.s))  
				nd_value = nd_value.c.c;
				if (nd_value.isMethodCall('toString') && !nd_value.c.s) {
					nd_value = nd_value.c.c;
				}
			} else if(nd_value.isMethodCall('padEnd') && nd_value.c.s && nd_value.c.s.node_class == N_NUMBER) {
				need_iomanip = 1;
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('left').setFlags(DOT_CLASS))  ;
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('setw').setFlags(DOT_CLASS).call(nd_value.c.s))  
				nd_value = nd_value.c.c;
				if (nd_value.isMethodCall('toString') && !nd_value.c.s) {
					nd_value = nd_value.c.c;}};
			
			
			if (nd_value.isMethodCall('toExponential') && !nd_value.c.s) {
				need_iomanip = 1;
				need_ios = 1
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('scientific').setFlags(DOT_CLASS))  
				nd_value = nd_value.c.c;
				nd_stream = nBinop(nd_stream, '<<', nd_value);
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('defaultfloat').setFlags(DOT_CLASS))  ;
				ndi = ndi_next;
				continue;
			} else if(nd_value.isMethodCall('toFixed') && nd_value.c.s && nd_value.c.s.node_class == N_NUMBER) {
				need_iomanip = 1;
				need_ios = 1;
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('fixed').setFlags(DOT_CLASS))  
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('setprecision').setFlags(DOT_CLASS).call(nd_value.c.s))  ;
				nd_value = nd_value.c.c;
				nd_stream = nBinop(nd_stream, '<<', nd_value);
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('defaultfloat').setFlags(DOT_CLASS))  ;
				ndi = ndi_next;
				continue;
			}
			nd_stream = nBinop(nd_stream, '<<', nd_value);
			ndi = ndi_next;
		}
		if (options.tail) {
			nd_stream = nBinop(nd_stream, '<<', options.tail == '\n' ? nRef('std').dot('endl').setFlags(DOT_CLASS) : nString(options.tail));  
		}
		if (options.just_format) {
			need_sstream = 1;
			nd_stream = nCall(nSymbol('(std::ostringstream&)'), nd_stream).enclose('()').dot('str').call();
		}
		nd_call.ReplaceWith(nd_stream);
		nd_stream.comments_after = '';
	}
	for (let nd_call of console_uses) {
		if (backend == 'std::format') {
			ReplaceWithStdFormat(nd_call);
		} else if(backend == 'iostream') {
			ReplaceWithIOStream(nd_call);
		} else {
		throw new Error('unknown backend ' + backend)
		;
		}
	}
	if (backend == 'std::format') {
		nd_root.InsertDependency(DEP_C_INCLUDE | DEPF_C_INCLUDE_NONSTR, '<format>');
		nd_root.InsertDependency(DEP_C_INCLUDE | DEPF_C_INCLUDE_NONSTR, '<stdio.h>');
	} else if(backend == 'iostream') {
		if (need_ios) {
			nd_root.InsertDependency(DEP_C_INCLUDE | DEPF_C_INCLUDE_NONSTR, '<ios>');
		}
		if (need_iomanip) {
			nd_root.InsertDependency(DEP_C_INCLUDE | DEPF_C_INCLUDE_NONSTR, '<iomanip>');
		}
		if (need_sstream) {
			nd_root.InsertDependency(DEP_C_INCLUDE | DEPF_C_INCLUDE_NONSTR, '<sstream>');
		}
		nd_root.InsertDependency(DEP_C_INCLUDE | DEPF_C_INCLUDE_NONSTR, '<iostream>');}};



jsism.EnableJSON = function(nd_root) {};
	//TODO: stringify / parse callback generation
	//how to differentiate on-call vs persistent?
	//on-call generation needs type system
	//for persistent, we want StringifyToImpl in header: #pragma inside the class
	//#pragma for impl for now?


jsism.EnableJSLambdaSyntax = function(nd_root) {
	//()=>{} <=> [&](){}
	for (let nd_func of nd_root.FindAll(N_FUNCTION, null)) {
		let nd_before = nd_func.c;
		let nd_after = nd_func.c.s.s;
		if (nd_after.isSymbol('=>')) {
			nd_after.ReplaceWith(nAir()).setCommentsBefore('').setCommentsAfter('');
			nd_before.ReplaceWith(.([&]));}}};




jsism.EnableJSLambdaSyntax.inverse = function(nd_root) {
	//()=>{} <=> [&](){}
	let nd_template = .([&]);
	for (let nd_func of nd_root.FindAll(N_FUNCTION, null)) {
		let nd_before = nd_func.c;
		let nd_after = nd_func.c.s.s;
		if (nd_before.Match(nd_template)) {
			nd_after.ReplaceWith(nSymbol('=>')).setCommentsBefore(' ');
			nd_before.ReplaceWith(nAir());
		}
	}
};
