# Amalang

A meta-language to enable any feature in any language, then deliver idiosyncratic code in the original language.

The project itself seems written in C++11, but once you build it and run `script/sync.js`, you'll see the true dialect it uses.

## Quick Start 

### Unix

```sh
mkdir -p build
cd build
cmake ..
make
cd ..
ln -s "$(pwd)/modules" ~/.ama_modules
build/ama script/sync.js
```

### Windows

```bat
md build
cd build
cmake ..
cmake --build .
cd ..
xcopy /e /y modules %USERPROFILE%\.ama_modules
build\ama script\sync.js
```
