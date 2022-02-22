#!/usr/bin/env ama
'use strict';
const path=require('path');
const fs=require('fs');
const fsext=require('fsext');
const depends=require('depends');
const classes=require('class');
const typing=require('cpp/typing');

(function(){
	let nd_node_hpp=depends.LoadFile(path.resolve(__dirname, '../src/ast/node.hpp'));
	let nd_node_class=nd_node_hpp.Find(N_CLASS,'Node');
	let desc=nd_node_class.ParseClass();
	for(let item of desc.properties){
		if(item.enumerable&&item.kind=='variable'){
			console.log(item,typing.ComputeType(item.node).dump());
		}
	}
	let dedup=new Set();
	for(let nd_method of nd_node_class.FindAll(N_FUNCTION)){
		let name_i=nd_method.data;
		if(!name_i){continue;}
		if(name_i=='LastChildSP'){continue;}
		let nd_type=typing.ComputeReturnType(nd_method);
		console.log('>>>',name_i,nd_type&&nd_type.dump());
		let nd_paramlist=nd_method.Find(N_PARAMETER_LIST);
		if(!nd_paramlist){continue;}
		for(let ndi=nd_paramlist.c;ndi;ndi=ndi.s){
			//console.log(ndi.GetName(),JSON.stringify(ndi));
			let nd_type=typing.ComputeType(ndi.FindAllDef()[0]);
			console.log(ndi.GetName(),nd_type&&nd_type.dump());
		}
		dedup.add(name_i);
	}
})();
