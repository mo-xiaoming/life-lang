# life-lang Compiler - AI Coding Agent Instructions

**IMPORTANT**: This file contains detailed instructions for the AI coding agent to assist in the development of the life-lang compiler. AI should follow these guidelines closely to ensure code quality, consistency, and maintainability, always adhering to the established patterns and conventions, applying best practices in C++20, Boost.Spirit X3, and software engineering.

## Meta-Instruction: Keeping This Document Up-to-Date

**CRITICAL**: Whenever you (the AI) make changes that the user accepts and stages/commits, you MUST update this document to reflect those changes. This includes:

- **New patterns established**: If you introduce a new coding pattern that works well, document it here
- **Architecture changes**: Any structural changes to how AST nodes, parsers, or tests are organized
- **Naming conventions**: Any evolution in naming patterns (new prefixes, suffixes, or style changes)
- **Tool usage**: New tools, build configurations, or development workflows
- **Best practices discovered**: Solutions to tricky problems (e.g., Spirit X3 quirks, clang-tidy false positives)
- **Common pitfalls**: New mistakes to avoid based on actual development experience
- **Removal of obsolete advice**: Strike through or remove outdated patterns that have been superseded

**How to update**: When the user asks you to update this file, review `git diff --cached` to see what changed, then update all relevant sections to match the current codebase state. Add new sections if introducing fundamentally new concepts.

**Why this matters**: This document is the source of truth for how the codebase should be developed. Keeping it synchronized with actual code changes ensures consistency and helps both you (in future conversations) and other developers understand the current conventions.

## Project Overview
A C++20 compiler for the "life-lang" language using the latest Boost.Spirit X3 for parsing. The architecture is parser → AST → JSON serialization. Example hello world:
```life
fn main(args: Std.Array<Std.String>): I32 {
    Std.print("Hello, world!");
    return 0;
}
```

## Future Goals
- Full language support: control flow, data structures, modules, reflection etc. (in progress)
- Intermediate representation (IR) generation (JSON) from AST (in progress)
- Interpreter or JIT execution of JSON IR
- Optimizations on JSON IR
- Lower JSON IR to LLVM IR or machine code

## Critical Setup Requirements
- Dependencies managed via vcpkg
- Requires GCC with full C++20 support (GCC 14 recommended)
- Ninja build system required

## Build & Test Workflow
```bash
# **Note**: Use `dev` preset for none clang-tidy builds for faster iteration.

# Configuration (use 'dev' preset for development - disables clang-tidy)
cmake --preset dev              # or debug, release, debug-asan-ubsan, coverage

# Build
cmake --build --preset dev

# Test
ctest --preset dev

# Run compiler
./build/dev/src/lifec
```

## Code Organization
- **src/ast.hpp**: AST node definitions using Boost.Spirit X3 variants and forward_ast for recursive structures
  - **Organized into 9 sections**: Forward Declarations, Type & Path System, Literal Types, Expression Types, Statement Types, Function Types, Module Types, Helper Functions, JSON Serialization, Utility
  - All nodes follow pattern: `struct Type { static constexpr string_view k_name; /* fields */ };`
  - Helper functions: `make_path()`, `make_integer()`, etc. for construction (with `a_` prefix for parameters)
  - JSON serialization via `to_json_string()` (defined with ADL)
  - **Module**: Top-level AST node representing a compilation unit (contains zero or more function definitions)
  
- **src/rules.hpp/cpp**: Boost.Spirit X3 parser rules
  - **Public API**: `parser::parse()` returns `Parse_Result<ast::Module>` - parses complete compilation units
  - **Internal API**: `internal::Parse_<Type>()` functions for testing individual AST nodes
  - Each rule has Tag struct inheriting `error_handler, x3::position_tagged`
  - Pattern: `x3::rule<Tag, ast::Type> const rule = "description";`
  - Must use `BOOST_SPIRIT_DEFINE()` and `BOOST_SPIRIT_INSTANTIATE()` macros
  - **Centralized keyword management**: `k_keywords` symbol table with individual `k_kw_*` parsers
  - Use NOLINT comments for false-positive clang-tidy warnings (e.g., `misc-const-correctness` on mutable symbol tables)

- **src/main.cpp**: Entry point - parses hardcoded test input and prints JSON AST

