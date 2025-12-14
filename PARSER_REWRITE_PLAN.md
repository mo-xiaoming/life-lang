# Parser Rewrite Plan: Spirit X3 → Hand-Written Recursive Descent

## Motivation

**Problem**: Boost.Spirit X3 causes extremely slow builds with clang-tidy enabled
- Current build time: ~20 minutes (debug preset with clang-tidy)
- Bottleneck: `rules.cpp` (2400+ lines of Spirit X3 template code)
- Impact: `ast.hpp` includes `x3::position_tagged`, `x3::variant`, `x3::forward_ast` - included in all files
- Future: Will only get worse as codebase grows

**Solution**: Hand-written recursive descent parser (like rustc, Go compiler)
- Expected build time: ~2-3 minutes
- clang-tidy analysis: seconds instead of minutes
- Full control over error messages
- Zero heavy dependencies (no Boost)

## Pre-Rewrite Checklist

- [x] Semantic analysis work saved to separate branch
- [ ] Return to parser-only branch (before semantic analysis)
- [ ] Verify all 70 tests passing (1093 assertions)
- [ ] Document current test structure

## Architecture Changes

### Files to Modify

| Current File | New File | Changes |
|--------------|----------|---------|
| `src/ast.hpp` | `src/ast.hpp` | Remove `x3::position_tagged`, `x3::variant`, `x3::forward_ast` |
| `src/rules.cpp` (2400 lines) | `src/parser.cpp` | Hand-written recursive descent |
| `src/rules.hpp` | `src/parser.hpp` | Parser class/function declarations |
| `src/diagnostics.cpp` | `src/diagnostics.cpp` | Minimal changes (already independent) |
| `vcpkg.json` | `vcpkg.json` | Remove `boost-spirit` dependency |

### Files Unchanged

- **All test files** (`tests/parser/*.cpp`, `tests/integration/*.cpp`)
- **Test utilities** (`tests/parser/utils.hpp`, `tests/parser/internal_rules.hpp`)
- **Test data** - All input/expected/should_succeed values stay identical

## New AST Design

### Position Tracking
Replace `x3::position_tagged` with custom struct:
```cpp
struct Source_Position {
  std::size_t line;
  std::size_t column;
  std::size_t offset;  // Byte offset in source
};

struct Source_Range {
  Source_Position start;
  Source_Position end;
};
```

Every AST node includes `Source_Range pos;` member.

### Variant Types
Replace `x3::variant<A, B, C>` with `std::variant<A, B, C>`:
```cpp
// Before
struct Type_Name : x3::variant<Path_Type, Tuple_Type, Function_Type>,
                   x3::position_tagged { ... };

// After
struct Type_Name {
  std::variant<Path_Type, Tuple_Type, Function_Type> value;
  Source_Range pos;
};
```

### Recursive Types
Replace `x3::forward_ast<T>` with `std::unique_ptr<T>`:
```cpp
// Before
struct Binary_Expr : x3::position_tagged {
  x3::forward_ast<Expr> lhs;
  Operator op;
  x3::forward_ast<Expr> rhs;
};

// After
struct Binary_Expr {
  std::unique_ptr<Expr> lhs;
  Operator op;
  std::unique_ptr<Expr> rhs;
  Source_Range pos;
};
```

## Parser Implementation

### Core Parser Class
```cpp
class Parser {
public:
  explicit Parser(std::string_view a_source);
  
  auto parse_module() -> tl::expected<ast::Module, std::vector<Diagnostic>>;
  
private:
  // Lexer state
  std::string_view m_source;
  std::size_t m_pos = 0;
  std::size_t m_line = 1;
  std::size_t m_column = 1;
  
  // Error accumulation
  std::vector<Diagnostic> m_errors;
  
  // Parsing functions (one per AST node)
  auto parse_expr() -> tl::expected<ast::Expr, void>;
  auto parse_statement() -> tl::expected<ast::Statement, void>;
  auto parse_func_def() -> tl::expected<ast::Func_Def, void>;
  // ... etc
  
  // Utilities
  auto peek() const -> char;
  auto peek(std::size_t a_offset) const -> char;
  auto advance() -> char;
  void skip_whitespace_and_comments();
  auto current_position() const -> Source_Position;
  void error(std::string a_message, Source_Range a_range);
  auto expect(char a_ch) -> bool;
  auto expect(std::string_view a_str) -> bool;
  auto match_keyword(std::string_view a_keyword) -> bool;
  auto parse_identifier() -> tl::expected<std::string, void>;
};
```

