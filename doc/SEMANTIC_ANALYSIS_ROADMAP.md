# Semantic Analysis Phase - Implementation Roadmap

**Status**: Parser complete, ready to implement semantic analysis
**Goal**: Build type-safe, module-aware semantic analyzer with comprehensive error reporting

---

## Overview

Semantic analysis transforms the parsed AST into a validated, type-checked representation with full symbol resolution and visibility enforcement. This phase catches logical errors that can't be detected by syntax parsing alone.

**Key Outputs:**

1. Symbol table with complete type information
2. Resolved imports and module dependencies
3. Type-checked AST with inferred/validated types
4. Module dependency graph (for compilation ordering)
5. High-quality error diagnostics for semantic violations

---

## Phase 1: Symbol Table Foundation

**Goal**: Build the core infrastructure for tracking symbols across modules

### 1.1 Symbol Table Design

**Components:**

- **Symbol**: Represents a declaration (function, type, variable, etc.)

  ```cpp
  struct Symbol {
    std::string name;
    Symbol_Kind kind;  // Function, Type, Variable, Module, etc.
    Type type;         // For typed symbols
    Visibility vis;    // Module_Internal or Public
    Source_Location location;  // For error reporting
    // Additional metadata per symbol kind
  };
  ```

- **Scope**: Namespace for symbols (module, block, function, etc.)

  ```cpp
  struct Scope {
    Scope_Kind kind;  // Module, Block, Function
    std::unordered_map<std::string, Symbol> symbols;
    Scope* parent;    // For nested scopes
  };
  ```

- **Symbol_Table**: Top-level manager

  ```cpp
  class Symbol_Table {
    // Module-level scopes (key = module path like "Geometry.Shapes")
    std::unordered_map<std::string, Scope> modules;

    // Current scope stack (for nested scopes during traversal)
    std::vector<Scope*> scope_stack;

    // API
    auto declare(std::string name, Symbol symbol) -> Result<void>;
    auto lookup(std::string name) -> std::optional<Symbol>;
    auto enter_scope(Scope_Kind kind) -> void;
    auto exit_scope() -> void;
  };
  ```

**Key Operations:**

- `declare()`: Add symbol to current scope, check for duplicates
- `lookup()`: Search current scope + parent chain
- Scope management: push/pop during AST traversal

### 1.2 Type Representation

**Type System Structure:**

```cpp
struct Type {
  enum class Kind {
    Primitive,  // I32, F64, Bool, String, etc.
    Struct,     // User-defined structs
    Enum,       // User-defined enums
    Function,   // fn(T): U
    Array,      // [T; N]
    Tuple,      // (T, U, V)
    Generic,    // T (type parameter)
    Unit,       // ()
  };

  Kind kind;
  std::string name;  // For named types
  std::vector<Type> params;  // For generics: Vec<T>, fn(I32): Bool
  // Additional fields per kind
};
```

**Builtin Types Registry:**

```cpp
struct Builtin_Types {
  Type i32, i64, f32, f64, bool_type, char_type, string;
  Type unit;  // ()

  auto lookup(std::string_view name) -> std::optional<Type>;
};
```

### 1.3 Module Registry

**Track all modules in the project:**

```cpp
struct Module_Info {
  std::string path;           // "Geometry.Shapes"
  Scope scope;                // Module's symbol table
  std::vector<std::string> dependencies;  // Imported modules
  Source_Location location;   // First file in module
};

class Module_Registry {
  std::unordered_map<std::string, Module_Info> modules;

  auto register_module(std::string path, ast::Module const& ast) -> Result<void>;
  auto get_module(std::string path) -> std::optional<Module_Info const*>;
  auto build_dependency_graph() -> std::vector<std::string>;  // Topological sort
};
```

**Tasks:**

- [x] ~~Implement `Symbol` struct with all metadata~~
- [x] ~~Implement `Scope` with symbol lookup and insertion~~
- [x] ~~Implement `Symbol_Table` with scope stack management~~
- [x] ~~Implement `Type` representation with all variants~~
- [x] ~~Register builtin types (I32, Bool, String, etc.)~~
- [x] ~~Implement `Module_Registry` for tracking all modules~~
- [x] ~~Add tests for symbol table operations~~

---

## Phase 2: Module System & Name Resolution

**Goal**: Implement folder-based modules with imports and visibility

### 2.1 Module Discovery

**Filesystem Processing:**