- **tests/parser/**: Parameterized Catch2 tests using custom `PARSE_TEST()` macro
  - Test structure: `Parse_Test_Params<Ast_Type>` with input, expected, should_succeed, rest
  - Naming convention: `test_<ast_node>.cpp` tests `parse_<ast_node>` function
  - Use `utils.hpp` macros for test boilerplate

## AST Design Patterns
- **Recursive structures** use `boost::spirit::x3::forward_ast<T>` to break circular dependencies
  - Example: `Function_Call_Expr` in `Expr` variant (Expr contains Function_Call_Expr which contains vector<Expr>)
  - `forward_ast` wraps types in `shared_ptr` internally for copyability while deferring instantiation
  - **Cannot be replaced with `std::unique_ptr`** - Spirit X3 requires copyable attributes
- **Variants** for sum types: `Expr`, `Statement` are `x3::variant<...>`
- **No Boost.Fusion adaptation needed** - structs use C++20 aggregate initialization

## Testing Conventions
- **JSON comparison**: Tests compare `to_json_string(expected, 2)` vs `to_json_string(*got, 2)`
- **Input exhaustion**: Tests verify `.rest` (unparsed input) matches expected remainder
- **Failure cases**: Include `should_succeed = false` tests for invalid syntax

## CMake Presets
- **dev**: Debug build, clang-tidy OFF (fast iteration)
- **debug**: Debug build, clang-tidy ON (strict checking)
- **release**: Optimized with `-march=native` and IPO/LTO
- **debug-asan-ubsan**: Debug + AddressSanitizer + UndefinedBehaviorSanitizer
- **coverage**: Debug build with --coverage flags (GCC only)

## Compiler Flags Philosophy
- **Debug mode**: `-O0 -ggdb3 -fno-omit-frame-pointer -fno-inline -D_GLIBCXX_ASSERTIONS`
- **Warnings**: Extensive (`-Wall -Wextra -Wshadow -Wold-style-cast -Wcast-qual` etc.)
- **Release mode**: `-Werror -Wfatal-errors -march=native`
- **No exceptions/RTTI**: Commented out but ready to enable

## Common Pitfalls
- **Forgetting BOOST_SPIRIT_INSTANTIATE**: Rules won't link without explicit instantiation
- **Module flag conflicts**: GCC 14 requires CMAKE_CXX_SCAN_FOR_MODULES OFF for clang-tidy
- **Symbol table pollution**: Add all keywords to symbol table before defining rules
- **Underscore validation**: Integer literals reject leading/trailing underscores in semantic action
- **False positive clang-tidy warnings**: Use NOLINT comments when necessary (e.g., `misc-const-correctness` for symbol table initialization)

## Naming Conventions
**All naming conventions are enforced by `.clang-tidy` configuration.**

### Key Conventions (see .clang-tidy for complete rules):
- **Types** (struct/class/typedef/type alias): `Camel_Snake_Case` (e.g., `Path_Segment`, `Iterator_Type`)
- **Functions**: `lower_case` (e.g., `parse`, `make_path`)
- **Variables/parameters**: `lower_case`
- **Parameter prefix**: `a_` (e.g., `a_begin`, `a_value`)
- **Private members prefix**: `m_` (e.g., `m_data`, `m_count`)
- **Global constants/constexpr**: `lower_case` with `k_` prefix (e.g., `k_path_rule`, `k_kw_fn`)
- **Namespaces**: `lower_case` (e.g., `life_lang`, `parser`)

### File Naming:
- Parser tests: `test_<lowercase_ast_node>.cpp`

### Rule Naming Pattern:
- **Main/public rules** (with `_rule` suffix): Rules that parse complete AST nodes and have a public `Parse_<Type>()` function
  - Examples: `k_path_rule`, `k_expr_rule`, `k_function_parameter_rule`
  - Pattern: `x3::rule<Tag, ast::Type> const k_<name>_rule = "description";`
  - Must have corresponding `PARSE_FN_DECL(<Type>)` in `rules.hpp`
- **Helper/utility rules** (no `_rule` suffix): Internal parsers used as building blocks
  - Examples: `k_segment_name`, `k_template_params`, `k_func_name`, `k_kw_fn`, `k_snake_case`
  - Pattern: `x3::rule<tag, Type> const k_<name> = "description";` or `auto const k_<name> = ...;`
  - Not exposed in public API, used only within rule definitions

## When Adding New AST Nodes
1. Define struct in `ast.hpp` with `k_name` constant
2. Add `make_<node>()` helper function (with `a_` prefix for parameters)
3. Define parser rule in `rules.cpp` with Tag struct
   - Use `_rule` suffix: `x3::rule<Tag, ast::Type> const k_<name>_rule = "description";`
   - Add corresponding `_def` definition: `auto const k_<name>_rule_def = ...;`
4. Add `PARSE_FN_DECL(<Type>)` macro in `rules.hpp` to declare public parse function
5. Implement `Parse_<Node>()` wrapper function in `rules.cpp`
6. Add `BOOST_SPIRIT_DEFINE(k_<name>_rule)` and `BOOST_SPIRIT_INSTANTIATE` in `rules.cpp`
7. Create `test_<node>.cpp` with parameterized tests
8. Add to relevant variant type if needed (e.g., `Statement`, `Expr`)
