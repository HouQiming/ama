/*
@ama
const amacpp=require('cpp/amacpp');
const fs=require('fs');
const path=require('path');
fs.writeFileSync(path.join(__dirname,'test_auto_header.hpp'),[
	'#ifndef __TEST_AUTOHEADER_H',
	'#define __TEST_AUTOHEADER_H',
	'namespace ama{',
	'	int test2();',
	'}',
	'#endif\n'
].join('\n'))
console.log(ParseCurrentFile({parse_indent_as_scope:1}).then(amacpp).toSource());
*/
#include "./test_auto_header.hpp"

namespace ama{
	public int test2(){
		return 42;
	}
	public int test(){
		return 42;
	}
	int ama::test3(){
		return 42;
	}
}
