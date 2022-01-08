const assert=require('assert');
const pipe=require('pipe.js');

module.exports=function Transform(nd_run_root){
	let nd_check_root=nd_run_root.Clone();
	let imms=[];
	for(let nd_immediate of nd_check_root.children){
		if(nd_immediate.node_class!=N_AIR&&nd_immediate.node_class!=N_FUNCTION&&!nd_immediate.FindAllDef().length&&!nd_immediate.Find(N_KEYWORD_STATEMENT,'data')){
			nd_immediate.Unlink();
			imms.push(nd_immediate);
		}
	}
	////////////
	for(let nd_func of nd_check_root.FindAll(N_FUNCTION)){
		let nd_rettype=nd_func.c.s.s;
		nd_rettype.ReplaceWith(nAir());
		if(nd_rettype.node_class==N_LABELED){
			nd_rettype=nd_rettype.c.s;
		}else if(nd_rettype.node_class==N_AIR){
			nd_rettype=nRef('auto');
		}
		nd_func.c.ReplaceWith(nRaw(nd_rettype.Clone(),nRef(nd_func.data).setCommentsBefore(' ')));
		for(let nd_param of nd_func.c.s.children){
			if(nd_param.node_class==N_LABELED){
				nd_param.ReplaceWith(@(@(nd_param.c.s.Clone()) const &@(nd_param.c.Clone())));
			}
		}
	}
	for(let nd_func of nd_run_root.FindAll(N_FUNCTION)){
		let nd_rettype=nd_func.c.s.s;
		if(nd_rettype.node_class!=N_AIR){
			nd_rettype.ReplaceWith(nAir());
		}
		for(let nd_param of nd_func.c.s.children){
			if(nd_param.node_class==N_LABELED){
				nd_param.ReplaceWith(nd_param.c.Clone());
			}
		}
	}
	////////////
	for(let nd_data of nd_check_root.FindAll(N_KEYWORD_STATEMENT,'data')){
		assert(nd_data.c.node_class==N_ASSIGNMENT);
		let nd_proto=nd_data.c.c;
		let nd_ctor=nd_data.c.c.s;
		assert(nd_proto.node_class==N_CALL_TEMPLATE);
		assert(nd_ctor.node_class==N_CALL);
		let template_args=[];
		for(let nd_arg=nd_proto.c.s;nd_arg;nd_arg=nd_arg.s){
			let template_uses=nd_ctor.FindAll(N_CALL_TEMPLATE,nd_arg.data);
			let arg_type='typename';
			if(template_uses.length){
				let arg_type_parts=['template<'];
				let n_args=template_uses[0].children.length-1;
				for(let i=0;i<n_args;i++){
					if(i){arg_type_parts.push(',');}
					arg_type_parts.push('typename');
				}
				arg_type_parts.push('>class');
				arg_type=arg_type_parts.join('');
			}
			template_args.push(nRaw(nRef(arg_type),nRef(nd_arg.data).setCommentsBefore(' ')));
		}
		let body=[];
		let ctor_params=[];
		let ctor_initializers=[];
		let field_counter=0;
		for(let ndi=nd_ctor.c.s;ndi;ndi=ndi.s){
			body.push(nSemicolon(nRaw(ndi.Clone(),nRef('m_field_'+field_counter.toString()).setCommentsBefore(' '))));
			ctor_params.push(@(@(ndi.Clone()) const& @(nRef('value_'+field_counter.toString()))));
			ctor_initializers.push(nSymbol(','),nCall(nRef('m_field_'+field_counter.toString()),nRef('value_'+field_counter.toString())));
			field_counter+=1;
		}
		if(ctor_initializers.length){ctor_initializers[0]=nSymbol(':');}
		body.push(nFunction(nd_proto.c.Clone(),nParameterList.apply(null,ctor_params),nRaw.apply(null,ctor_initializers),nScope()));
		nd_data.ReplaceWith(nScopedStatement(
			'template',nParameterList.apply(null,template_args).setFlags(PARAMLIST_TEMPLATE),
			nClass('struct',nAir(),nd_proto.c.Clone().setCommentsBefore(' '),nScope.apply(null,body))
		));
	}
	for(let nd_data of nd_run_root.FindAll(N_KEYWORD_STATEMENT,'data')){
		let nd_proto=nd_data.c.c;
		assert(nd_proto.node_class==N_CALL_TEMPLATE);
		nd_data.ParentStatement().ReplaceWith(@(function @(nd_proto.c.Clone())(...args){return args;}));
	}
	////////////
	if(imms.length){
		nd_check_root.Insert(POS_BACK,@(static void check_main(){@(nScope.apply(null,imms))}));
	}
	nd_check_root.TranslateTemplates([
		{from:@(Maybe),to:@(std::optional)},
		{from:@(number),to:@(double)},
		{from:@(let),to:@(auto)},
		{from:@(console.log),to:nAir()},
	],true);
	nd_check_root.Insert(POS_FRONT,@(
	#include <optional>
	#include <vector>
	template<typename T>using Array=std::vector<T>;
	template<typename T>T __fromMaybe(T const &dflt,std::optional<T> b){return b.value_or(dflt);}
	template<typename T,typename U>T __fromMaybe(U &&dflt,std::optional<T> b){return b.value_or(std::forward<U>(dflt));}
	));
	for(let nd_array of nd_check_root.FindAll(N_RAW)){
		if(!nd_array.isRawNode('[',']')){continue;}
		nd_array.flags=0x7d7b;//{}
	}
	for(let nd_just of nd_check_root.FindAll(N_CALL,'Just')){
		if(nd_just.c.node_class==N_CALL_TEMPLATE){
			nd_just.c.Insert(POS_AFTER,@(std::in_place));
			nd_just.c.c.ReplaceWith(@(std::optional))
		}
	}
	for(let nd_dflt of nd_check_root.FindAll(N_BINOP,'||')){
		nd_dflt.ReplaceWith(@(__fromMaybe(@(nd_dflt.c.s.Clone()),@(nd_dflt.c.Clone()))));
	}
	nd_run_root.Insert(POS_FRONT,@(function Just(a){return a;}));
	for(let nd_type_stuff of nd_run_root.FindAll(N_CALL_TEMPLATE)){
		nd_type_stuff.ReplaceWith(nd_type_stuff.c.Clone());
	}
	////////////
	let fn_cpp=nd_check_root.data+'.typecheck.audit.cpp';
	nd_check_root.Save(fn_cpp);
	let status=pipe.run('c++ -c -std=c++17 '+fn_cpp)
	if(status!=0){process.exit(status);}
	////////////
	return nd_run_root;
}

module.exports.setup = function(code, options) {
	Object.assign(options, {
		keywords_statement:options.keywords_statement+' data'
	});
};
