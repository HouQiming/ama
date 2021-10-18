'use strict'
const path = require('path');

function Generate(version,my_call) {
	let nd_node_jch = undefined;
	let code_func = [];
	let code = [];
	if(version==='jc'){
		nd_node_jch=compiler.Load(path.resolve(__dirname, '../ast/node.jch'));
	}else{
		require('class');
		nd_node_jch=require('depends').LoadFile(path.resolve(__dirname, '../ast/node.hpp'));
		code_func.push('namespace ama{\n');
	}
	let class_name = 'Node';
	let class_full_name = 'ama::Node';
	let classid = 'ama::g_node_classid';
	let proto = 'ama::g_node_proto';
	function ClassifyType(nd_type) {
		let s_src = nd_type.toSource();
		if( s_src.indexOf('Node') >= 0 ) {
			if( s_src.indexOf('[') >= 0 ) {
				return 'Node[]';
			} else {
				return 'Node';
			}
		} else if( s_src.indexOf('char[') >= 0 ) {
			if( s_src.indexOf('^') >= 0 || s_src.indexOf('[|]') >= 0 ) {
				return 'string^';
			} else {
				return 'string';
			}
		} else if( s_src.indexOf('char*') >= 0 ) {
			return 'charptr';
		} else if( s_src.indexOf('void*') >= 0 ) {
			return 'void*';
		} else if( s_src.indexOf('JSValue') >= 0 ) {
			return 'JSValue';
		} else if( s_src.indexOf('float') >= 0 || s_src.indexOf('double') >= 0 ) {
			return 'float';
		} else {
			return 'int';
		}
	}
	function WrapValue(nd_type, jc_expr) {
		let s_type = ClassifyType(nd_type);
		if( s_type === 'Node' ) {
			return ['ama::WrapNode(', jc_expr, ')'].join('');
		} else if( s_type === 'void*' ) {
			return ['ama::WrapPointer(', jc_expr, ')'].join('');
		} else if( s_type === 'Node[]' ) {
			return ['ama::WrapNodeArray(', jc_expr, ')'].join('');
		} else if( s_type === 'string' ) {
			return ['ama::WrapString(', jc_expr, ')'].join('');
		} else if( s_type === 'string^' ) {
			return ['ama::WrapStringNullable(', jc_expr, ')'].join('');
		} else if( s_type === 'charptr' ) {
			return ['JS_NewString(jsctx,', jc_expr, ')'].join('');
		} else if( s_type === 'int' ) {
			//we don't have i64 in Node
			return ['JS_NewInt32(jsctx,', jc_expr, ')'].join('');
		} else if( s_type === 'float' ) {
			return ['JS_NewFloat64(jsctx,', jc_expr, ')'].join('');
		} else if( s_type === 'JSValue' ) {
			return jc_expr;
		} else {
			throw new Error('bad type ' + s_type);
		}
	}
	let res_counter = 0;
	function UnwrapValue(nd_type, js_value) {
		let ret = {
			validation: '',
			jc_expr: ''
		};
		let s_type = ClassifyType(nd_type);
		if( s_type === 'Node' ) {
			//we don't validate here: if it's not a node, we DO want a null
			ret.jc_expr = ['ama::UnwrapNode(', js_value, ')'].join('');
		} else if( s_type === 'void*' ) {
			ret.jc_expr = ['ama::UnwrapPointer(', js_value, ')'].join('');
		} else if( s_type === 'Node[]' ) {
			ret.validation = [
				'if(!JS_IsArray(', js_value, ')){return JS_ThrowTypeError(jsctx, "array expected for `', js_value, '`");}'
			].join('');
			ret.jc_expr = ['ama::UnwrapNodeArray(', js_value, ')'].join('');
		} else if( s_type === 'string' || s_type === 'string^' ) {
			ret.validation = [
				'if(!JS_IsString(', js_value, ')){return JS_ThrowTypeError(jsctx, "string expected for `', js_value, '`");}'
			].join('');
			//TODO: replace UnwrapString with sth cheaper
			ret.jc_expr = [nd_type.toSource().indexOf('[+]') >= 0 ? 'ama::UnwrapStringResizable(' : 'ama::UnwrapString(', js_value, ')'].join('');
		} else if( s_type === 'charptr' ) {
			ret.validation = [
				'if(!JS_IsString(', js_value, ')){return JS_ThrowTypeError(jsctx, "string expected for `', js_value, '`");}'
			].join('');
			ret.jc_expr = ['JS_ToCString(jsctx, ', js_value, ')'].join('');
		} else if( s_type === 'JSValue' ) {
			ret.validation = '';
			ret.jc_expr = js_value;
		} else if( s_type === 'int' ) {
			//we don't have i64 in Node
			ret.validation = [
				'int32_t res', res_counter, '=0;',
				'if(JS_ToInt32(jsctx, &res', res_counter, ', ', js_value, ')<0){return JS_ThrowTypeError(jsctx, "int expected for `', js_value, '`");}'
			].join('');
			ret.jc_expr = 'res' + res_counter.toString();
			res_counter += 1;
		} else if( s_type === 'float' ) {
			//we don't have i64 in Node
			ret.validation = [
				'double res', res_counter, '=0.0;',
				'if(JS_ToFloat64(jsctx, &res', res_counter, ', ', js_value, ')<0){return JS_ThrowTypeError(jsctx, "float expected for `', js_value, '`");}'
			].join('');
			ret.jc_expr = 'res' + res_counter.toString();
			res_counter += 1;
		} else {
			throw new Error('bad type ' + s_type);
		}
		return ret;
	}
	let properties=[];
	let methods=[];
	if(version==='jc'){
		let nd_class_scope = nd_node_jch.Find(N_CLASS,'Node').LastChild();
		nd_class_scope.FindAllWithin(BOUNDARY_SCOPE | BOUNDARY_FUNCTION | BOUNDARY_PROTOTYPE, N_DECLARATION, null).forEach(nd_i=>{
			let nd_def = nd_i.c;
			let nd_value = nd_i.c.s;
			if( !nd_value || nd_value.node_class === N_NULL || nd_value.node_class === N_NUMBER ) {
				let nd_type = nd_def.c;
				properties.push({
					name:nd_def.data,
					type:nd_type
				})
			}
		});
		nd_class_scope.FindAllWithin(BOUNDARY_SCOPE | BOUNDARY_FUNCTION, N_EXTERN, null).forEach(nd_i=> {
			let nd_def = nd_i.c;
			if( nd_def.data === 'CloneEx' || nd_def.data === 'CloneCB' || nd_def.data === 'forEach' ) {
				return;
			}
			if( nd_def.data === '__init__' || nd_def.data === '__done__' ) {
				return;
			}
			//if( nd_def.getTag('nojs') ) {
			//	return;
			//}
			//let nd_ret_type = nd_i.c.s.c.s;
			//if( nd_ret_type ) {
			//	if( nd_ret_type.getTag('nojs') ) {
			//		return;
			//	}
			//}
			methods.push({
				name:nd_def.data,
				paramlist:nd_i.c.s.c,
				return_type:nd_i.c.s.c.s
			})
		});
	}else{
		//console.log(JSON.stringify(nd_node_jch,null,1))
		const typing=require('cpp/typing');
		let class_desc = nd_node_jch.Find(N_CLASS,'Node').ParseClass();
		for(let ppt of class_desc.properties){
			if(ppt.enumerable){
				let nd_ref=ppt.node;
				properties.push({
					name:nd_ref.data,
					type:typing.ComputeType(nd_ref)
				})
			}
			if(ppt.method){
				if( ppt.name === 'CloneEx' || ppt.name === 'CloneCB' || ppt.name === 'forEach' ) {
					continue;
				}
				let nd_func=ppt.node;
				methods.push({
					name:nd_func.data,
					paramlist:nd_func.c.s,
					return_type:typing.ComputeReturnType(nd_func)
				});
			}
		}
	}
	for(let ppt of properties){
		let nd_type = ppt.type;
		let unwrap_code = UnwrapValue(nd_type, 'val');
		//all fields are nullable
		//nd_type.toSource().indexOf('?')>=0
		if( unwrap_code.validation && ClassifyType(nd_type) !== 'int' && ClassifyType(nd_type) !== 'float' ) {
			//it's nullable, hack the null case
			unwrap_code.validation = [
				'if(JS_IsNull(val)){',
				(
					'nd.' + ppt.name
				),
				'=NULL;',
				'}else{',
				unwrap_code.validation
			].join('');
			unwrap_code.jc_expr = unwrap_code.jc_expr + ';}';
		}
		code_func.push(
			'auto ', class_name, 'Get_', ppt.name, '(JSContext*+ jsctx, JSValueConst this_val){',
			'auto nd=(', class_full_name, '*)(JS_GetOpaque(this_val, ', classid, '));',
			'return ', WrapValue(nd_type, 'nd.' + ppt.name), ';',
			'}',
			'auto ', class_name, 'Set_', ppt.name, '(JSContext*+ jsctx, JSValueConst this_val, JSValueConst val){',
			'auto nd=(', class_full_name, '*)(JS_GetOpaque(this_val, ', classid, '));',
			unwrap_code.validation
		);
		if( ppt.name === 'sys_flags' || ppt.name === 'node_class' ) {
			code_func.push(
				'nd.' + ppt.name, '=(', nd_type.toSource(), ')(', unwrap_code.jc_expr, ')', ';'
			);
		} else {
			code_func.push(
				'nd.' + ppt.name, '=', unwrap_code.jc_expr, ';'
			);
		}
		code_func.push(
			'return JS_UNDEFINED;',
			'}'
		);
		code.push(
			'JS_DefinePropertyGetSet(jsctx,',
			proto, ',',
			//we don't plan to free these atoms
			'JS_NewAtom(jsctx,', JSON.stringify(ppt.name), '),',
			'JS_NewCFunction2(jsctx,(JSCFunction*+)(', class_name, 'Get_', ppt.name, '),"get_', ppt.name, '",0,JS_CFUNC_getter,0),',
			'JS_NewCFunction2(jsctx,(JSCFunction*+)(', class_name, 'Set_', ppt.name, '),"set_', ppt.name, '",1,JS_CFUNC_setter,0),',
			'JS_PROP_C_W_E',
			');\n'
		);
	}
	for(let method_i of methods){
		if( method_i.name === 'CloneEx' || method_i.name === 'CloneCB' ) {
			continue;
		}
		if( method_i.name === '__init__' || method_i.name === '__done__' ) {
			continue;
		}
		let nd_ret_type = method_i.return_type;
		code_func.push(
			'JSValue ', class_name, 'Call_', method_i.name, '(JSContext*+ jsctx, JSValueConst this_val, int argc, JSValueConst*+ argv){',
			class_full_name, '*+ nd=(', class_full_name, '*+)(JS_GetOpaque(this_val, ', classid, '));'
		);
		if( class_name === 'Node' ) {
			if( method_i.name === 'Insert' ) {
				code_func.push('if(nd==NULL){return JS_ThrowTypeError(jsctx, "cannot insert at a null node");}');
				code_func.push('if(JS_IsNull(argv[1])||JS_IsUndefined(argv[1])){return JS_ThrowTypeError(jsctx, "cannot insert a null node");}');
			}
		}
		let nd_paramlist = method_i.paramlist;
		let code_call = ['nd.', method_i.name, '('];
		//translate argv, skip the JC this
		let i = 0;
		for(let ndi = nd_paramlist.c; ndi; ndi = ndi.s) {
			let nd_def = ndi.c;
			if(version==='ama'){
				nd_def=nd_def.FindAll(N_REF,null).filter(nd=>nd.flags&REF_DECLARED)[0];
			}
			if( nd_def.GetName() === 'this' ) {
				continue;
			}
			let nd_type=undefined;
			if(version==='jc'){
				nd_type=nd_def.c;
			}else{
				const typing=require('cpp/typing');
				nd_type=typing.ComputeType(nd_def);
			}
			let unwrap_code = UnwrapValue(nd_type, '(' + i.toString() + '<argc?argv[' + i.toString() + 'L]:JS_UNDEFINED)');
			if( ClassifyType(nd_type) !== 'int' && ClassifyType(nd_type) !== 'float' && unwrap_code.validation ) {
				//it's nullable, hack the null case
				code_func.push(
					'if(argc>', i.toString(), '&&!JS_IsNull(argv[', i.toString(), 'L])){',
					unwrap_code.validation,
					'}'
				);
				unwrap_code.jc_expr = ['argc>', i.toString(), '&&!JS_IsNull(argv[', i.toString(), 'L])?', unwrap_code.jc_expr, ':NULL'].join('');
			} else {
				code_func.push(unwrap_code.validation);
			}
			if( i ) {
				code_call.push(',');
			}
			code_call.push(unwrap_code.jc_expr);
			i += 1;
		}
		code_call.push(')');
		let s_call = code_call.join('');
		//wrap the return value 
		let nd_ret_type_stripped=nd_ret_type;
		if(version==='jc'){
			nd_ret_type_stripped=nd_ret_type_stripped.StripTypeQualifiers();
		}
		if( nd_ret_type && !(new Set(['auto', 'void'])).has(nd_ret_type_stripped.toSource()) ) {
			code_func.push('return ', WrapValue(nd_ret_type, s_call), ';');
		} else {
			code_func.push(s_call, '; return JS_UNDEFINED;');
		}
		code_func.push(
			'}'
		);
		code.push(
			'JS_SetPropertyStr(jsctx,', proto, ',', JSON.stringify(method_i.name), ',',
			'JS_NewCFunction(jsctx,(', class_name, 'Call_', method_i.name, '),', JSON.stringify(class_name + '.' + method_i.name), ',', i.toString(), ')',
			');'
		);
	};
	let node_constants = [];
	if(version==='jc'){
		node_constants = nd_node_jch.Find(N_CLASS, 'ama').LastChild().FindAllWithin(
			BOUNDARY_CLASS | BOUNDARY_FUNCTION | BOUNDARY_PROTOTYPE, N_DEF, null
		).filter(nd => { return nd.p.node_class === N_DECLARATION; }).map(nd=>{return {
			nd_def:nd,
			nd_value:nd.p.c.s
		}});
	}else{
		let class_desc = nd_node_jch.Find(N_CLASS, 'ama').ParseClass();
		for(let ppt of class_desc.properties){
			if(ppt.enumerable){
				let nd_asgn = ppt.node.Owning(N_ASSIGNMENT);
				if(!nd_asgn){
					continue;
				}
				node_constants.push({
					nd_def:ppt.node,
					nd_value:nd_asgn.c.s
				});
			}
		}
	}
	let p_placeholder=code.length;
	code.push(
		'',
		'JSValueConst global = JS_GetGlobalObject(ama::jsctx);'
	);
	//JS global constants
	let n_largest = 0;
	for(let cns of node_constants) {
		let nd = cns.nd_def;
		let nd_value = cns.nd_value;
		if( nd.data.startsWith('N_') ) {
			function toCamelCase(s) {
				if( s === 'N' ) { return 'n'; }
				return s.substr(0, 1) + s.substr(1).toLowerCase();
			}
			if( nd_value.node_class === N_NUMBER ) {
				n_largest = parseInt(nd_value.data);
			}
			let builder_name = nd.data.split('_').map(toCamelCase).join('');
			code.push(
				'ama::g_node_class_names[',nd.data,'] = ',JSON.stringify(nd.data),';',
				'ama::g_builder_names[',nd.data,'] = ',JSON.stringify(builder_name),';'
			);
		} else if( nd_value.node_class !== N_FUNCTION ) {
			code.push(
				'JS_SetPropertyStr(ama::jsctx, global, ',JSON.stringify(nd.data),', JS_NewInt32(ama::jsctx, ',nd.data,'));'
			);
		}
	}
	n_largest += 1;
	code[p_placeholder] = [
		'ama::g_node_class_names.resize(',n_largest.toString(),');',
		'ama::g_builder_names.resize(',n_largest.toString(),');'
	].join('');
	//JS node builders are created in JS
	if(version==='jc'){
		my_call.RootStatement().InsertBefore(compiler.ParseCode(code_func.join('')).c || nEmpty());
		return compiler.ParseCode(code.join('')).c || nEmpty();
	}else{
		//my_call is actually nd_root
		code_func.push(
			'void GeneratedJSBindings(){\n',
			code.join(''),
			'}\n',
			//for namespace ama
			'}'
		);
		my_call.Insert(POS_BACK,ParseCode(code_func.join('').replace(/\*\+/g,'*')).AutoFormat().c.setCommentsAfter(''));
		my_call.Insert(POS_BACK,ParseCode('#pragma gen_end(js_bindings)').setCommentsBefore('\n'));
		return my_call;
	}
};

module.exports=Generate;