### Parsing Strategy
**Top-down recursive descent with precedence climbing for expressions**

Example structure:
```cpp
auto Parser::parse_expr() -> tl::expected<ast::Expr, void> {
  return parse_binary_expr(0);  // Start with lowest precedence
}

auto Parser::parse_binary_expr(int a_min_precedence) -> tl::expected<ast::Expr, void> {
  auto lhs = parse_unary_expr();
  if (!lhs) return tl::unexpected();
  
  while (true) {
    auto op = peek_binary_operator();
    if (!op || precedence(*op) < a_min_precedence) break;
    
    advance_operator();
    auto rhs = parse_binary_expr(precedence(*op) + 1);
    if (!rhs) return tl::unexpected();
    
    lhs = ast::Binary_Expr{std::move(*lhs), *op, std::move(*rhs)};
  }
  
  return lhs;
}
```

### Error Recovery
- **Synchronization points**: Statement boundaries (`;`), block boundaries (`}`), item boundaries
- **Panic mode**: On error, skip tokens until synchronization point
- **Multiple errors**: Accumulate all errors, don't stop at first
- **Error messages**: Leverage existing `Diagnostic` infrastructure

## Migration Strategy

### Phase 1: Infrastructure (Week 1, Days 1-2)
1. Create new `Source_Position` and `Source_Range` structs in `diagnostics.hpp`
2. Create `Parser` class skeleton in `src/parser.hpp` and `src/parser.cpp`
3. Implement basic lexer utilities (peek, advance, skip_whitespace)
4. Write lexer tests (keywords, identifiers, operators)

### Phase 2: Simple Nodes (Week 1, Days 3-5)
Migrate in order of dependency (leaf nodes first):
1. **Literals**: Integer, Float, String, Char, Unit → Test: `test_integer.cpp`, `test_float.cpp`, etc.
2. **Names**: Variable_Name, Type_Name (path only) → Test: existing identifier tests
3. **Comments**: Line/block comments → Test: `test_comment.cpp`

### Phase 3: Type System (Week 2, Days 1-2)
1. **Type names**: Path_Type, Tuple_Type, Function_Type → Test: `test_func_type.cpp`, `test_unit_type.cpp`
2. **Type parameters**: Template syntax → Test: `test_trait_bounds.cpp`
3. **Where clauses** → Test: `test_where_clause.cpp`

### Phase 4: Expressions (Week 2, Days 3-5)
1. **Primary expressions**: Literals, variables, parenthesized
2. **Unary expressions**: `-`, `!`, `*`, `&` → Test: `test_unary_expr.cpp`
3. **Binary expressions**: Precedence climbing → Test: `test_binary_expr.cpp`
4. **Field access**: Dot notation → Test: `test_field_access.cpp`
5. **Function calls**: Method chaining → Test: `test_method_chaining.cpp`
6. **Control flow**: If, Match, For, While → Test: `test_if_expr.cpp`, `test_match_expr.cpp`, etc.
7. **Range expressions** → Test: `test_range_expr.cpp`
8. **Blocks** → Test: `test_block.cpp`

### Phase 5: Statements (Week 3, Days 1-2)
1. **Let statements** → Test: `test_let_statement.cpp`
2. **Assignment** → Test: `test_assignment.cpp`
3. **Expression statements**
4. **Return, Break, Continue** → Test: `test_break_statement.cpp`, `test_continue_statement.cpp`

### Phase 6: Declarations (Week 3, Days 3-5)
1. **Function definitions** → Test: `test_func_def.cpp`
2. **Struct definitions** → Test: `test_struct_def.cpp`
3. **Enum definitions** → Test: `test_enum_def.cpp`
4. **Trait definitions** → Test: `test_trait_def.cpp`
5. **Type aliases** → Test: `test_type_alias.cpp`
6. **Impl blocks** → Test: `test_impl_block.cpp`
7. **Trait impls** → Test: `test_trait_impl.cpp`

### Phase 7: Integration & Cleanup (Week 4)
1. **Module parsing**: Top-level integration → Test: `test_parse_module_integration.cpp`, `test_readme_example.cpp`
2. **Error message quality**: Polish diagnostics
3. **Performance testing**: Verify build time improvements
4. **Documentation**: Update EBNF grammar documentation
5. **Remove Boost dependency**: Update `vcpkg.json`, clean up includes

