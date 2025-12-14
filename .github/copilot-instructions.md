# life-lang Compiler - Development Guide

Prescriptive guide for working on this C++20 compiler.  
**Check existing code patterns first - they're the source of truth.**

## Project Overview

**Current Status**: Hand-written recursive descent parser complete, AST definitions complete  
**Next Phase**: Implementing semantic analysis (type checking, name resolution)

### Build Presets
- `dev`: Fast iteration, clang-tidy disabled
- `debug`: Full checks enabled, slower builds

### Development Philosophy: Lightweight Solutions First

**Principle**: Prioritize lightweight implementations to minimize compile time and clang-tidy overhead.

**Examples of this principle in action:**
- ✅ **Hand-written parser** over Boost.Spirit: Reduced clang-tidy time from 20m → 3m
- ✅ **doctest** over Catch2: Lighter-weight test framework, faster compilation
- ✅ **Minimal template metaprogramming**: Avoid heavy template instantiation
- ✅ **Direct implementations** over library abstractions when feasible

**When evaluating solutions:**
1. Check compile-time cost (does it slow down iteration?)
2. Check clang-tidy impact (does it add analysis overhead?)
3. Prefer simple, direct code over clever abstractions
4. Libraries should justify their weight with clear benefits

**Red flags:**
- Heavy template libraries (Spirit, Boost.Hana, etc.)
- Complex macro systems
- Header-only libraries with deep instantiation
- Test frameworks with heavy compile-time overhead

This project values **fast iteration** - every second saved in compile/analyze time compounds across development.

### Core Components
- **src/parser.cpp/hpp**: Hand-written recursive descent parser
- **src/ast.hpp**: AST node definitions with JSON serialization
- **src/diagnostics.cpp**: Error reporting infrastructure (clang-style diagnostics)
- **tests/**: Test infrastructure using doctest (lightweight test framework)
- **GRAMMAR.md**: Formal EBNF grammar specification (authoritative source of truth)

### Parser/Grammar Synchronization
⚠️ **CRITICAL**: Parser implementation must match GRAMMAR.md exactly

**When modifying the parser:**
1. Check GRAMMAR.md first - it defines what's valid
2. Update parser.cpp to implement the grammar rules
3. Update GRAMMAR.md if adding/changing language features
4. Add tests in tests/parser/ for the grammar rule
5. Add integration tests in tests/integration/ for complete workflows

**When modifying the grammar:**
1. Update GRAMMAR.md with new/changed EBNF rules
2. Update parser.cpp parse_* methods to match
3. Update ast.hpp if new AST nodes are needed
4. Add corresponding tests

**Key enforcement points:**
- `parse_module()`: Only accepts items (fn, struct, enum, impl, trait, type) - NOT arbitrary statements
- Module-level validation happens in parse_module(), checking for valid item keywords
- Grammar rules define precedence, associativity, and syntax - parser must not be more permissive

---

## Language Design

### Type System
- **Value semantics**: Immutable by default
- **Primitives**: `I32`, `I64`, `F32`, `F64`, `Bool`, `Char`, `String`, etc.
- **Unit type**: `()` - represents "no value"
- **Structs**: Named fields, public by default
  ```rust
  struct Point { x: I32, y: I32 }
  let p = Point { x: 1, y: 2 };
  ```
- **Enums**: Sum types with variants
  ```rust
  enum Option<T> { Some(T), None }
  ```
- **Tuples**: Ordered product types `(I32, String, Bool)`

### Functions and Methods
```rust
// Free function
fn add(x: I32, y: I32): I32 { return x + y; }

// Methods in impl block
impl Point {
  fn distance(self): F64 { /* ... */ }        // self type inferred
  fn translate(mut self, dx: I32): Point { /* ... */ }  // mutation (future)
}

// Call syntax
add(1, 2)           // function call
point.distance()    // method call (dot notation)
```
- `self` parameter **only** in impl blocks (enforced semantically)
- No implicit `this` - always explicit `self`

### Control Flow
```rust
// If expression (else required for expression form)
let max = if x > y { x } else { y };

// If statement (else optional)
if condition { do_something(); }

// Match expression
match value {
  Pattern1 => expr1,
  Pattern2 => expr2,
}

