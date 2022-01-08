# Amalang

A system to amalgamate features from other languages into any code base you work on, and you can keep interacting with upstream in the original language.

For example, we could write a `fizzbuzz.cpp` like this:

```Python
int main()
  for i<10
    if i%3==0&&i%5==0
      console.log("FizzBuzz");
    else if i%3==0
      console.log("Fizz");
    else if i%5==0
      console.log("Buzz");
    else
      console.log(i);
  return 0
```

And run:
```sh
ama -f cpp/cpp_indent -f auto_paren \
    -f cpp/sane_for -f cpp/jsism.EnableConsole \
    fizzbuzz.cpp
```

To get:
```C++
#include <iostream>
int main() {
  for (auto i = 0; i < 10; i++) {
    if (i % 3 == 0 && i % 5 == 0) {
      std::cout << ("FizzBuzz") << std::endl;
    } else if (i % 3 == 0) {
      std::cout << ("Fizz") << std::endl;
    } else if (i % 5 == 0) {
      std::cout << ("Buzz") << std::endl;
    } else {
      std::cout << (i) << std::endl;
    }
  }
  return 0
}
```

The project itself is a bigger example. You can build this repository with plain C++11, but it's actually developed in a different dialect. Try building it and run `script/sync.js`, then check the generated `*.ama.cpp`.

Key features:
- **Works on anything** The system can process any code as is. It will parse whatever it understands and let unrecognized syntax pass through unchanged.
- **Bidirectional** Many filters have an inverse version so you can translate upstream to your favorite dialect, develop there, then translate them back before committing.
- **Low effort** The core of Amalang is a very simple Javascript API. One can hook up a tiny DSL in hours then ditch it when there is no longer a need to keep up.

Works best on languages with a reasonable resemblance to C. That includes Python. See `example/` to see more of Amalang in action.

## How to Use

### Build

#### Unix

```sh
mkdir -p build
cd build
cmake ..
make
cd ..
build/ama script/sync.js
```

#### Windows

```bat
md build
cd build
cmake ..
cmake --build .
cd ..
build\ama script\sync.js
```

### Start with a Single File

To quickly try out the features, run `ama --help` to see a list of filters and use `ama -f <filter> <file>` to try one.

For bigger tasks, add some Javascript to your code under an `@ama` tag. See `example/hello_world.cpp` for a simple tutorial. See `example/cmake` for an automatic C++ build / run setup.

### Configure an Existing Project

On an existing project, the recommended way is to set up a bidirectional synchronization script like `script/sync.js`. The script should update in-repository source files when you change the custom-dialect files, and synchronize back any pulled upstream change.

This process can be automated by the `ama --init` command. The command automatically creates `script/sync.js` with a complete list of customization features, which can be enabled by uncommenting. You can then `ama --build` to build the project with cmake, or add `ama script/sync.js` as a pre-build command to your build system.

Currently the auto-created `sync.js` only customizes C++ development.

### Performance Critical Applications

Performance critical applications could use the internal C++ API directly and forgo the Javascript interpreter. For that purpose, you can link with `libamal.so` (`amal.dll` on Windows) and `#include "<Amalang project directory>/src/script/jsapi.hpp"` in your C++ project.

Alternatively, you can create a native module and call that from Javascript. See`example/native_module` for an example. The native module still needs to link `libamal.so` and include `jsapi.hpp`.

## Documentation

[AST API Reference](doc/api_node.md)

[Filter and Module Reference](doc/api_module.md)

## Contributing

We welcome pull requests. We need your help to build up an ecosystem.

See the [document for contributors](doc/contrib.md).

## Credit

Amalang is powered by [QuickJS](https://bellard.org/quickjs/).
