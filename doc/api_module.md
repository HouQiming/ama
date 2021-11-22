# Javascript Module Reference

Amalang uses the CommonJS module system. To load a module, use `require('module_path')`.

## Modules

Here is a list of modules for ama script development.

### bisync

`const bisync = require("bisync");`

Bidirectional synchronization system:

Usage with default option values:

```Javascript
require('bisync')({
    dir_src: path.resolve(__dirname, '../src'),
    middle_extension: '.ama',
    processed_extensions: ['.cpp','.hpp','.cu'],
    features: [],
})
```

The module will search for all files with `processed_extensions` in `dir_src`. It will apply filters specified in `features` to files with `middle_extension` (e.g. `foo.ama.cpp`) to generate the corresponding non-ama file (e.g. `foo.cpp`). When available, it will also apply the inverse version of the filters to generate ama files from non-ama files. Between each pair of ama and non-ama files, `bisync` will always synchronize the the newer file's content to its older counterpart.

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

Here is a list of filters intended for the `features` option of `bisync`.

### auto_paren

`require("auto_paren")` Enable Python-style `if foo:` / `for foo:` / ... for C / C++ / Javascript..

This filter will add '()' and `{}` automatically.    Before:

```C++
int main() {
	for int i=0;i<10;i++
		printf("iteration %d\n",i);
	return 0;
}
```

After:

```C++
int main() {
	for (int i = 0;i < 10;i++) { 
		printf("iteration %d\n", i);
	}
	return 0;
}
```

### auto_semicolon

`require("auto_semicolon")` Automatically add ';' for C / C++ / Javascript..

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

`require("cmake").AutoCreate` Create cmake build files for C/C++ source.

This filter only applies to files containing a main function. It will create a build target for that file and a wrapping `CMakeLists.txt` if it can't find one. If there is already a build target, it will search for dependent files and update the source file list when necessary. Non-include dependency can be added with `#pragma add("c_files","./foo.c")`.

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


