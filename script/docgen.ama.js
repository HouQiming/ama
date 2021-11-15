'use strict';
const path=require('path');
const fs=require('fs');
const fsext=require('fsext');
const depends=require('depends');

let g_templates={};
function ApplyMDTemplate(s_template,params){
	return s_template.replace(/【(.*)】/g,(match,pname)=>(params[pname]||'').toString());
}

function CleanupDocString(s){
	let paragraphs=[];
	let cur_paragraph=[];
	for(let line of s.split('\n')){
		if(line.match(/^[* \t/]+$/)){continue;}
		if(line==='/*'||line==='*/'){continue;}
		line=line.replace(/^[/]+/,'').trim();
		if(!line){
			if(cur_paragraph.length){
				paragraphs.push(cur_paragraph.join(' ').replace(/[ \t]+/,' ').trim()+'\n');
				cur_paragraph.length=0;
			}
			continue;
		}
		cur_paragraph.push(line);
	}
	return paragraphs.join('\n');
}

function CreateNodeAPIItem(name,nd_func,doc){
	let nd_paramlist=nd_func.Find(N_PARAMETER_LIST);
	if(!nd_paramlist){return;}
	let prototype=[];
	for(let ndi=nd_paramlist.c;ndi;ndi=ndi.s){
		let name=ndi.GetName();
		prototype.push(name);
		if(ndi.s){
			prototype.push(',');
		}
	}
	return ApplyMDTemplate(g_templates.node_api,{
		name:name,
		prototype:prototype.join(''),
		params:'',
		description:doc
	})
}

function GenerateDocuments(){
	//for copy-pasting: 【】
	for(let fn_template of fsext.FindAllFiles(path.join(__dirname,'doc_templates'))){
		if(path.extname(fn_template)==='.md'){
			g_templates[path.parse(fn_template).name.toLowerCase()]=fs.readFileSync(fn_template).toString();
		}
	}
	////////////////
	//TODO: all_node_fields
	let all_node_apis=[];
	let nd_node_hpp=depends.LoadFile(path.resolve(__dirname, '../src/ast/node.hpp'));
	let nd_node_class=nd_node_hpp.Find(N_CLASS,'Node');
	let dedup=new Set();
	for(let nd_method of nd_node_class.FindAll(N_FUNCTION)){
		let name_i=nd_method.data;
		let doc_i=CleanupDocString(nd_method.ParentStatement().comments_before);
		if(!name_i){continue;}
		all_node_apis.push(CreateNodeAPIItem(name_i,nd_method,doc_i));
		dedup.add(name_i);
	}
	let nd_init_js=depends.LoadFile(path.resolve(__dirname, '../modules/_init.js'));
	for(let match of nd_init_js.MatchAll(nAssignment(Node.MatchDot(.(Node),'name'),Node.MatchAny('impl')))){
		let name_i=match.name.data;
		if(dedup.has(name_i)){continue;}
		let doc_i=CleanupDocString(match.nd.ParentStatement().comments_before);
		//console.log(match.impl.dump());
		all_node_apis.push(CreateNodeAPIItem(name_i,match.impl,doc_i));
	}
	fs.writeFileSync(
		path.resolve(__dirname, '../doc/api_node.md'),
		ApplyMDTemplate(g_templates.node,{
			//fields:all_node_fields.join(''),
			apis:all_node_apis.join(''),
			//ctor:
		})
	)
	//ApplyMDTemplate(g_templates.node_api,)
	//TODO: markdown
}

module.exports=GenerateDocuments;
