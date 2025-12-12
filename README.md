# life-lang compiler

[![CI Build and Test](https://github.com/mo-xiaoming/life-lang/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/mo-xiaoming/life-lang/actions/workflows/build-and-test.yml)
[![codecov](https://codecov.io/gh/mo-xiaoming/life-lang/branch/master/graph/badge.svg)](https://codecov.io/gh/mo-xiaoming/life-lang)

A (yet another) programming language with value semantics, (maybe) Rust-style traits.

## Language Overview

**Design Principles**:
- **Value semantics by default**: Immutability without explicit keywords, modifications return new values
- **Expression-oriented**: If, match, and blocks are expressions that return values
- **Explicit and safe**: No implicit conversions, no null pointers, clear(?) error handling

**Key Features**:
- ✅ Structs with named fields
- ✅ Enums with unit, tuple, and struct variants
- ✅ Pattern matching (match expressions, let bindings)
- ✅ Trait system (declarations, implementations, bounds, where clauses)
- ✅ Generics with trait bounds
- ✅ Impl blocks for methods
- ✅ Control flow expressions (if, while, for)
- ⏳ Type inference (pending semantic analysis)
- ⏳ Module system (pending)

## Example Code

```rust
// Generic Result type with struct variants
enum Result<T, E> {
    Ok(T),
    Err(E),
}

// Trait with generic bounds and where clause
trait Processor<T> 
where 
    T: Display + Clone
{
    fn process(self, item: T): Result<T, String>;
}

// Struct with impl block
struct Point {
    x: I32,
    y: I32,
}

impl Point {
    fn distance(self): F64 {
        let dx = self.x * self.x;
        let dy = self.y * self.y;
        return Std.Math.sqrt((dx + dy).into());
    }
}

// Trait implementation
impl Display for Point {
    fn to_string(self): String {
        return Std.Format.format("({}, {})", self.x, self.y);
    }
}

// Pattern matching with if-expression
fn process_result<T: Display>(result: Result<T, String>): I32 {
    return match result {
        Result.Ok(value) => {
            Std.IO.println("Success: " + value.to_string());
            0
        },
        Result.Err(msg) => {
            Std.IO.println("Error: " + msg);
            1
        },
    };
}

fn main(args: Array<String>): I32 {
    let point = Point { x: 3, y: 4 };
    let dist = point.distance();
    
    let result = if dist > 5.0 {
        Result.Ok(point)
    } else {
        Result.Err("Too close")
    };
    
    return process_result(result);
}
```

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
