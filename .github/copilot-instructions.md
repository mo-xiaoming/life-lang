# life-lang Compiler - Development Guide

Prescriptive guide for working on this C++20 compiler using Boost.Spirit X3.  
**Check existing code patterns first - they're the source of truth.**

## Project Overview

**Pipeline**: Source code → Parser (Spirit X3) → AST (position-tagged) → JSON output  
**Status**: Parser phase complete, all language features implemented  
**Next**: Semantic analysis (type checking, name resolution)

### Build Presets
- `dev`: Fast iteration, clang-tidy disabled
- `debug`: Full checks enabled, slower builds

### Core Components
- **src/ast.hpp**: AST node definitions (all inherit `x3::position_tagged`)
- **src/rules.cpp**: Spirit X3 parser definitions + **EBNF grammar documentation (lines 12-276)**
- **src/rules.hpp**: Parser function declarations
- **src/diagnostics.cpp**: Error reporting (clang-style diagnostics)
- **src/symbol_table.{hpp,cpp}**: Symbol table for semantic analysis
- **src/semantic_analyzer.{hpp,cpp}**: Semantic analysis (declaration collection, name resolution)
- **tests/parser/**: Comprehensive parser tests (57 test files, 1093 assertions)
- **tests/semantic/**: Semantic analysis tests

### ⚠️ CRITICAL: EBNF Grammar Synchronization
**MANDATORY WORKFLOW when modifying parser rules:**
1. **Update EBNF first** - Edit grammar documentation in `src/rules.cpp` (lines 12-276)
2. **Then modify parser** - Change the actual Spirit X3 rule implementation
3. **Verify alignment** - Ensure EBNF accurately reflects parser behavior
4. **Update tests** - Add/modify test cases to cover changes

**Why this matters**: The EBNF is the single source of truth for language syntax. Parser bugs are hard to catch - wrong EBNF leads to language design drift.

**Examples of what to update**:
- Adding/removing operators → Update expression hierarchy
- Changing pattern syntax → Update pattern rules
- Modifying statement semicolons → Update statement rules
- New control flow → Update expr/statement sections

---

## Language Design

### Type System
- **Value semantics**: Immutable by default
- **Primitives**: `I32`, `I64`, `F32`, `F64`, `Bool`, `Char`, `String`, etc.
- **Unit type**: `()` - zero-element tuple type
- **Structs**: Named fields, public by default
  ```rust
  struct Point { x: I32, y: I32 }
  let p = Point { x: 1, y: 2 };
  ```
- **Enums**: Sum types with variants
  ```rust
  enum Option<T> { Some(T), None }
  ```
- **Tuples**: Ordered product types `(I32, String, Bool)`, `(Vec<T>, Map<K, V>)`, `()` for unit
- **Function types**: `fn(I32, String): Bool`, `fn(): ()`, `fn(fn(I32): Bool): Bool`

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

---

## C++ Implementation Patterns

### Code Style
- **Return types**: Use standard syntax `RetType func()`, not trailing `auto func() -> RetType`
- **String conversion**: Use free functions `std::string to_string(Type const&)`, not member functions
- **Simple structs**: Prefer aggregate initialization, avoid unnecessary constructors
- **NOLINT Policy**: Do not add NOLINT suppressions without explicit user approval. Fix the underlying issue instead. Only use NOLINT for unavoidable cases:
  - Intentional design decisions (e.g., reference members by design)
  - False positives where the code is correct but clang-tidy misinterprets it
- **C-array warnings**: Suppressed globally in `.clang-tidy` - string literals naturally use C-arrays, no NOLINT needed

### AST Node Structure
Every AST node:
1. Inherits from `x3::position_tagged` (for source location)
2. Has `static constexpr std::string_view k_name` (for error messages)
3. Uses `make_<node>()` helper for construction
4. Implements `to_json()` with snake_case keys

```cpp
// In ast.hpp
struct My_Node : x3::position_tagged {
  static constexpr std::string_view k_name = "my_node";
  std::string value;
};

inline auto make_my_node(std::string a_value) -> My_Node {
  return My_Node{{}, std::move(a_value)};  // ⚠️ Empty braces for base class
}

inline void to_json(nlohmann::json& a_json, My_Node const& a_node) {
  a_json = {{"value", a_node.value}};  // ⚠️ snake_case keys
}
```

### Parser Rule Pattern
```cpp
// In rules.hpp
PARSE_FN_DECL(My_Node, my_node)  // Declares parse_my_node()

// In rules.cpp
namespace {
  struct My_Node_Tag : x3::annotate_on_success, Error_Handler {};
  auto const my_node_parser = /* parser expression */;
  auto const k_my_node_rule = x3::rule<My_Node_Tag, ast::My_Node>{"my_node"};
  
  BOOST_SPIRIT_DEFINE(k_my_node_rule)
}