```cpp
class Module_Loader {
  // Scan project directory, identify modules
  auto discover_modules(fs::path root) -> std::vector<Module_Descriptor>;

  // Parse all files in a module, merge into single AST
  auto load_module(Module_Descriptor const& desc) -> ast::Module;
};

struct Module_Descriptor {
  std::string name;        // "Geometry"
  std::string path;        // "Geometry" (dot-separated)
  fs::path directory;      // "/src/geometry"
  std::vector<fs::path> files;  // All .life files
};
```

**Rules:**

- Top-level `.life` file → file-module (module name = filename)
- Folder with `.life` files → folder-module (module name = folder name)
- Nested folders → nested modules (`Geometry.Shapes`)

**Tasks:**

- [x] ~~**2.1.1** Create `Module_Descriptor` struct with name, path, directory, files~~
- [x] ~~**2.1.2** Implement filesystem scanning to find all `.life` files~~
  - ~~Distinguish file-modules (single `.life` file) from folder-modules (directory with `.life` files)~~
  - ~~Handle nested directories for qualified module names (e.g., `Geometry.Shapes`)~~
  - ~~Implement lowercase filesystem → Camel_Snake_Case conversion~~
  - ~~Enforce `src/` directory convention~~
- [x] ~~**2.1.3** Implement `Module_Loader::discover_modules()` to scan project root~~
  - ~~Build module descriptors for all discovered modules~~
  - ~~Automatic module name/path derivation from filesystem~~
- [x] ~~**2.1.5** Add comprehensive tests~~
  - ~~Test single-file modules~~
  - ~~Test folder modules with multiple files~~
  - ~~Test nested modules~~
  - ~~Test case conversion~~
- [x] ~~**2.1.4** Implement `Module_Loader::load_module()` to parse all files in a module~~
  - ~~Parse each `.life` file in the module~~
  - ~~Merge all top-level items into single `ast::Module`~~
  - ~~Report errors for parse failures~~

### 2.2 Import Resolution

**Process imports to build symbol dependencies:**

```cpp
class Import_Resolver {
  Symbol_Table& symtab;
  Module_Registry& registry;

  auto resolve_imports(
    ast::Module const& module,
    std::string module_path
  ) -> Result<void>;

  auto resolve_import_item(
    ast::Import_Item const& item,
    std::string module_path
  ) -> Result<Symbol>;
};
```

**Import Processing:**

1. Parse import statement: `import Geometry.{Point, Circle as C};`
2. Resolve module path: `"Geometry"` → lookup in registry
3. For each imported item:
   - Check if symbol exists in source module
   - Check if symbol is `pub` (visibility)
   - Add to current module's scope (with alias if present)
4. Track dependency edge: `current_module → imported_module`

**Error Cases:**

- Module not found: `import Unknown.{Item};`
- Item not found in module: `import Geometry.{NonExistent};`
- Item not public: `import Geometry.{InternalHelper};` (where `InternalHelper` is not `pub`)

**Tasks:**

- [ ] **2.2.1** Create `Import_Resolver` class with Symbol_Table and Module_Registry access
- [ ] **2.2.2** Implement `resolve_imports()` to process all imports in a module
  - Iterate through all import statements in module AST
  - Process each import item sequentially
- [ ] **2.2.3** Implement module path resolution
  - Parse dot-separated module path (e.g., `Geometry.Shapes`)
  - Lookup module in Module_Registry
  - Report error if module not found
- [ ] **2.2.4** Implement symbol lookup in imported module
  - For each imported item, lookup symbol in source module's scope
  - Check symbol exists
  - Verify symbol has `pub` visibility
  - Report detailed errors with source location
- [ ] **2.2.5** Handle import aliases
  - Support `Item as Alias` syntax
  - Add symbol to current scope with alias name
  - Preserve original symbol metadata
- [ ] **2.2.6** Implement dependency tracking
  - Record edge from current module to each imported module
  - Build dependency graph for compilation order
  - Detect and report circular dependencies
- [ ] **2.2.7** Add comprehensive tests
  - Test simple imports: `import Geometry.{Point};`
  - Test multiple items: `import Geometry.{Point, Circle, Line};`
  - Test aliases: `import Geometry.{Circle as C};`
  - Test nested modules: `import Std.Collections.{Vec, Map};`
  - Test error: module not found
  - Test error: symbol not found in module
  - Test error: symbol not public
  - Test error: circular dependencies

### 2.3 Name Resolution

