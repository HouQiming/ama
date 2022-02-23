'use strict';
const assert = require('assert');
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
/*
#filter Translate `console.log` to `std::cout << foo`
The filter also supports some Javascript formatting methods like `toFixed` and `padStart`.
Before:
```C++
int main(){
	console.log("hello world",(0.25).toFixed(3));
	return 0;
}
```
*/
jsism.EnableConsole = function(nd_root, options) {
	options = options || {};
	let backend = options.backend || 'iostream';
	let console_uses = nd_root.FindAll(N_CALL).filter(nd_call => {
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
			let nd_value = ndi;
			let fmt = '';
			if (nd_value.node_class == N_STRING) {
				format_parts.push(nd_value.GetStringValue());
				continue;
			}
			if (nd_value.isMethodCall('padStart') && nd_value.c.s && nd_value.c.s.node_class == N_NUMBER) {
				fmt = ':>' + nd_value.c.s.data;
				nd_value = nd_value.c.c;
				if (nd_value.isMethodCall('toString') && !nd_value.c.s) {
					nd_value = nd_value.c.c;
				}
			} else if (nd_value.isMethodCall('padEnd') && nd_value.c.s && nd_value.c.s.node_class == N_NUMBER) {
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
			} else if (nd_value.isMethodCall('toFixed') && nd_value.c.s && nd_value.c.s.node_class == N_NUMBER) {
				if (!fmt) {fmt = ':';}
				fmt = fmt + '.' + nd_value.c.s.data + 'f';
				nd_value = nd_value.c.c;
			}
			format_parts.push('{' + fmt + '}');
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
		nd_format.comments_before = nd_call.c.c.comments_before;
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
			} else if (options.separator) {
				nd_stream = nBinop(nd_stream, '<<', nString(options.separator));  
			}
			let nd_value = ndi;
			if (nd_value.isMethodCall('padStart') && nd_value.c.s && nd_value.c.s.node_class == N_NUMBER) {
				need_iomanip = 1;
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('right').setFlags(DOT_CLASS))  ;
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('setw').setFlags(DOT_CLASS).call(nd_value.c.s));  
				nd_value = nd_value.c.c;
				if (nd_value.isMethodCall('toString') && !nd_value.c.s) {
					nd_value = nd_value.c.c;
				}
			} else if (nd_value.isMethodCall('padEnd') && nd_value.c.s && nd_value.c.s.node_class == N_NUMBER) {
				need_iomanip = 1;
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('left').setFlags(DOT_CLASS))  ;
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('setw').setFlags(DOT_CLASS).call(nd_value.c.s));  
				nd_value = nd_value.c.c;
				if (nd_value.isMethodCall('toString') && !nd_value.c.s) {
					nd_value = nd_value.c.c;
				}
			}
			if (nd_value.isMethodCall('toExponential') && !nd_value.c.s) {
				need_iomanip = 1;
				need_ios = 1;
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('scientific').setFlags(DOT_CLASS));  
				nd_value = nd_value.c.c;
				nd_stream = nBinop(nd_stream, '<<', nd_value);
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('defaultfloat').setFlags(DOT_CLASS))  ;
				ndi = ndi_next;
				continue;
			} else if (nd_value.isMethodCall('toFixed') && nd_value.c.s && nd_value.c.s.node_class == N_NUMBER) {
				need_iomanip = 1;
				need_ios = 1;
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('fixed').setFlags(DOT_CLASS));  
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('setprecision').setFlags(DOT_CLASS).call(nd_value.c.s))  ;
				nd_value = nd_value.c.c;
				nd_stream = nBinop(nd_stream, '<<', nd_value);
				nd_stream = nBinop(nd_stream, '<<', nRef('std').dot('defaultfloat').setFlags(DOT_CLASS))  ;
				ndi = ndi_next;
				continue;
			}
			nd_stream = nBinop(nd_stream, '<<', nParen(nd_value));
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
		} else if (backend == 'iostream') {
			ReplaceWithIOStream(nd_call);
		} else {
			throw new Error('unknown backend ' + backend);
		}
	}
	if (backend == 'std::format') {
		nd_root.InsertCInclude('<format>');
		nd_root.InsertCInclude('<stdio.h>');
	} else if (backend == 'iostream') {
		if (need_ios) {
			nd_root.InsertCInclude('<ios>');
		}
		if (need_iomanip) {
			nd_root.InsertCInclude('<iomanip>');
		}
		if (need_sstream) {
			nd_root.InsertCInclude('<sstream>');
		}
		nd_root.InsertCInclude('<iostream>');
	}
};

