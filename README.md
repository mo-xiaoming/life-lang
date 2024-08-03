# life-lang compiler

## How to build

Make sure you have vcpkg installed. You can follow the official setup guide [here](https://github.com/microsoft/vcpkg#quick-start).

```bash
cmake --preset vcpkg # a debug build with asan and ubsan turned on
cmake --build build
./build/lifec
```