**Resolve all identifiers to their declarations:**

```cpp
class Name_Resolver {
  Symbol_Table* symtab;  // Non-owning
  Diagnostic_Engine* diag;  // Non-owning

  // Resolve variable/function references
  auto resolve_var_name(ast::Var_Name const& name) -> Expected<Symbol, Diagnostic_Engine>;

  // Resolve type references
  auto resolve_type_name(ast::Type_Name const& name) -> Expected<Type, Diagnostic_Engine>;

  // Traverse AST and resolve all names
  auto resolve_module(ast::Module const& module) -> Expected<void, Diagnostic_Engine>;
};
```

**Status:** ✅ Basic implementation complete (single-segment names)

**Resolution Rules:**

**Tasks:**

- [x] ~~**2.3.1** Basic Name_Resolver implementation with single-segment names~~
- [x] ~~**2.3.2** Implement `resolve_var_name()` for simple identifiers~~
- [x] ~~**2.3.3** Implement `resolve_type_name()` for simple type names~~
- [x] ~~**2.3.4** Add basic tests for name resolution~~
- [ ] **2.3.5** Extend `resolve_var_name()` for qualified names (multi-segment paths)
  - Parse path segments (e.g., `Geometry.Point.x`)
  - Resolve module path prefix
  - Lookup symbol in target module's scope
- [ ] **2.3.6** Extend `resolve_type_name()` for qualified type names
  - Support paths like `Std.Collections.Vec<T>`
  - Handle generic type parameters
- [ ] **2.3.7** Implement `resolve_module()` for full AST traversal
  - Visit all expressions and resolve variable references
  - Visit all type annotations and resolve type names
  - Visit all statements (let bindings, assignments, returns)
  - Visit all function definitions and register symbols
- [ ] **2.3.8** Add comprehensive tests
  - Test qualified variable names across modules
  - Test qualified type names
  - Test nested field access
  - Test error cases (undefined qualified names)

**Resolution Rules (continued):**

- **Variables/Functions**: Search scope stack (local → module → imports)
- **Types**: Search type scope (struct/enum/trait names)
- **Module-qualified names**: `Geometry.Point` → resolve to imported symbol

**Error Cases:**

- Undefined variable: `let x = unknown_var;`
- Undefined type: `let p: UnknownType = ...;`
- Ambiguous import: Two modules export same name (require qualification)
- [ ] Add error diagnostics for unresolved names
- [ ] Add tests for name resolution

---

## Phase 3: Type Checking

**Goal**: Verify all expressions have valid types, enforce type safety

### 3.1 Expression Type Inference

**Type Checker Core:**

```cpp
class Type_Checker {
  Symbol_Table& symtab;

  // Infer type of expression
  auto check_expr(ast::Expr const& expr) -> Result<Type>;

  // Check statement validity
  auto check_statement(ast::Statement const& stmt) -> Result<void>;

  // Check function body against signature
  auto check_func_def(ast::Func_Def const& def) -> Result<void>;
};
```

**Type Inference Rules:**

| Expression | Type Inference Rule |
|------------|-------------------|
| Integer literal | `I32` (default) or inferred from context |
| Float literal | `F64` (default) or inferred from context |
| String literal | `String` |
| Bool literal | `Bool` |
| Variable | Lookup symbol, return its type |
| Binary expr | Check operand types, infer result type |
| Function call | Check argument types vs parameters, return function return type |
| Field access | Check struct type, return field type |
| If expr | Check condition is `Bool`, ensure all branches have same type |
| Match expr | Check pattern types, ensure all arms have same result type |
| Cast expr | Validate conversion is legal (`as` operator) |

**Example Flow:**

```rust
// Source code
fn add(x: I32, y: I32): I32 { return x + y; }

// Type checking steps:
// 1. Register function signature: add: fn(I32, I32): I32
// 2. Check function body:
//    - x: I32 (from parameter)
//    - y: I32 (from parameter)
//    - x + y: Check binary expr
//      - Left: I32
//      - Right: I32
//      - Op: + (valid for I32)
//      - Result: I32
//    - return x + y: Check return type matches signature
//      - Expression type: I32
//      - Expected: I32
//      - ✅ OK
```

**Tasks:**

