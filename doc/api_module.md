# Javascript Module Reference

Amalang uses the CommonJS module system. To load a module, use `require('module_path')`.

## Modules

Here is a list of modules for ama script development.

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

### auto_paren

`const auto_paren = require("auto_paren");`

DO NOT use ama features: this is a "chicken" file which gets called when formatting other JS files

### auto_semicolon

`const auto_semicolon = require("auto_semicolon");`

DO NOT use ama features: this is a "chicken" file which gets called when formatting other JS files

### _init

`const _init = require("_init");`

this module is automatically executed by ama::InitScriptEnv() the `require` system isn't ready here so don't use it @ama ParseCurrentFile().Save()

### omnichecker

`const omnichecker = require("omnichecker");`

Generic checker for module invariance note: this module is currently experimental

### pipe

`const pipe = require("pipe");`

Run external commands

Provided methods:

- `pipe.run(command)`

### _sandbox

`const _sandbox = require("_sandbox");`

this module is automatically executed by ama::JSRunInSandbox at most once it's in a sandbox and does not have access to ANY native function @ama ParseCurrentFile().Save() convention: sandbox objects should be dumb, the shouldn't have methods

### fs

`const fs = require("fs");`

Node.js-compatible file system module.

Provided methods:

- `fs.readFileSync(path)`
- `fs.existsSync(path)`
- `fs.writeFileSync(path, data)`
- `fs.statSync(path)`
- `fs.readdirSync(dir,{withFileTypes:true})`

### bisync

`const bisync = require("bisync");`

the bidirectional synchronization workflow

### fsext

`const fsext = require("fsext");`

Extra file system utility.

Provided methods:

- `fsext.FindAllFiles(dir)`: recursively find files in dir and return an array of absolute paths
- `fsext.SyncTimestamp(fn_src, fn_tar)`: make the timestamps of `fn_src` and `fn_tar` identical


## Filters

Here is a list of filters intended for the `features` section of `bisync` options.

### cpp/cpp_indent

`require("cpp/cpp_indent")` Indent-based scoping for C++.

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

### cmake.AutoCreate

`require("cmake").AutoCreate` Create cmake build files for C/C++ source.

This filter only applies to files containing a main function. It will create a build target for that file and a wrapping `CMakeLists.txt` if it can't find one. If there is already a build target, it will search for dependent files and update the source file list when necessary. Non-include dependency can be added with `#pragma add("c_files","./foo.c")`.


