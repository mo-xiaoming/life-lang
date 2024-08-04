# life-lang compiler

## How to build

Make sure you have vcpkg installed. You can follow the official setup guide [here](https://github.com/microsoft/vcpkg#quick-start).

```bash
# a debug build with asan and ubsan turned on
cmake --preset debug
cmake --build --preset default -j8

ctest --preset default

./build/lifec
```