- [ ] Implement expression type inference for all expr variants
- [ ] Implement binary operator type checking (+, -, *, /, ==, <, etc.)
- [ ] Implement function call type checking (args vs params)
- [ ] Implement field access type checking
- [ ] Implement if/match expression type checking (branch consistency)
- [ ] Implement cast expression validation
- [ ] Check return statement types vs function signature
- [ ] Add comprehensive error messages with type mismatches
- [ ] Add tests for type inference

### 3.2 Type Compatibility & Coercion

**Type Compatibility Rules:**

- Exact match: `I32` == `I32`
- Subtyping (future): For trait objects, enums
- No implicit coercion (except integer literals in limited contexts)

**Cast Validation:**

```cpp
auto is_valid_cast(Type from, Type to) -> bool {
  // Numeric conversions: I32 → I64, F32 → F64, I32 → F64
  // Pointer/reference conversions (future)
  // Enum → discriminant (future)
}
```

**Tasks:**

- [ ] Implement type equality checking
- [ ] Implement cast validation for primitive types
- [ ] Add error reporting for invalid casts
- [ ] Add tests for cast operations

### 3.3 Pattern Type Checking

**Pattern Matching Type Safety:**

```cpp
auto check_pattern(ast::Pattern const& pattern, Type expected) -> Result<void> {
  // Ensure pattern can match the expected type
  // Bind pattern variables with correct types
}
```

**Rules:**

- Simple pattern (`x`): Bind variable with `expected` type
- Tuple pattern: Check tuple arity and element types
- Struct pattern: Check struct name and field types
- Enum pattern: Check variant exists and argument types match
- Or pattern: All alternatives must bind same variables with same types

**Tasks:**

- [ ] Implement pattern type checking for all pattern kinds
- [ ] Bind pattern variables in symbol table
- [ ] Validate or-pattern consistency
- [ ] Add tests for pattern matching

---

## Phase 4: Struct, Enum, and Trait Semantics

**Goal**: Validate user-defined types and trait implementations

### 4.1 Struct Validation

**Checks:**

- No duplicate field names
- Field types are valid (all types are defined)
- Struct name follows naming convention (`Camel_Snake_Case`)
- Generic parameters are used consistently

**Struct Literal Validation:**

```rust
// Definition
struct Point { x: I32, y: I32 }

// Usage
let p = Point { x: 1, y: 2 };  // ✅ OK
let q = Point { x: 1 };        // ❌ Missing field 'y'
let r = Point { x: 1, y: 2, z: 3 };  // ❌ Unknown field 'z'
```

**Tasks:**

- [ ] Check struct field uniqueness
- [ ] Validate field types are defined
- [ ] Check struct literal completeness (all fields present)
- [ ] Check struct literal field types match definition
- [ ] Add tests for struct validation

### 4.2 Enum Validation

**Checks:**

- No duplicate variant names
- Variant field types are valid
- Enum name follows naming convention

**Enum Pattern Validation:**

```rust
enum Option<T> { Some(T), None }

match opt {
  Option.Some(x) => x,  // ✅ OK
  Option.None => 0,     // ✅ OK
  Option.Unknown => 0,  // ❌ Unknown variant
}
```

**Tasks:**

- [ ] Check enum variant uniqueness
- [ ] Validate variant field types
- [ ] Check enum patterns reference valid variants
- [ ] Check enum constructor calls match variant signatures
- [ ] Add tests for enum validation

### 4.3 Trait & Impl Block Validation

**Trait Definition Checks:**

- Associated type names are unique
- Method signatures are valid
- Trait bounds are valid types

**Impl Block Checks:**

- Impl target type exists
- No duplicate methods
- Method signatures match trait requirements (for trait impls)
- `self` parameter only appears in impl blocks (not free functions)

**Trait Impl Validation:**

```rust
trait Display { fn show(self): String; }

impl Display for Point {
  fn show(self): String { return "Point"; }  // ✅ OK
}

impl Display for Circle {
  fn show(self): I32 { return 0; }  // ❌ Wrong return type (expected String)
}
```

**Tasks:**

- [ ] Validate trait method signatures
- [ ] Check impl block method uniqueness
- [ ] Validate trait impl completeness (all required methods)
- [ ] Check trait impl method signatures match trait
- [ ] Validate `self` parameter only in impl blocks
- [ ] Add tests for trait/impl validation

---

## Phase 5: Visibility and Access Control

**Goal**: Enforce `pub` visibility rules and prevent visibility leaks

### 5.1 Visibility Checking

**Rules:**