PARSE_FN_IMPL(My_Node, my_node, k_my_node_rule)
BOOST_SPIRIT_INSTANTIATE(k_my_node_rule, Iterator_Type, Context_Type)
```

### Variants (Sum Types)
```cpp
struct Expr : x3::variant<Integer, String, Binary_Expr>, x3::position_tagged {
  using Base_Type = x3::variant<Integer, String, Binary_Expr>;
  using Base_Type::Base_Type;      // Inherit constructors
  using Base_Type::operator=;       // Inherit assignment
  static constexpr std::string_view k_name = "expr";
};
```

### Recursive Types
Use `x3::forward_ast<T>` (wraps in `shared_ptr`):
```cpp
struct Binary_Expr {
  x3::forward_ast<Expr> left;   // ⚠️ Not unique_ptr - X3 requires copyable
  Operator op;
  x3::forward_ast<Expr> right;
};
```

---

## Spirit X3 Parser Patterns

### Operator Reference
| Operator | Meaning | Backtracking | Use Case |
|----------|---------|--------------|----------|
| `a > b` | Expect | ❌ No - commits after `a` | Required syntax: `k_kw_fn > k_snake_case` |
| `a >> b` | Sequence | ✅ Yes - commits after both | Optional syntax: `k_stmt >> ';'` |
| `a \| b` | Alternative | First match wins | Try alternatives: `k_trait_impl \| k_impl_block` |
| `a % b` | List | One or more | `k_param % ','` |
| `-(a % b)` | Optional list | Zero or more | `-(k_param % ','` for empty `()` |
| `-a` | Optional | Zero or one | `-k_type_annotation` |
| `*a` | Kleene star | Zero or more | `*k_whitespace` |
| `+a` | Plus | One or more | `+x3::digit` |

### Critical Pattern: Statement Semicolons
**Always use `>> ';'` for statements to enable block trailing expressions:**
```cpp
// ✅ CORRECT - allows backtracking when semicolon missing
auto const k_return_statement_rule = k_kw_return > -k_expr >> ';';

// ❌ WRONG - expects semicolon, errors if missing
auto const k_return_statement_rule = k_kw_return > -k_expr > ';';
```
**Why**: Blocks can end with expressions (no semicolon). Statement parsers must fail gracefully and backtrack to try expression statement.

### Rule Ordering
Order alternatives **most specific → most general**:
```cpp
// ✅ CORRECT
auto const k_impl_rule = k_trait_impl | k_impl_block;  // trait impl is more specific

// ❌ WRONG
auto const k_impl_rule = k_impl_block | k_trait_impl;  // impl_block matches trait impls
```

### Semantic Validation
Reject valid parses when semantics require it:
```cpp
auto const validate = [](auto& a_ctx) {
  auto const& node = x3::_attr(a_ctx);
  if (/* invalid condition */) {
    x3::_pass(a_ctx) = false;  // Reject, try next alternative
    return;
  }
};

auto const k_rule = parser[validate];
```
**Example**: Reject `()` as enum type name to prefer tuple pattern.

### Error Handling
- **Tag structs**: Must inherit from **both** `x3::annotate_on_success` and `Error_Handler`
- **Error_Handler::on_error()**: Intercepts expectation failures, logs to `x3::error_handler` stream
- **parse_with_rule()**: Extracts errors, builds clang-style diagnostics with line:column

---

## Testing Pattern

### Standard Test Structure
```cpp
#include "utils.hpp"
using life_lang::ast::My_Node;

PARSE_TEST(My_Node, parse_my_node)  // Generates check_parse() and Params type

namespace {
// Constants in order: should_succeed → input → expected
constexpr auto k_simple_should_succeed = true;
constexpr auto k_simple_input = "code";
inline auto const k_simple_expected = R"({"value": "code"})";

// Use helpers to reduce duplication
inline auto const k_complex_expected = fmt::format(
  R"({{"field": {}}})", test_json::var_name("x")
);
}

TEST_CASE("Parse My_Node", "[parser]") {
  auto const params = GENERATE(
    Catch::Generators::values<My_Node_Params>({
      {"simple", k_simple_input, k_simple_expected, k_simple_should_succeed},
      {"invalid", "bad input", "", false},  // should_succeed = false
    })
  );
  DYNAMIC_SECTION(params.name) { check_parse(params); }
}
```

### Test Helpers
- `test_json::var_name("name")` - Generate Variable_Name JSON
- `test_json::type_name("Type")` - Generate Type_Name JSON
- `fmt::format()` - Dynamic JSON construction

---

## Naming Conventions

### C++ Code (Enforced by `.clang-tidy`)
- **Types**: `Camel_Snake_Case` - `Path_Segment`, `Iterator_Type`
- **Functions/variables**: `lower_case` - `parse_expr`, `token_count`
- **Parameters**: `a_` prefix - `a_begin`, `a_value`
- **Private members**: `m_` prefix - `m_data`, `m_count`
- **Global constants**: `k_` prefix - `k_path_rule`, `k_kw_fn`
- **Parser rules**: `k_<name>_rule` - `k_expr_rule`, `k_statement_rule`
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
| Missing `BOOST_SPIRIT_INSTANTIATE()` | Linker error | Add instantiation in rules.cpp |
| Designated initializers with `position_tagged` | Compile error | Use `Type{{}, member1, member2}` |
| `> ';'` for statements | Blocks reject trailing expressions | Use `>> ';'` instead |
| `a % b` for optional lists | Rejects empty `()` | Use `-(a % b)` |
| Wrong alternative order | Wrong parser matches | Order specific → general |
| Parser structure can't enforce constraint | Accepts invalid input | Add semantic validation |

---

## Debugging

### Approach 1: Manual Tracing (Recommended)
Add debug output in semantic actions:
```cpp
auto const debug = [](auto& a_ctx) {
  auto const& node = x3::_attr(a_ctx);
  std::cerr << "Parsed: " << node.value << "\n";
};
auto const k_rule = parser[debug];
```
**Pros**: Targeted, shows actual values  
**Cons**: Requires code changes

### Approach 2: BOOST_SPIRIT_DEBUG
Enable automatic trace:
```cpp
BOOST_SPIRIT_DEFINE(k_my_rule)
BOOST_SPIRIT_DEBUG(k_my_rule)
```
**Pros**: No code changes, comprehensive  
**Cons**: Extremely verbose, hard to parse

---

## Next Phase: Semantic Analysis

**Status**: Symbol table and declaration collection implemented. Name resolution in progress.

Completed:
1. ✅ **Symbol table**: Tracks declarations (functions, types, variables) with scope management
2. ⏳ **Name resolution**: Resolve identifiers to declarations (in progress)
3. ❌ **Type checking**: Verify type correctness (TODO)
4. ❌ **Trait resolution**: Check trait bounds, impl blocks (TODO)
5. ❌ **Borrow checking**: Validate lifetime rules (future)

Key considerations:
- Preserve position information for error reporting
- Build on existing JSON serialization for IR output
- Maintain value semantics throughout
