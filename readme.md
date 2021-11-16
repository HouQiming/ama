# Amalang

A system to amalgamate any features you want into any language you need, while keep interacting with upstream in the original language.

The project itself is an example. You can build this repository in plain C++11, but it's actually developed in a different dialect. Try building it and run `script/sync.js`, then check the generated `*.ama.cpp`.

Key features:
- **Zero setup** The system can parse any code as is, even if it's unbuildable or incomplete. It will parse whatever it understands and let unrecognized syntax pass through unchanged.
- **Zero disruption** Amalang lets you develop in a different language or workflow than you deliver. You can start or quit at any time you choose in any project you need, without disrupting upstream at all.
- **Simple API** Most work only involves simple Javascript manipulating objects of one class `Node`. No need to learn special programming patterns or compiler theory.

Works best on languages with a reasonable resemblance to C. And that includes Python. See `example/` to see Amalang in action.

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

See `example/hello_world.cpp` for a tutorial.

### Configure an Existing Project

On an existing project, the recommended way is to set up a bidirectional synchronization script like `script/sync.js`. The script should update in-repository source files when you change the custom-dialect files, and synchronize back any pulled upstream change. The `bisync` module can help with that.

Please refer to `script/sync.js` and the relevant module code for details.

### Performance Critical Applications

Performance critical applications could use the internal C++ API directly and forgo the Javascript interpreter. For that purpose, you can link with `libamal.so` (`amal.dll` on Windows) and `#include "<Amalang project directory>/src/script/jsapi.hpp"` in your C++ project.

Alternatively, you can create a native module and call that from Javascript. See`example/native_module` for an example. The native module still needs to link `libamal.so` and include `jsapi.hpp`.

## Documentation

[AST API Reference](doc/api_node.md)

TODO: module reference, talk about CommonJS in module reference

## Contributing

We welcome pull requests. We need your help to build up an ecosystem.

See the [document for contributors](doc/contrib.md).

## Credit

Amalang is powered by [QuickJS](https://bellard.org/quickjs/).
