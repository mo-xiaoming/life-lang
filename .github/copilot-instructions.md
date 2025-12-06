# life-lang Compiler - AI Coding Agent Instructions

High-level guidance for AI assistance on this C++20 compiler project using Boost.Spirit X3.  
**When in doubt, check existing code patterns - they're the source of truth.**

## Architecture
Parser (Boost.Spirit X3) → AST (position-tagged nodes) → JSON serialization  
Use `dev` preset for fast iteration (clang-tidy disabled), `debug` for strict checks.

## Language Design Principles

### Type System
- **Value semantics**: Immutable by default, modifications return new values
- **No destructors initially**: Simplifies memory model during bootstrap
- **Structs**: Records with named fields, public by default
  - Fields: `struct Point { x: I32, y: I32 }`
  - Literals: `Point { x: 1, y: 2 }` (trailing comma optional)
  - Access: `point.x`, `obj.field.nested` (chained field access)

### Functions and Methods
- **Keywords**: `fn` for functions, `self` reserved keyword for UFCS parameter
- **UFCS Supported**: Functions with `self` parameter can be used like methods
  - Define: `fn distance(self: Point): I32 { return 42; }`
  - Call syntax: `obj.method()` desugars to `method(obj)` (semantic analysis phase, not parser)
  - `self` allowed as parameter/variable name despite being a keyword
- **Mutation**: `mut self` for methods that modify receiver (future feature)
- **No implicit `this`**: Explicit `self` parameter required

### Values and Scoping
- **No special "constant" concept**: Everything is immutable by default (value semantics)
- **Module-level values**: Use functions instead of constants
  - `fn max_size(): I32 { return 100; }`
  - `fn default_config(): Config { return Config { timeout: 30 }; }`
- **Rationale**: Simpler design, no need for separate constant category in value semantics

## Key Files
- **src/ast.hpp**: All AST nodes inherit from `x3::position_tagged` with `k_name` constant
- **src/rules.cpp**: Spirit X3 parsers use `BOOST_SPIRIT_DEFINE()` + `BOOST_SPIRIT_INSTANTIATE()`
  - Keywords: `struct Keyword_Symbols : x3::symbols<>` (canonical Spirit X3 pattern)
  - Tag structs: `struct Tag : x3::annotate_on_success, Error_Handler {}` (both required)
  - Error handling: Done in `parse_with_rule()`, extracts from `x3::error_handler` stream

## Critical Patterns

### AST Construction (C++20 + position_tagged)
- **Cannot use designated initializers** with `position_tagged` base class
- **Must use**: `return Type{{}, member1, member2};` (empty braces for base)

### Recursive AST Nodes
- Use `x3::forward_ast<T>` to break circular dependencies (wraps in `shared_ptr`)
- **Cannot use `std::unique_ptr`** - Spirit X3 requires copyable types

### Variants as Structs (Spirit X3 idiom)
```cpp
struct Expr : x3::variant<...>, x3::position_tagged {
  using Base_Type = x3::variant<...>;
  using Base_Type::Base_Type;      // Inherit constructors
  using Base_Type::operator=;      // Inherit assignment
};
```

### Error Handling Architecture
- **Error_Handler::on_error()**: Intercepts expectation failures, logs to `x3::error_handler` stream
  - Returns `fail` (stop) - no error recovery yet
- **parse_with_rule()**: Extracts errors from stream, builds clang-style diagnostics
  - Combines rule name with Spirit X3's expectation message
  - Uses `Position_Tracker` for line:column conversion
- **Tag Structs**: Must inherit from both `x3::annotate_on_success` and `Error_Handler`
- **Limitation**: Must use `x3::error_handler` (required by `annotate_on_success` for position tracking)

## Common Pitfalls
- Forgetting `BOOST_SPIRIT_INSTANTIATE()` - rules won't link
- Using designated initializers with `position_tagged` - won't compile
- GCC 14+ requires `CMAKE_CXX_SCAN_FOR_MODULES OFF` for clang-tidy compatibility

## Naming Conventions

