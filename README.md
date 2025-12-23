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

This project uses **[Nix flakes](https://nixos.org/manual/nix/stable/)** for reproducible development environments. All dependencies (GCC, Clang, CMake, Ninja, etc.) are managed automatically with exact version pinning.

### Automated Setup (Recommended)

Run the setup script to install Nix, direnv, and all dependencies:

```bash
git clone https://github.com/mo-xiaoming/life-lang.git
cd life-lang
./scripts/setup-dev-environment.sh
```

The script will:
- ✅ Install Nix package manager with flakes support
- ✅ Install direnv + nix-direnv for automatic environment activation
- ✅ Download and build all development tools (~760 MB, cached)
- ✅ Configure shell integration
- ✅ Install VSCode extensions (optional)

**After installation:**
- Restart your shell: `source ~/.bashrc` (or `~/.zshrc`)
- Allow direnv: `direnv allow` (if using direnv)
- Start coding! Environment auto-activates on `cd`

### Manual Setup

If you prefer manual installation:

1. **Install Nix with flakes:**
   ```bash
   curl --proto '=https' --tlsv1.2 -sSf -L https://install.determinate.systems/nix | sh -s -- install
   ```

2. **Clone and enter environment:**
   ```bash
   git clone https://github.com/mo-xiaoming/life-lang.git
   cd life-lang
   nix develop  # Enter development shell
   ```

3. **Optional: Install direnv + nix-direnv for auto-activation:**
   ```bash
   # Install direnv and nix-direnv
   nix profile install nixpkgs#direnv nixpkgs#nix-direnv

   # Configure direnv shell hook
   echo 'eval "$(direnv hook bash)"' >> ~/.bashrc  # or ~/.zshrc

   # Configure nix-direnv integration
   mkdir -p ~/.config/direnv
   echo 'source $HOME/.nix-profile/share/nix-direnv/direnvrc' >> ~/.config/direnv/direnvrc

   # Allow direnv for this project
   direnv allow
   ```

### VSCode Setup

1. Open the repo in VSCode
2. Install recommended extensions when prompted
3. Includes: clangd, CMake Tools, Nix IDE, direnv, Error Lens, and more
4. All settings (LSP, formatting, etc.) apply automatically
5. Terminal automatically uses Nix environment (with direnv extension)

### Building

```bash
# Configuration and build
cmake --preset debug
cmake --build --preset debug
ctest --preset debug

# Helper scripts (in PATH automatically)
show-versions      # Display all tool versions
run-tidy          # Run clang-tidy on all files
generate-coverage # Generate code coverage report
```

### Tool Versions

All tools are pinned via `flake.lock` for reproducibility:
- GCC 15.2.0 (full package with gcov)
- Clang 21.1.2
- CMake 3.31.2
- See `show-versions` for complete list

**Note:** CI uses identical Nix environment - no version mismatches!

## How to build

```bash
# Inside Nix environment (or with direnv auto-activation)
cmake --preset debug
cmake --build --preset debug
ctest --preset debug

# the language
echo 'fn main(args: Std.Array<Std.String>): I32 {
    Std.print("Hello, world!");
    return 0;
}' | ./build/debug/src/lifec -

# clang style error message
echo 'fn main(): I32 {
    return 0;
}

fn broken_syntax_here
}' | ./build/debug/src/lifec -
<stdin>:5:1: error: Failed to parse module: Expecting: '(' here:
    fn broken_syntax_here
    ^
```
