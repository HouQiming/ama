# Amalang

A meta-language to customize any language for development, while keep interacting with upstream in the original language.

The project itself is an example. You can build this repository in plain C++11, but it's actually developed in a different dialect. Try building it and run `script/sync.js`, then check `*.ama.cpp`.

The core is a generic parser and a set of AST manipulation APIs, exposed through a simple Javascript interface.

## How to Use

### Build

#### Unix

```sh
mkdir -p build
cd build
cmake ..
make
cd ..
ln -s "$(pwd)/modules" ~/.ama_modules
build/ama script/sync.js
```

#### Windows

```bat
md build
cd build
cmake ..
cmake --build .
cd ..
xcopy /e /y modules %USERPROFILE%\.ama_modules
build\ama script\sync.js
```

### Start with a Single File

To quickly try out the features, add a comment with `@ama` and some Javascript to a source code file, then pass that file to `ama`.

See `example/hello_world.cpp` for a tutorial example.

### Configure an Existing Project

To customize development of an existing project, the recommended way is to set up a bidirectional synchronization script like `script/sync.js`. The script should update in-repository source files when you change the custom-dialect files, and synchronize back any pulled upstream change. The `bisync` module can help with that.

Please refer to `script/sync.js` and the relevant module code for details.

## Documentation

TODO: custom markdown generation

TODO: ParseCurrentFile, ParseCode, Node, node-like modules

## Contributing

TODO: ama.cpp features: indent, no (), array types

TODO: rules: we never stop programmers from building the original code, as idiosyncratic as possible, try to avoid recursion
