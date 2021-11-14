# Amalang

A meta-language to enable any feature in any language, and keep interacting with upstream in the original language.

The project itself is an example. You can build this repository in standard C++11, but it's actually developed in a different dialect. Try building it and run `script/sync.js`, then check `*.ama.cpp`.

The core is a generic parser and AST manipulation APIs, exposed through a simple Javascript interface.

## How to Use

### Build the Project

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

TODO: test_jsism, test_borrow0

### Configure an Existing Project

TODO: point to examples

### API Reference

## Contributing

TODO: ama.cpp features: indent, no (), array types
