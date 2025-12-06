# life-lang compiler

[![CI Build and Test](https://github.com/mo-xiaoming/life-lang/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/mo-xiaoming/life-lang/actions/workflows/build-and-test.yml)
[![codecov](https://codecov.io/gh/mo-xiaoming/life-lang/branch/master/graph/badge.svg)](https://codecov.io/gh/mo-xiaoming/life-lang)

## How to build

Make sure you have vcpkg installed. You can follow the official setup guide [here](https://github.com/microsoft/vcpkg#quick-start).

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug

# the language
echo 'fn main(args: Std.Array<Std.String>): I32 {
    Std.print("Hello, world!");
    return 0;
}' | ./build/dev/debug/lifec -

# clang style error message
echo 'fn main(): I32 {
    return 0;
}

fn broken_syntax_here
}' | ./build/dev/debug/lifec -
<stdin>:5:1: error: Failed to parse module: Expecting: '(' here:
    fn broken_syntax_here
    ^
```