1. **Module-internal items** (no `pub`): Only visible within same module
2. **Public items** (`pub`): Exported from module, importable
3. **Visibility leak**: Public item cannot reference private types in its public interface

**Visibility Leak Examples:**

```rust
// geometry/point.life
struct Internal_Helper { x: I32 }  // Not pub

pub struct Point {
  pub data: Internal_Helper  // ❌ LEAK: pub field with private type
}

pub fn create(): Internal_Helper { /* ... */ }  // ❌ LEAK: pub function returns private type
```

**Checks:**

- Public struct fields must use public types
- Public function parameters/returns must use public types
- Public methods must use public types in signatures
- Impl blocks for public types don't leak private methods to public API

**Tasks:**

- [ ] Implement visibility checking for struct fields
- [ ] Implement visibility checking for function signatures
- [ ] Detect visibility leaks in public APIs
- [ ] Add error diagnostics with clear explanations
- [ ] Add tests for visibility enforcement

### 5.2 Access Control

**Field Access Validation:**

```rust
// In module Geometry
struct Point {
  pub x: I32,
  y: I32  // Private
}

// In another module
import Geometry.{Point};
let p = Point { x: 1, y: 2 };  // ❌ Can't access private field 'y'
let x_val = p.x;  // ✅ OK - public field
let y_val = p.y;  // ❌ Can't access private field 'y'
```

**Tasks:**

- [ ] Check field access respects visibility
- [ ] Check method call respects visibility
- [ ] Add tests for access control

---

## Phase 6: Generics and Trait Bounds (Future)

**Goal**: Type-check generic code with trait constraints

**Key Challenges:**

- Monomorphization vs type erasure
- Trait bound checking
- Associated type resolution
- Where clause validation

**Deferred to Future Milestone** - Focus on non-generic code first.

---

## Phase 7: Error Reporting

**Goal**: Provide high-quality, actionable error messages

### 7.1 Error Message Design

**Format** (inspired by Rust/Clang):

```
error: type mismatch in return statement
  --> src/main.life:5:10
   |
 5 |   return "hello";
   |          ^^^^^^^ expected `I32`, found `String`
   |
note: function signature declared here
  --> src/main.life:3:1
   |
 3 | fn add(x: I32, y: I32): I32 {
   | ^^^^^^^^^^^^^^^^^^^^^^^^^^^ expected return type `I32`
```

**Components:**

- Error level (error, warning, note)
- Primary message
- Source location with snippet
- Inline annotations (^^^)
- Additional context (note/help)

**Leverage Existing Infrastructure:**

- Use `diagnostics.cpp/hpp` from parser phase
- Extend with semantic-specific error types

**Tasks:**

- [ ] Define semantic error types (type mismatch, undefined symbol, visibility violation)
- [ ] Implement error formatting with source snippets
- [ ] Add multi-location diagnostics (definition + usage)
- [ ] Add suggestions where applicable ("did you mean X?")
- [ ] Add tests for error message output

### 7.2 Error Recovery

**Strategy**: Continue analysis after errors to report multiple issues

**Approaches:**

- On type mismatch: Assume error type, continue checking
- On undefined symbol: Insert placeholder, continue
- Track error count, abort if too many (prevent cascading errors)

**Tasks:**

- [ ] Implement error recovery in type checker
- [ ] Prevent error cascades (one root error causing many secondary errors)
- [ ] Add tests for multi-error reporting

---

## Phase 8: Integration & Testing

**Goal**: End-to-end semantic analysis pipeline

### 8.1 Semantic Analyzer Pipeline

```cpp
class Semantic_Analyzer {
  Module_Loader loader;
  Module_Registry registry;
  Symbol_Table symtab;
  Name_Resolver name_resolver;
  Type_Checker type_checker;

  auto analyze_project(fs::path root) -> Result<Analyzed_Project> {
    // 1. Discover modules
    auto modules = loader.discover_modules(root);

    // 2. Parse all modules
    for (auto& mod : modules) {
      auto ast = loader.load_module(mod);
      registry.register_module(mod.path, ast);
    }

    // 3. Resolve imports (build dependency graph)
    for (auto& [path, info] : registry.modules) {
      import_resolver.resolve_imports(info.ast, path);
    }

    // 4. Check for circular dependencies
    auto order = registry.build_dependency_graph();

    // 5. Analyze modules in dependency order
    for (auto& path : order) {
      auto& mod = registry.get_module(path);
      name_resolver.resolve_module(mod.ast);
      type_checker.check_module(mod.ast);
    }

    return Analyzed_Project{registry, symtab};
  }
};

struct Analyzed_Project {
  Module_Registry registry;
  Symbol_Table symtab;
  // Ready for code generation
};
```

