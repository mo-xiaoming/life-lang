# life-lang compiler

## How to build

Make sure you have vcpkg installed. You can follow the official setup guide [here](https://github.com/microsoft/vcpkg#quick-start).

```bash
# a debug build with asan and ubsan turned on
cmake --preset debug
# using `-j8` because of Makefile being used internally instead of Ninja,
# because Ninja/gcc14 adds -fmodules-ts -fmodule-mapper flags to command line
# and those flags make clang-tidy throw "unknown argument"
# and I don't know how to turn it off
cmake --build --preset default -j8

ctest --preset default

./build/lifec
```
