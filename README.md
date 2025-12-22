# life-lang compiler

[![CI Build and Test](https://github.com/mo-xiaoming/life-lang/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/mo-xiaoming/life-lang/actions/workflows/build-and-test.yml)
[![codecov](https://codecov.io/gh/mo-xiaoming/life-lang/branch/master/graph/badge.svg)](https://codecov.io/gh/mo-xiaoming/life-lang)

A programming language with value semantics, guided by three core principles:

## Design Philosophy

1. **Consistent behavior** across all build modes (debug, release, optimized)
   - Same code always produces same results
   - No hidden surprises between development and production
   - Test once, deploy with confidence

2. **Fail fast** on programmer errors
   - Overflow, underflow, division by zero → immediate panic
   - Detect bugs early rather than propagate incorrect values
   - Explicit alternatives available when needed (`checked_*`, `wrapping_*`, `saturating_*`)

3. **Predictable results**
   - No undefined behavior
   - No silent data corruption
   - Clear, documented semantics for all operations

These principles prioritize **correctness and user-friendliness over raw performance**. When maximum performance is needed, explicit opt-in mechanisms are provided.

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

## Development Setup

This project uses **[Devbox](https://www.jetify.com/devbox)** for reproducible development environments. All dependencies (GCC, Clang, CMake, Ninja, etc.) are managed automatically.

### Quick Start

1. **[Install Devbox](https://www.jetify.com/devbox/docs/installing_devbox/)** (one-time setup)
2. **Clone the repo:**
   ```bash
   git clone https://github.com/mo-xiaoming/life-lang.git
   cd life-lang
   ```

3. **Environment activation:**
   - **Option A: Auto-activation with direnv (Recommended)**
     - [Install direnv](https://direnv.net/docs/installation.html) (one-time)
   - **Option B: Manual activation**
     ```bash
     devbox shell  # Enter the dev environment manually
     ```

4. **VSCode setup** (optional):
   - Open the repo in VSCode
   - When prompted, click **"Install Recommended Extensions"**
   - Installs: clangd, CMake Tools, Devbox, direnv, Error Lens, and more
   - All settings (LSP, formatting, etc.) apply automatically
   - **With direnv extension:** VSCode terminal automatically uses devbox environment

4. **Build and test:**
   ```bash
   devbox run configure debug  # or: cmake --preset debug
   devbox run build debug      # or: cmake --build --preset debug
   devbox run test debug       # or: ctest --preset debug
   devbox run lint             # or: run-clang-tidy -p .
   ```

### Alternative: System Tools

Without Devbox, install manually:
- GCC 15+ or latest
- Clang 20+ or latest
- CMake 3.31+
- Ninja
- vcpkg

**Warning:** CI uses Devbox, so tool version mismatches may cause non-reproducible failures.

## How to build

```bash
# With Devbox (recommended)
devbox shell
devbox run configure && devbox run build && devbox run test

# Or use cmake directly (works with or without Devbox)
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