/*
#filter Enable `JSON.stringify` and `JSON.parse<T>` in C++
To use this filter, you need to: 
```C++
#include "<.ama_modules dir>/modules/cpp/json/json.h"
```
And add `json.cpp` to your project. For each class you wish to stringify or parse, add:
```C++
#pragma gen(JSON.stringify<YourClass>)
```
or
```C++
#pragma gen(JSON.parse<YourClass>)
```
in a file with this filter enabled. The pragmas will be translated to stringify / parse implementation.
If the class is in the same file as the `JSON.foo`, the pragma can be omitten.

Before:
```C++
#include <iostream>
#include "<.ama_modules dir>/modules/cpp/json/json.h"

struct Test{
	int a;
};
int main(){
	Test obj{3};
	std::cout<<JSON.stringify(obj)<<std::endl;
	return 0;
}
```
*/
jsism.EnableJSON = function(nd_root) {
	//stringify / parse callback generation
	//for persistent, we want StringifyToImpl in header
	//replace #pragma with JSON impl
	//generate #pragma when call scanner finds a type without implementation
	const typing = require('cpp/typing');
	const gentag = require('cpp/gentag');
	require('class');
	for (let nd_json of nd_root.FindAll(N_REF, 'JSON')) {
		let nd_parent = nd_json.p;
		if (nd_parent.node_class != N_DOT || nd_parent.flags != 0) {continue;}
		let nd_owning_pragma = nd_json.Owning(N_KEYWORD_STATEMENT);
		if (nd_owning_pragma && nd_owning_pragma.data == '#pragma') {continue;}
		let nd_call = nd_parent.p;
		let type = undefined;
		if (nd_parent.data == 'stringify' && nd_call && nd_call.node_class == N_CALL && nd_call.c.s) {
			type = typing.ComputeType(nd_call.c.s);
		} else if (nd_parent.data == 'parse' && nd_call && nd_call.node_class == N_CALL_TEMPLATE && nd_call.c.s) {
			type = typing.ComputeType(nd_call.c.s);
		} else {
			continue;
		}
		if (type && type.node_class == N_CLASS) {
			//look for implementation in both class header and the call site
			//#pragma gen(JSON.stringify<foo>)
			//#pragma gen_begin(JSON.stringify<foo>)
			//#pragma gen_end(JSON.stringify<foo>)
			//dedicated finder for generator tags? replacement / inverse? make generating passes use those tags?
			//how do we differentiate request from result? replace gen with gen_begin and gen_end
			let nd_tag = @(JSON.foo<@(typing.AccessTypeAt(type, nd_root))>);
			nd_tag.Find(N_DOT, 'foo').data = nd_parent.data;
			let match_gentag = gentag.FindGenTag(nd_root, nd_tag);
			if (!match_gentag) {
				match_gentag = gentag.FindGenTag(type.Root(), nd_tag);;
			}
			if (!match_gentag) {
				//create a new gentag and insert it
				//COULDDO: insert recursively
				let nd_gentag = @(#pragma gen(@(nd_tag)));
				if (type.Root() == nd_root) {
					type.ParentStatement().Insert(POS_AFTER, nd_gentag.setCommentsBefore('\n'));
				} else {
					//let deps = nd_root.FindAll(N_DEPENDENCY, null);
					//if (deps.length) {
					//	deps[deps.length - 1].Insert(POS_AFTER, nd_gentag.setCommentsBefore('\n'));
					//} else {
					//	nd_root.Insert(POS_FRONT, nd_gentag);
					//}
					nd_root.Insert(POS_BACK, nd_gentag);
				}
			}
		}
		//make it `JSON::`
		nd_parent.flags = DOT_CLASS;
	}
	//generate code for the gentags
	for (let match of gentag.FindAllGenTags(nd_root, @(JSON.stringify<@(Node.MatchAny('type'))>))) {
		let type = typing.ComputeType(match.type);
		if (type.node_class != N_CLASS) {continue;}
		//generate and replace
		let desc = type.ParseClass();
		let nd_generated = @(
			namespace JSON {
				template <>
				struct StringifyToImpl<@(typing.AccessTypeAt(type, match.gentag))> {
					//`type` is only used for SFINAE
					typedef void type;
					template <typename T = @(typing.AccessTypeAt(type, match.gentag))>
					static void stringifyTo(std::string& buf, @(typing.AccessTypeAt(type, match.gentag)) const& a) {
						buf.push_back('{');
						__INSERT_HERE;
						buf.push_back('}');
					}
				};
			}
		);
		let body = [];
		for (let ppt of desc.properties) {
			if (!ppt.enumerable) {continue;}
			//comment tag check
			let nd_stmt = ppt.node.ParentStatement();
			if (nd_stmt.comments_before.indexOf('nojson') >= 0 || nd_stmt.comments_after.indexOf('nojson') >= 0) {
				continue;
			}
			if (body.length) {
				body.push(@(
					buf.push_back(',');));
			}
			body.push(@(
				buf.append(@(nString(JSON.stringify(ppt.name) + ':')));));
			body.push(@(
				JSON::stringifyTo(buf, @(nRef('a').dot(ppt.name)));));
		}
		nd_generated.Find(N_REF, '__INSERT_HERE').ParentStatement().ReplaceWith(nScope.apply(null, body).c);
		gentag.UpdateGenTagContent(match.gentag, nd_generated);
	}
	for (let match of gentag.FindAllGenTags(nd_root, @(JSON.parse<@(Node.MatchAny('type'))>))) {
		let type = typing.ComputeType(match.type);
		if (type.node_class != N_CLASS) {continue;}
		//generate and replace
		let desc = type.ParseClass();
		let nd_generated = @(
			namespace JSON {
				template <>
				struct ParseFromImpl<@(typing.AccessTypeAt(type, match.gentag))> {
					//`type` is only used for SFINAE
					typedef void type;
					template <typename T = @(typing.AccessTypeAt(type, match.gentag))>
					static @(typing.AccessTypeAt(type, match.gentag)) parseFrom(JSONParserContext& ctx, @(typing.AccessTypeAt(type, match.gentag))**) {
						T ret{};
						if ( ctx.begin == ctx.end ) {
							return std::move(ret);
						}
						ctx.SkipSpace();
						if ( *ctx.begin != '{' ) {
							ctx.error = "'{' expected";
							ctx.error_location = ctx.begin;
							return std::move(ret);
						}
						ctx.begin += 1;
						ctx.SkipSpace();
						while ( ctx.begin != ctx.end && ctx.begin[0] != '}' ) {
							ctx.SkipSpace();
							__PARSE_FIELD_HERE;
							skip:
							ctx.SkipStringBody();
							skip_after_name:
							ctx.SkipField();
							done:
							if ( ctx.error ) {
								return std::move(ret);
							}
							ctx.SkipSpace();
							if ( ctx.begin != ctx.end && (*ctx.begin == ',' || *ctx.begin == '}') ) {
								if ( *ctx.begin == ',' ) {
									ctx.begin += 1;
								}
							} else {
								ctx.error = "',' or '}' expected";
								ctx.error_location = ctx.begin;
								return std::move(ret);
							}
						}
						if ( ctx.begin != ctx.end ) {
							ctx.begin += 1;
						}
						return std::move(ret);
					}
				};
			}
		);
		let properties = [];
		for (let ppt of desc.properties) {
			if (!ppt.enumerable) {continue;}
			//comment tag check
			let nd_stmt = ppt.node.ParentStatement();
			if (nd_stmt.comments_before.indexOf('nojson') >= 0 || nd_stmt.comments_after.indexOf('nojson') >= 0) {
				continue;
			}
			if (!ppt.name) {continue;}
			ppt.type = typing.ComputeType(ppt.node);
			if (!ppt.type) {continue;}
			ppt.json_name = JSON.stringify(ppt.name);
			properties.push(ppt);
		}
		properties.sort((a, b) => (a.json_name < b.json_name ? -1 : (a.json_name > b.json_name ? 1 : 0)));
		function GenerateParseField(properties, lg_eaten) {
			if (!properties.length) {return nAir();}
			//eating the common prefix
			let name0 = properties[0].json_name;
			let name1 = properties[properties.length - 1].json_name;
			let lg_prefix = Math.min(__byte_length(name0), __byte_length(name1)) - lg_eaten;
			for (let i = 0; i < lg_prefix; i++) {
				if (__byte_at(name0, i + lg_eaten) != __byte_at(name1, i + lg_eaten)) {
					lg_prefix = i;
					break;
				}
			}
			let nd_prefix = undefined;
			if (lg_prefix > 0) {
				nd_prefix = @(
					if ( !ctx.TrySkipName(@(nString(__byte_substr(name0, lg_eaten, lg_prefix)))) ) {
						@(lg_eaten ? @(goto skip;) : @(
							ctx.SkipField();
							goto skip_after_name;))
					} else {
						__SWITCH;
					}
				);
				lg_eaten += lg_prefix;
			}
			//no-switch special case
			let nd_switch = undefined;
			if (properties.length == 1) {
				nd_switch = @({
					ctx.SkipColon();
					if ( ctx.error ) {
						return std::move(ret);
					}
					@(nRef('ret').dot(properties[0].name)) = JSON::parseFrom(ctx, (@(typing.AccessTypeAt(properties[0].type, match.gentag))**)(NULL));
					if ( ctx.error ) {
						return std::move(ret);
					}
					goto done;
				});
			} else {
				//there cannot be a directly-return case: quotes should ensure that for us
				//first-byte switch
				let i0 = 0;
				let byte_i0 = undefined;
				let cases = [];
				for (let i = 0; i < properties.length; i++) {
					assert(lg_eaten < __byte_length(properties[i].json_name));
					let byte_i = __byte_at(properties[i].json_name, lg_eaten) | 0;
					if (byte_i0 == undefined || byte_i != byte_i0) {
						if (i0 < i) {
							cases.push({
								start: i0,
								end: i,
								indicator: byte_i0
							});
						}
						i0 = i;
						byte_i0 = byte_i;
					}
				}
				if (i0 < properties.length) {
					cases.push({
						start: i0,
						end: properties.length,
						indicator: byte_i0
					});
				}
				//the default clause breaks and skips
				nd_switch = @(
					switch (*ctx.begin) {}
				);
				let nd_body = nd_switch.Find(N_SCOPE, null);
				for (let i = 0; i < cases.length; i++) {
					let nd_char = undefined;
					if (cases[i].indicator >= 32 && cases[i].indicator < 127) {
						nd_char = nString(String.fromCharCode(cases[i].indicator)).setFlags(STRING_SINGLE_QUOTED | LITERAL_PARSED);
					} else {
						nd_char = nNumber(cases[i].indicator.toString());
					}
					//we must not have ; after the @(): it screws up multi-node insertion
					nd_body.Insert(POS_BACK, @(
						case @(nd_char): {
							ctx.begin += 1;
							@(GenerateParseField(properties.slice(cases[i].start, cases[i].end), lg_eaten + 1))/*
							*/break;
						}
					));
				}
			}
			if (nd_prefix) {
				nd_prefix.Find(N_REF, '__SWITCH').ParentStatement().ReplaceWith(nd_switch);
				nd_switch = nd_prefix.setCommentsBefore('');
			}
			return nd_switch;
		}
		nd_generated.Find(N_REF, '__PARSE_FIELD_HERE').ParentStatement().ReplaceWith(GenerateParseField(properties, 0));
		gentag.UpdateGenTagContent(match.gentag, nd_generated);
	}
};

/*
#filter Enable Javascript `()=>{}` syntax for C++ lambda
Before:
```C++
std::sort(a.begin(), a.end(), (int x, int y)=>{return x<y;});
```
*/
jsism.EnableJSLambdaSyntax = function(nd_root) {
	//()=>{} <=> [&](){}
	for (let nd_func of nd_root.FindAll(N_FUNCTION, null)) {
		let nd_before = nd_func.c;
		let nd_after = nd_func.c.s.s;
		if (nd_after.isSymbol('=>')) {
			nd_after.ReplaceWith(nAir()).setCommentsBefore('').setCommentsAfter('');
			nd_before.ReplaceWith(@([&]));
		}
	}
};

jsism.EnableJSLambdaSyntax.inverse = function(nd_root) {
	//()=>{} <=> [&](){}
	let nd_template = @([&]);
	for (let nd_func of nd_root.FindAll(N_FUNCTION, null)) {
		let nd_before = nd_func.c;
		let nd_after = nd_func.c.s.s;
		if (nd_before.Match(nd_template)) {
			nd_after.ReplaceWith(nSymbol('=>')).setCommentsBefore(' ');
			nd_before.ReplaceWith(nAir());
		}
	}
};

/*
#filter Enable single-quoted strings for C/C++
Do not use this filter if you need multi-char constants.

Before:
```C++
puts('hello world');
```
*/
jsism.EnableSingleQuotedStrings = function(nd_root) {
	for (let nd_str of nd_root.FindAll(N_STRING)) {
		let bk_flags = nd_str.flags;
		let bk_data = nd_str.data;
		if (nd_str.data.startsWith("'") && nd_str.GetStringValue().length > 1) {
			nd_str.flags &= ~STRING_SINGLE_QUOTED;
		} else {
			//avoid unescaping / re-escaping strings unnecessarily
			nd_str.data = bk_data;
			nd_str.flags = bk_flags;
		}
	}
};
