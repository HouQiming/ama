/*
@ama
//let nd_root=ParseCurrentFile();
//console.log(JSON.stringify(nd_root,null,1));
//console.log(nd_root.toSource());
const amacpp=require('cpp/amacpp');
let nd_root=ParseCurrentFile({parse_indent_as_scope:1});
console.log(JSON.stringify(nd_root,null,1));
nd_root.then(amacpp);
console.log(nd_root.toSource());
*/
namespace bad{
	int T;
	int U;
	int V;
	int V1;
	int template;
}

template<template<class T>class U,int V=3>
T testFunction(){
	return V;
}

template<int V=3,class V1,template<class T>class U>
class TClass:V1{
	U<V1> field;
}

V test2(T a,U b,V1 c){
	return V;
}