// For loop
for item in collection { /* ... */ }
for (key, value) in map { /* ... */ }
```
- **No parentheses** around conditions (braces provide boundaries)
- **Block trailing expressions**: Last expression without semicolon is block value

### Naming Conventions
| Element | Convention | Example |
|---------|-----------|---------|
| Variables, functions, fields | `snake_case` | `my_var`, `calculate_sum` |
| Types, structs, modules | `Camel_Snake_Case` | `Point`, `User_Profile` |
| Constants | Use functions | `fn max_size(): I32 { return 100; }` |

**Note**: See `GRAMMAR.md` for the complete EBNF grammar specification of the life-lang language.

---

## C++ Implementation Patterns

### AST Node Structure
Every AST node:
1. Has `static constexpr std::string_view k_name` (for error messages)
2. Uses `make_<node>()` helper for construction
3. Can be printed to S-expression format via `to_sexp_string()` for debugging

```cpp
// In ast.hpp
struct My_Node {
  static constexpr std::string_view k_name = "my_node";
  std::string value;
};

inline auto make_my_node(std::string value_) -> My_Node {
  return My_Node{std::move(value_)};
}

// In sexp.hpp - S-expression printing handled by visitor
// Output: (my_node "value")
```

### Variants (Sum Types)
```cpp
struct Expr : std::variant<Integer, String, Binary_Expr> {
  using Base_Type = std::variant<Integer, String, Binary_Expr>;
  using Base_Type::Base_Type;      // Inherit constructors
  using Base_Type::operator=;       // Inherit assignment
  static constexpr std::string_view k_name = "expr";
};
```

### Recursive Types
Use `std::shared_ptr<T>` for recursive types:
```cpp
struct Binary_Expr {
  std::shared_ptr<Expr> left;
  Operator op;
  std::shared_ptr<Expr> right;
};
```

### Debug Output
Use S-expression format for debugging (compact Lisp-style syntax):
```cpp
auto const module = parser.parse_module();
// Indented output (2 spaces, readable):
std::cout << to_sexp_string(*module, 2) << '\n';
// Compact output (no indentation, for tests):
std::cout << to_sexp_string(*module, 0) << '\n';
```

**Output examples:**
```lisp
// Indented (indent=2, default for human reading)
(func_def
  (func_decl
    "main"
    ()
    ()
    (path
      ((type_segment
        "I32"))))
  (block
    ((return
      (integer
        "42")))))

// Compact (indent=0, for tests and logs)
(func_def (func_decl "main" () () (path ((type_segment "I32")))) (block ((return (integer "42")))))
```

**Rationale**: S-expressions are more compact and readable than JSON, with no heavyweight dependencies.

**Grammar**: See `SEXP_GRAMMAR.md` for complete S-expression syntax specification.

---

## Naming Conventions

### C++ Code (Enforced by `.clang-tidy`)
- **Types**: `Camel_Snake_Case` - `Path_Segment`, `Iterator_Type`
- **Functions/variables**: `lower_case` - `parse_expr`, `token_count`
- **Parameters**: `_` suffix - `begin_`, `value_`
- **Private members**: `m_` prefix - `m_data`, `m_count`
- **Global constants**: `k_` prefix - `k_kw_fn`, `k_max_depth`
- **Test files**: `test_<node>.cpp` - `test_binary_expr.cpp`

### JSON Output
All `to_json()` functions use **snake_case** keys:
```cpp
// ✅ CORRECT
a_json = {
  {"template_parameters", node.template_parameters},
  {"return_type", node.return_type}
};

// ❌ WRONG
a_json = {
  {"templateParameters", node.template_parameters},  // camelCase - wrong!
  {"returnType", node.return_type}
};
```

---

## Common Pitfalls

| Issue | Symptom | Fix |
|-------|---------|-----|
| Using `std::unique_ptr` for recursive types | Move semantics issues | Use `std::shared_ptr` instead |
| Heavyweight dependencies | Slow compile/clang-tidy times | Prefer direct implementations, avoid template-heavy libraries |

---

## Next Phase: Semantic Analysis

Parser is complete. Next steps:
1. **Symbol table**: Track declarations (functions, types, variables)
2. **Name resolution**: Resolve identifiers to declarations
3. **Type checking**: Verify type correctness
4. **Trait resolution**: Check trait bounds, impl blocks
5. **Borrow checking**: Validate lifetime rules (future)

Key considerations:
- Preserve position information for error reporting
- Build on existing JSON serialization for IR output
- Maintain value semantics throughout
