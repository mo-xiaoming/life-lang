# life-lang Compiler - AI Coding Agent Instructions

High-level guidance for AI assistance on this C++20 compiler project using Boost.Spirit X3.  
**When in doubt, check existing code patterns - they're the source of truth.**

## Architecture
Parser (Boost.Spirit X3) → AST (position-tagged nodes) → JSON serialization  
Use `dev` preset for fast iteration (clang-tidy disabled), `debug` for strict checks.

## Key Files
- **src/ast.hpp**: All AST nodes inherit from `x3::position_tagged` with `k_name` constant
- **src/rules.cpp**: Spirit X3 parsers use `BOOST_SPIRIT_DEFINE()` + `BOOST_SPIRIT_INSTANTIATE()`
  - Keywords: `struct Keyword_Symbols : x3::symbols<>` (canonical Spirit X3 pattern)
  - Tag structs: `struct Tag : x3::annotate_on_success, Error_Handler {}` (both required)
  - Error handling: Done in `parse_with_rule()`, extracts from `x3::error_handler` stream
- **tests/parser/**: Catch2 tests using `PARSE_TEST()` macro from `utils.hpp`

## Critical Patterns

### AST Construction (C++20 + position_tagged)
- **Cannot use designated initializers** with `position_tagged` base class
- **Must use**: `return Type{{}, member1, member2};` (empty braces for base)
- **NOT**: `return Type{.member = value};` (compile error)

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
**Enforced by `.clang-tidy` - see that file for complete rules.**

- **Types**: `Camel_Snake_Case` (e.g., `Path_Segment`, `Iterator_Type`)
- **Functions/variables**: `lower_case`
- **Parameters**: `a_` prefix (e.g., `a_begin`, `a_value`)
- **Private members**: `m_` prefix (e.g., `m_data`, `m_count`)
- **Global constants**: `k_` prefix (e.g., `k_path_rule`, `k_kw_fn`)
- **Parser rule pattern**: `k_<name>_rule` for public rules with `PARSE_FN_DECL()`
- **Test files**: `test_<lowercase_ast_node>.cpp`

## Adding New AST Nodes
1. Define struct in `ast.hpp` inheriting from `x3::position_tagged` with `k_name`
2. Add `make_<node>()` helper - use `Type{{}, member1, member2}` (not designated init)
3. Define parser in `rules.cpp` with Tag inheriting from `x3::annotate_on_success, Error_Handler`
4. Add `BOOST_SPIRIT_DEFINE()` and `BOOST_SPIRIT_INSTANTIATE()`
5. Use `PARSE_FN_DECL()` in `rules.hpp` and `PARSE_FN_IMPL()` in `rules.cpp`
6. Create `test_<node>.cpp` with parameterized tests
7. Add to variant if needed (e.g., `Statement`, `Expr`)