### C++ Implementation (Enforced by `.clang-tidy`)
- **Types**: `Camel_Snake_Case` (e.g., `Path_Segment`, `Iterator_Type`)
- **Functions/variables**: `lower_case`
- **Parameters**: `a_` prefix (e.g., `a_begin`, `a_value`)
- **Private members**: `m_` prefix (e.g., `m_data`, `m_count`)
- **Global constants**: `k_` prefix (e.g., `k_path_rule`, `k_kw_fn`)
- **Parser rule pattern**: `k_<name>_rule` for public rules with `PARSE_FN_DECL()`
- **Test files**: `test_<lowercase_ast_node>.cpp`

### life-lang Language (Parser enforces via rules)
- **`snake_case`**: Variables, functions, struct fields, function parameters
  - Example: `my_var`, `calculate_sum`, `field_name`, `param_value`
  - Validated by: `k_snake_case` parser rule (lowercase start, allows `_` and digits)
  - Semantic: Object-related identifiers (runtime values, behaviors on objects)

- **`Camel_Snake_Case`**: Types, struct names, modules
  - Example: `Point`, `User_Profile`, `Std`, `Http_Request`
  - Validated by: `k_camel_snake_case` parser rule (uppercase start, allows `_` and digits)
  - Semantic: Type and module identifiers (compile-time namespace/organization)

**Note:** No separate "constant" naming convention needed. In a value semantics system, everything is immutable by default. Use functions for module-level values: `fn max_size(): I32 { return 100; }`

### Expression Context Rules
- **Paths with dots**: Only in type positions and function call names
  - `Std.IO.print()` → function call with qualified name
  - `param: Std.String` → type annotation with qualified path
- **Field access**: Dots in expression positions are always field access
  - `obj.field.nested` → parsed as `Field_Access_Expr` chain

## Adding New AST Nodes
1. Define struct in `ast.hpp` inheriting from `x3::position_tagged` with `k_name`
2. Add `make_<node>()` helper - use `Type{{}, member1, member2}` (not designated init)
3. Define parser in `rules.cpp` with Tag inheriting from `x3::annotate_on_success, Error_Handler`
4. Add `BOOST_SPIRIT_DEFINE()` and `BOOST_SPIRIT_INSTANTIATE()`
5. Use `PARSE_FN_DECL()` in `rules.hpp` and `PARSE_FN_IMPL()` in `rules.cpp`
6. Create `test_<node>.cpp` with parameterized tests (see Test Pattern below)
7. Add to variant if needed (e.g., `Statement`, `Expr`)

## Test Pattern (All test files must follow this)

### Standard Pattern (with JSON comparison)
```cpp
#include "utils.hpp"
using life_lang::ast::Ast_Type;

PARSE_TEST(Ast_Type, parser_function_name)  // Generates check_parse() and Params type

namespace {
// For each test case, define constants in this exact order:
constexpr auto k_test_name_should_succeed = true;  // 1. SUCCESS FLAG FIRST
constexpr auto k_test_name_input = "code";          // 2. Input second
inline auto const k_test_name_expected = R"(json)"; // 3. Expected LAST

// Use helper functions to reduce JSON duplication:
// - test_json::var_name("name") for Variable_Name JSON
// - test_json::type_name("Type") for Type_Name JSON
// - fmt::format() for dynamic JSON generation
}  // namespace

TEST_CASE("Parse Ast_Type", "[parser]") {
  auto const params = GENERATE(
    Catch::Generators::values<Ast_Type_Params>({
      {"test", k_test_name_input, k_test_name_expected, k_test_name_should_succeed},
    })
  );
  DYNAMIC_SECTION(params.name) { check_parse(params); }
}
```

### Special Case (success-only validation, no JSON comparison)
When expected JSON is too complex to construct (e.g., deeply nested variants):
```cpp
// See tests/parser/test_field_access.cpp for full example
namespace {
constexpr auto k_test_should_succeed = true;  // Still use should_succeed first
constexpr auto k_test_input = "code";
// No k_test_expected - will manually verify parse result
}

TEST_CASE("Parse Special", "[parser]") {
  // Custom Test_Case struct without expected field
  // Manually call parse function and verify success/failure only
}
```

### Key Principles
1. **Constant order**: `should_succeed` → `input` → `expected` (always)
2. **Extract all inline data**: No inline strings/JSON in test arrays
3. **Use PARSE_TEST macro**: Unless JSON comparison is impractical
4. **Group related constants**: Keep test case constants together with comments
5. **Invalid tests**: Use `should_succeed = false` for error cases