### 8.2 Test Strategy

**Unit Tests** (per component):

- Symbol table operations
- Type inference for each expression type
- Import resolution
- Visibility checking

**Integration Tests** (full analysis):

- Multi-module projects with imports
- Complex type checking scenarios
- Error reporting for common mistakes
- Generic code (when implemented)

**Test Organization:**

```
tests/
  semantic/
    test_symbol_table.cpp
    test_type_checker.cpp
    test_name_resolver.cpp
    test_import_resolver.cpp
    test_visibility.cpp
    integration/
      test_multi_module_project.cpp
      test_type_checking_integration.cpp
      test_error_reporting.cpp
```

**Tasks:**

- [ ] Add unit tests for each semantic analysis component
- [ ] Add integration tests for multi-module projects
- [ ] Add tests for error cases with expected diagnostics
- [ ] Add golden file tests for error message formatting

---

## Implementation Order (Recommended)

**Priority 1 - Core Foundation** (Weeks 1-2):

1. Symbol table + type representation
2. Builtin types registry
3. Module registry structure

**Priority 2 - Single Module Analysis** (Weeks 3-4):
4. Name resolution (single module, no imports)
5. Expression type checking (literals, binary ops, function calls)
6. Statement type checking (let, return, if, match)

**Priority 3 - Module System** (Weeks 5-6):
7. Module discovery and loading
8. Import resolution
9. Multi-module name resolution

**Priority 4 - Advanced Features** (Weeks 7-8):
10. Struct/enum validation
11. Trait and impl block checking
12. Visibility enforcement

**Priority 5 - Polish** (Weeks 9-10):
13. Comprehensive error reporting
14. Error recovery
15. Integration testing and debugging

---

## Success Criteria

**Milestone 1: Single Module Type Checking**

- [ ] Parse module, build symbol table
- [ ] Resolve all variable/function names
- [ ] Type-check expressions (literals, binary ops, calls)
- [ ] Type-check statements (let, return, if)
- [ ] Report undefined symbols and type mismatches

**Milestone 2: Multi-Module Support**

- [ ] Discover modules from filesystem
- [ ] Resolve imports across modules
- [ ] Check import visibility (pub enforcement)
- [ ] Detect circular dependencies

**Milestone 3: Full Semantic Validation**

- [ ] Validate structs, enums, traits, impls
- [ ] Enforce visibility rules (no leaks)
- [ ] Check trait implementations match signatures
- [ ] Comprehensive error reporting

**Milestone 4: Production Ready**

- [ ] Error recovery (report multiple errors)
- [ ] Performance optimization (large codebases)
- [ ] Full integration test suite
- [ ] Documentation and examples

---

## Open Design Questions

1. **Generics Strategy**: Monomorphization vs type erasure?
   - Leaning toward monomorphization (C++ style) for performance
   - Deferred until non-generic code works

2. **Trait Object Representation**: How to implement dynamic dispatch?
   - Future phase, not MVP

3. **Reference Counting Integration**: When to insert RC operations?
   - Code generation phase, not semantic analysis
   - Semantic analysis just tracks value vs heap types

4. **Module Compilation Units**: One binary per module or whole-program?
   - Initially: whole-program compilation
   - Future: incremental compilation per module

---

## Resources & References

**Existing Code to Study:**

- `src/diagnostics.cpp/hpp` - Error reporting infrastructure
- `src/parser.cpp` - AST traversal patterns
- `src/ast.hpp` - Complete AST node definitions

**Design Documents:**

- `doc/MODULE_SYSTEM.md` - Module system specification
- `doc/GRAMMAR.md` - Language syntax (for understanding semantics)
- `doc/MEMORY_MODEL.md` - Value semantics and mutation

**Related Compiler Resources:**

- Rust compiler (rustc) - Similar module and type system
- Clang diagnostics - Excellent error message design
- Modern Compiler Implementation in C++ (Appel) - Symbol table design

---

## Next Steps

1. **Review this roadmap** - Validate design decisions
2. **Create initial file structure** - `src/semantic/` directory
3. **Start with Phase 1** - Symbol table foundation
4. **Iterate incrementally** - Each phase builds on previous

This is a living document - update as design decisions solidify during implementation.