## Testing Strategy

### Continuous Validation
**After each phase, all affected tests must pass**
- Run tests incrementally: `ctest -R test_integer` (for specific test)
- Run all tests: `ctest --preset dev` (fast, no clang-tidy)
- Full validation: `ctest --preset debug` (with clang-tidy, before merge)

### Test Compatibility
**No test file changes required** - tests use high-level `parse_<node>()` functions:
```cpp
// tests/parser/utils.hpp - interface stays the same
template<typename T>
auto parse_with_parser(std::string_view input) -> tl::expected<T, std::vector<Diagnostic>>;

// Implementation changes from Spirit X3 to recursive descent, but signature identical
```

### Success Criteria
- [ ] All 70 test files pass (1093 assertions)
- [ ] JSON output format unchanged
- [ ] Error message quality equal or better
- [ ] Build time < 5 minutes (debug preset with clang-tidy)
- [ ] clang-tidy analysis < 2 minutes

## Rollback Plan

If rewrite encounters blockers:
1. **Semantic analysis work**: Already saved in separate branch
2. **Parser rewrite**: Can abandon and return to Spirit X3 version
3. **Partial completion**: Can merge phases incrementally if some parts work

Git workflow:
```bash
# Current state: semantic analysis on sym-table branch
git checkout -b parser-rewrite-backup  # Backup before starting
git checkout -b parser-rewrite         # Work branch
# ... implement rewrite ...
# If successful: merge to main
# If blocked: git checkout sym-table (semantic work safe)
```

## Expected Outcomes

### Build Performance
| Metric | Before (Spirit X3) | After (Hand-written) | Improvement |
|--------|-------------------|---------------------|-------------|
| Debug build (clang-tidy) | ~20 minutes | ~3-5 minutes | **75% faster** |
| Dev build (no tidy) | ~2 minutes | ~1 minute | 50% faster |
| Incremental rebuild | ~1 minute | ~10 seconds | 80% faster |
| CI build time | ~20 minutes | ~5 minutes | 75% faster |

### Code Metrics
- **Parser code**: ~2400 lines Spirit X3 → ~3000-3500 lines hand-written (40% increase)
- **AST code**: ~800 lines (similar, just different base types)
- **Total codebase**: ~6000 → ~7000 lines (15% increase, worth it for build speed)
- **Dependencies**: Boost.Spirit removed, only fmt, nlohmann-json, Catch2 remain

### Maintainability
**Pros**:
- Explicit control flow (no template metaprogramming)
- Better error messages possible
- Easier to debug (no template stack traces)
- Standard C++ patterns (no Spirit DSL to learn)

**Cons**:
- More code to maintain (~1000 extra lines)
- Manual precedence handling
- Error recovery requires explicit code

## References

### EBNF Grammar
Current grammar documented in `src/rules.cpp` lines 12-276 - **this is the spec**.
Hand-written parser must implement the exact same grammar.

### Similar Projects
- **Rust compiler (rustc)**: Hand-written recursive descent
- **Go compiler**: Hand-written recursive descent  
- **Clang**: Hand-written recursive descent
- **V8 JavaScript**: Hand-written recursive descent

Pattern: Production compilers favor hand-written parsers for performance and control.

### Resources
- Parser architecture: Crafting Interpreters by Robert Nystrom (Chapter 6)
- Precedence climbing: https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
- Error recovery: https://www.cs.cornell.edu/courses/cs412/2008sp/lectures/lec14.pdf

## Timeline

**Total estimated time**: 3-4 weeks (part-time work)
- Week 1: Infrastructure + simple nodes + type system (50% complete)
- Week 2: Expressions (75% complete)
- Week 3: Statements + declarations (95% complete)
- Week 4: Integration, polish, testing (100% complete)

**Checkpoints**:
- End of Week 1: All literal and type tests passing
- End of Week 2: All expression tests passing
- End of Week 3: All statement/declaration tests passing
- End of Week 4: Full integration tests passing, ready to merge

## Next Steps

1. Save semantic analysis work to separate branch ✓ (user doing this)
2. Return to parser-only state (checkout commit before semantic analysis)
3. Create `parser-rewrite` branch
4. Begin Phase 1: Infrastructure
5. Iterate through phases, validating tests continuously
