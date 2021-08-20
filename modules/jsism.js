'use strict'
let jsism=module.exports;

let console_method_to_options=Object.create(null,{
	log:{
		separator:' ',
		tail:'\n',
		ostream:'std::cout',
		std_stream:'stdout',
	},
	error:{
		separator:' ',
		tail:'\n',
		ostream:'std::cerr',
		std_stream:'stderr',
	},
	/////////////
	//extensions
	write:{
		separator:'',
		tail:'',
		ostream:'std::cout',
		std_stream:'stdout',
	},
	writeError:{
		separator:'',
		tail:'',
		ostream:'std::cerr',
		std_stream:'stderr',
	},
	format:{
		separator:'',
		tail:'',
		ostream:'/* error: need the std::format backend */',
		std_stream:'/* error: need the std::format backend */',
		just_format:1,
	},
});
jsism.EnableConsole=function(nd_root,options){
	options=options||{};
	let backend=options.backend||'std::format';
	let console_uses=nd_root.FindAll(N_CALL,null).filter(nd_call=>{
		return nd_call.c.node_class==N_DOT&&nd_call.c.c.node_class==N_REF&&nd_call.c.c.data=='console'&&console_method_to_options[nd_call.c.data];
	});
	//console.log(nd_root.FindAll(N_CALL,null))
	if(!console_uses.length){
		return;
	}
	function ReplaceWithStdFormat(nd_call){
		let format_parts=[];
		let options=console_method_to_options[nd_call.c.data];
		let args=[nRef('std').dot('format').setFlag(DOT_CLASS),/*filled later*/null];
		for(let ndi=nd_call.c.s;ndi;ndi=ndi.s){
			if(format_parts.length&&options.separator){
				format_parts.push(options.separator);
			}
			let nd_value=ndi
			let fmt='';
			if(nd_value.node_class===N_STRING){
				format_parts.push(nd_value.GetStringValue())
				continue
			}
			if(nd_value.isMethodCall('padStart')&&nd_value.c.s&&nd_value.c.s.node_class===N_NUMBER){
				fmt=':>'+nd_value.c.s.data;
				nd_value=nd_value.c.c;
				if(nd_value.isMethodCall('toString')&&!nd_value.c.s){
					nd_value=nd_value.c.c;
				}
			}else if(nd_value.isMethodCall('padEnd')&&nd_value.c.s&&nd_value.c.s.node_class===N_NUMBER){
				fmt=':<'+nd_value.c.s.data;
				nd_value=nd_value.c.c;
				if(nd_value.isMethodCall('toString')&&!nd_value.c.s){
					nd_value=nd_value.c.c;
				}
			}
			if(nd_value.isMethodCall('toFixed')&&nd_value.c.s&&nd_value.c.s.node_class===N_NUMBER){
				if(!fmt){fmt=':';}
				fmt=fmt+'.'+nd_value.c.s.data+'f';
				nd_value=nd_value.c.c
			}
			args.push(nd_value);
		}
		if(options.tail){
			format_parts.push(options.tail);
		}
		args[1]=nString(format_parts.join(''));
		let nd_format=nCall.apply(null,args);
		if(!options.just_format){
			if(options.std_stream==='stdout'){
				nd_format=nCall(nRef('printf'),nString('%s'),nCall(nd_format.dot('c_str')))
			}else{
				nd_format=nCall(nRef('fprintf'),nRef(options.std_stream),nString('%s'),nCall(nd_format.dot('c_str')))
			}
		}
		nd_call.ReplaceWith(nd_format)
	}
	for(let nd_call of console_uses){
		if(backend==='std::format'||nd_call.c.data==='format'){
			ReplaceWithStdFormat(nd_call)
		}else{
			throw new Error('unknown backend '+backend)
		}
	}
	if(backend==='std::format'){
		my_call.Root().InsertDependency(DEP_C_INCLUDE,'<stdio.h>');
		my_call.Root().InsertDependency(DEP_C_INCLUDE,'<format>');
	}
}
