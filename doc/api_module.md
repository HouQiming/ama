# Javascript Module Reference

Amalang uses the CommonJS module system. To load a module, use `require('module_path')`.

## Modules

Here is a list of modules for ama script development.

### bisync

`const bisync = require("bisync");`

Bidirectional synchronization system. Usage with default option values:

```Javascript
require('bisync')({
    dir_src: path.resolve(__dirname, '../src'),
    middle_extension: '.ama',
    processed_extensions: ['.cpp','.hpp','.cu'],
    filters: [],
})
```

The module will search for all files with `processed_extensions` in `dir_src`. It will apply filters specified in `filters` to files with `middle_extension` (e.g. `foo.ama.cpp`) to generate the corresponding non-ama file (e.g. `foo.cpp`). When available, it will also apply the inverse version of the filters to generate ama files from non-ama files. Between each pair of ama and non-ama files, `bisync` will always synchronize the the newer file's content to its older counterpart.

See [the filters section](#-filters) for possible filters.

### fs

`const fs = require("fs");`

Node.js-compatible file system module.

Provided methods:

- `fs.readFileSync(path)`
- `fs.existsSync(path)`
- `fs.writeFileSync(path, data)`
- `fs.statSync(path)`
- `fs.readdirSync(dir,{withFileTypes:true})`

### fsext

`const fsext = require("fsext");`

Extra file system utility.

Provided methods:

- `fsext.FindAllFiles(dir)`: recursively find files in dir and return an array of absolute paths
- `fsext.SyncTimestamp(fn_src, fn_tar)`: make the timestamps of `fn_src` and `fn_tar` identical

### omnichecker

`const omnichecker = require("omnichecker");`

Generic checker for module invariance.

Note: this module is currently experimental.

### path

`const path = require("path");`

Node.js-compatible path handling module.

Provided methods:

- `path.join(...)`
- `path.resolve(...)`
- `path.parse(path)`
- `path.dirname(path)`
- `path.basename(path)`
- `path.extname(path)`
- `path.isAbsolute(path)`
- `path.relative(path_a, path_b)`

### pipe

`const pipe = require("pipe");`

Run external commands.

Provided methods:

- `pipe.run(command)`


## Filters

Here is a list of filters intended for the `filters` option of `bisync`. Each filter can be:

- A string indicating a built-in filter, like `"Save"`
- A `function(nd_root, options)` that operates on the AST `nd_root`
- An object with option changes

### Save

`"Save"` Save the file, including changes from previous filters.

Put an option object `{change_ext:'.foo'}` before `"Save"` to save the changes under a different extension.

### StripRedundantPrefixSpace

`"StripRedundantPrefixSpace"` Strip redundant spaces.

### auto_paren

- Syntax: `require("auto_paren")`
- Description: Enable Python-style `if foo:` / `for foo:` / ... in C / C++ / Javascript.

This filter will add '()' and `{}` automatically. Before:

```C++
int main() {
	if rand()&1:{
		puts("rand()&1");
	}
	for int i=0;i<10;i++
		printf("iteration %d\n",i);
	return 0;
}
```

After:

```C++
int main() {
	if (rand() & 1) {
		puts("rand()&1");
	}
	for (int i = 0;i < 10;i++) { 
		printf("iteration %d\n", i);
	}
	return 0;
}
```

### auto_semicolon

- Syntax: `require("auto_semicolon")`
- Description: Automatically add ';' for C / C++ / Javascript.

Before:

```C++
int main() {
	puts("hello world")
	return 0
}
```

After:

```C++
int main() {
	puts("hello world");
	return 0;
}
```

### cmake.AutoCreate

- Syntax: `require("cmake").AutoCreate`
- Description: Create cmake build files for C/C++ source

This filter only applies to files containing a main function. It will create a build target for that file and a wrapping `CMakeLists.txt` if it can't find one. If there is already a build target, it will search for dependent files and update the source file list when necessary. Non-include dependency can be added with `#pragma add("c_files","./foo.c")`.

### cpp/asset

- Syntax: `require("cpp/asset")`
- Description: Pull in an external file to a zero-terminated C constant array.

Before:

```C++
#pragma gen(asset('./_doc_asset.txt'))
```

After:

```C++
#pragma gen_begin(asset('./_doc_asset.txt'))
static const uint8_t _doc_asset[]={
	72,101,108,108,111,32,119,111,114,108,100,33,0
};
#pragma gen_end(asset('./_doc_asset.txt'))
```

### cpp/auto_decl

- Syntax: `require("cpp/auto_decl")`
- Description: Automatically declare variables on assignment

Before:

```C++
int main(int argc){
	a=42;
	if(argc>=2){
		b=0;
		a=b;
	}else{
		a+=100;
	}
	return a;
}
```

After:

```C++
int main(int argc) {
	auto a = 42;
	if (argc >= 2) {
		auto b = 0;
		a = b;
	} else {
		a += 100;
	}
	return a;
}
```

### cpp/auto_dot

- Syntax: `require("cpp/auto_dot")`
- Description: Automatically deduce `->` or `::` from `.`

Before:

```C++
namespace ama{
	struct Node{...};
}
ama.Node* GetChild(ama.Node* nd){
	return nd.c;
}
```

After:

```C++
namespace ama {
	struct Node {...};
}
ama::Node* GetChild(ama::Node* nd) {
	return nd->c;
}
```

### cpp/auto_header

- Syntax: `require("cpp/auto_header")`
- Description: Synchronize methods and functions to classes and headers

If you `#include "./foo.hpp"` in `foo.cpp`, this filter will also add function forward declarations to `foo.hpp`. Use `#pragma no_auto_header()` to suppress this behavior.

Before:

```C++
struct TestClass{
	int a;
};
int TestClass::get_a(){
	return this->a;
}
void TestClass::set_a(int a){
	this->a = a;
}
```

After:

```C++
struct TestClass {
	int a;
	int get_a();
	void set_a(int a);
};
int TestClass::get_a() {
	return this->a;
}
void TestClass::set_a(int a) {
	this->a = a;
}
```

### cpp/cpp_indent

- Syntax: `require("cpp/cpp_indent")`
- Description: Indent-based scoping for C / C++ / Javascript

Before:

```C++
int main()
	puts("hello world");
	return 0;
```

After:

```C++
int main() {
	puts("hello world");
	return 0;
}
```

### cpp/gentag.GeneratedCode

- Syntax: `require("cpp/gentag").GeneratedCode`
- Description: Generic inverse filter for things like `jsism.EnableJSON`

This filter replaces code inside:

```C++
#pragma gen_begin(foo)
#pragma gen_end(foo)
```

with

```C++
#pragma gen(foo)
```

We recommend custom filters that generate C++ to follow this convention to put generated code inside `gen_begin` and `gen_end`. That allows the generated code to be cleared when synchronized back to `.ama.cpp` files.

### cpp/jsism.EnableConsole

- Syntax: `require("cpp/jsism").EnableConsole`
- Description: Translate `console.log` to `std::cout << foo`

The filter also supports some Javascript formatting methods like `toFixed` and `padStart`. Before:

```C++
int main(){
	console.log("hello world",(0.25).toFixed(3));
	return 0;
}
```

After:

```C++
#include <iostream>
#include <iomanip>
#include <ios>
int main() {
	std::cout << ("hello world") << " " << std::fixed << std::setprecision(3) << (0.25) << std::defaultfloat << std::endl;
	return 0;
}
```

### cpp/jsism.EnableJSON

- Syntax: `require("cpp/jsism").EnableJSON`
- Description: Enable `JSON.stringify` and `JSON.parse<T>` in C++

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

in a file with this filter enabled. The pragmas will be translated to stringify / parse implementation. If the class is in the same file as the `JSON.foo`, the pragma can be omitten.

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

After:

```C++
#include <iostream>
#include "<.ama_modules dir>/modules/cpp/json/json.h"

struct Test {
	int a;
};
#pragma gen_begin(JSON.stringify<Test>)
namespace JSON {
	template <>
	struct StringifyToImpl<Test> {
		typedef void type;
		template <typename T = Test>
		static void stringifyTo(std::string& buf, Test const& a) {
			buf.push_back('{');
			buf.append("\"a\":");
			JSON::stringifyTo(buf, a.a);
			buf.push_back('}');
		}
	};
}
#pragma gen_end(JSON.stringify<Test>)
int main() {
	Test obj{3};
	std::cout << JSON::stringify(obj) << std::endl;
	return 0;
}
```

### cpp/jsism.EnableJSLambdaSyntax

- Syntax: `require("cpp/jsism").EnableJSLambdaSyntax`
- Description: Enable Javascript `()=>{}` syntax for C++ lambda

Before:

```C++
std::sort(a.begin(), a.end(), (int x, int y)=>{return x<y;});
```

After:

```C++
std::sort(a.begin(), a.end(), [&](int x, int y) {return x < y;});
```

### cpp/jsism.EnableSingleQuotedStrings

- Syntax: `require("cpp/jsism").EnableSingleQuotedStrings`
- Description: Enable single-quoted strings for C/C++

Do not use this filter if you need multi-char constants.

Before:

```C++
puts('hello world');
```

After:

```C++
puts("hello world");
```

### cpp/line_sync

- Syntax: `require("cpp/line_sync")`
- Description: Allow using `#line __AMA_LINE__` to synchronize object file line numbers to `foo.ama.cpp`

The filter simply replaces `__AMA_LINE__` with actual line numbers. Before:

```C++
#pragma gen(some_generated_code)
#line __AMA_LINE__
int main(){
	return 0;
}
```

After:

```C++
#pragma gen(some_generated_code)
#line 3
int main() {
	return 0;
}
```

### cpp/move_operator

- Syntax: `require("cpp/move_operator")`
- Description: Use prefix `<<` for `std::move`

Before:

```C++
std::string MakeSomeString(){
	std::string ret;
	...
	return <<ret;
}
```

After:

```C++
std::string MakeSomeString() {
	std::string ret;
	...
	return std::move(ret);
}
```

### cpp/sane_export

- Syntax: `require("cpp/sane_export")`
- Description: Default global functions to `static` unless specified as `public`

Before:

```C++
int square(int x){return x*x;}
public int main(int argc){
	return square(argc);
}
```

After:

```C++
static int square(int x) {return x * x;}
int main(int argc) {
	return square(argc);
}
```

### cpp/sane_for

- Syntax: `require("cpp/sane_for")`
- Description: Extend C++ `for` syntax with `for(foo of bar)` and `for(i<foo)`

Before:

```C++
int main() {
	std::vector<int> a;
	for(int i<10){
		a.push_back(i);
	}
	for(int i in a){
		std::cout<<i;
	}
	return 0;
}
```

After:

```C++
int main() {
	std::vector<int> a;
	for (int i = 0; i < 10; i++) {
		a.push_back(i);
	}
	for (int i : a) {
		std::cout << i;
	}
	return 0;
}
```

### cpp/sane_init

- Syntax: `require("cpp/sane_init")`
- Description: Zero-initialize otherwise-uninitialized C++ variables

Before:

```C++
struct CFoo{
	int m_foo;
};
int g_foo;
int main(){
	int foo;
	return 0;
}
```

After:

```C++
struct CFoo {
	int m_foo{};
};
int g_foo{};
int main() {
	int foo{};
	return 0;
}
```

### cpp/sane_types.FixArrayTypes

- Syntax: `require("cpp/sane_types").FixArrayTypes`
- Description: Mark `[]` as a type suffix, a required setup step for `require("sane_types")`

The filter itself has no visible effect and must be used before `require("sane_types")`.

### cpp/sane_types

- Syntax: `require("cpp/sane_types")`
- Description: Short names for C++ template types

Correspondence:

- `foo[]` => `std::vector<foo>`
- `foo[:]` => `std::span<foo>`
- `foo[size]!` => `std::array<foo, size>`
- `Map<foo, bar>` => `std::unordered_map<foo, bar>`

Before:

```C++
int main() {
	int[] a;
	a.push_back(10);
	return 0;
}
```

After:

```C++
int main() {
	std::vector<int> a;
	a.push_back(10);
	return 0;
}
```

### cpp/short_types

- Syntax: `require("cpp/short_types")`
- Description: Short numerical type names for C++

Before:

```C++
f32 powi(f32 a, u32 p){
	return p == 0 ? 1.f : (p & 1 ? a : 1.f) * powi(a, p >> 1);
}
iptr addr_powi = (iptr)(void*)powi;
```

After:

```C++
float powi(float a, uint32_t p) {
	return p == 0 ? 1.f : (p & 1 ? a : 1.f) * powi(a, p >> 1);
}
intptr_t addr_powi = (intptr_t)(void*)powi;
```

### cpp/typing.DeduceAuto

- Syntax: `require("cpp/typing").DeduceAuto`
- Description: Replace C++ `auto` with the deduced type whenever possible

This filter is intended for source feedback, i.e., save the deduction result to a file later. The deduction is backed by ama's simple typing engine so the result may not be available or correct.

### cpp/unified_null

- Syntax: `require("cpp/unified_null")`
- Description: Use `NULL` for `nullptr`

Before:

```C++
void* g_ptr = NULL;
```

After:

```C++
void* g_ptr = nullptr;
```


