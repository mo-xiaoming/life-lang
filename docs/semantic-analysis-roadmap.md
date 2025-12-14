# Semantic Analysis Roadmap

This document outlines the phased approach to implementing semantic analysis for life-lang.

## Overview

Semantic analysis occurs after parsing and validates that the program is semantically correct. It operates on the AST produced by the parser and performs:

1. **Name Resolution**: Ensures all referenced names are declared and in scope
2. **Type Checking**: Verifies type correctness of expressions and statements
3. **Trait Resolution**: Validates trait bounds and implementations

## Phase 1: Symbol Table & Name Resolution

**Goal**: Build infrastructure to track declarations and resolve name references.

### Step 1.1: Symbol Table Infrastructure

**Files to Create**:
- `src/symbol_table.hpp`
- `src/symbol_table.cpp`

**Core Data Structures**:

```cpp
// Represents a single declared symbol
struct Symbol {
  std::string name;
  Symbol_Kind kind;        // Variable, Function, Type, Trait, etc.
  Type_Info type;          // Type information (TBD in Phase 2)
  Source_Location location; // For error reporting
  // Future: mutability, visibility, generic params
};

// Represents a lexical scope
struct Scope {
  Scope* parent;                                    // Outer scope (nullptr for global)
  std::unordered_map<std::string, Symbol> symbols;  // Declarations in this scope
  Scope_Kind kind;                                  // Function, Block, Impl, etc.
};

// Manages the scope stack during analysis
class Symbol_Table {
  std::vector<Scope*> scope_stack;  // Current scope chain
  
  auto enter_scope(Scope_Kind) -> void;
  auto exit_scope() -> void;
  auto insert(Symbol) -> bool;      // Returns false if duplicate
  auto lookup(std::string const&) -> Symbol*;  // Search up scope chain
};
```

**Test Cases**:
- Basic scope creation/destruction
- Symbol insertion and lookup
- Shadowing variables in nested scopes

---

### Step 1.2: Declaration Collection (First Pass)

**Files to Create**:
- `src/semantic_analyzer.hpp`
- `src/semantic_analyzer.cpp`

**Purpose**: Walk the AST and populate the symbol table with all declarations before validating references.

**What to Collect**:

1. **Top-Level Declarations**:
   - Function definitions (`Func_Def`) - name, parameters, return type
   - Struct definitions (`Struct_Def`) - name, fields, generic params
   - Enum definitions (`Enum_Def`) - name, variants, generic params
   - Trait definitions (`Trait_Def`) - name, methods, generic params
   - Type aliases (`Type_Alias`) - name, aliased type
   - Impl blocks (`Impl_Block`, `Trait_Impl`) - methods

2. **Nested Declarations**:
   - Function parameters
   - Let bindings in blocks
   - Pattern bindings (match arms, for loops)

**Implementation Pattern**:
```cpp
class Semantic_Analyzer {
  Symbol_Table symbol_table;
  Diagnostic_Engine& diagnostics;
  
  auto collect_declarations(ast::Module const&) -> void;
  auto collect_func_def(ast::Func_Def const&) -> void;
  auto collect_struct_def(ast::Struct_Def const&) -> void;
  // ... for each top-level construct
};
```

**Error Detection**:
- Duplicate declarations in same scope
- Invalid naming conventions (deferred from parser):
  - Types/Traits/Modules: `Camel_Snake_Case`
  - Variables/Functions/Fields: `snake_case`

**Test Cases**:
- Collect multiple function definitions
- Detect duplicate function names
- Detect duplicate struct names
- Report naming convention violations

---

### Step 1.3: Name Resolution (Second Pass)

**Purpose**: Resolve all name references and verify they exist in scope.

**What to Resolve**:

1. **Type Names** (`Type_Name`):
   - Function parameter types
   - Return types
   - Struct field types
   - Trait bounds
   - Type alias targets

2. **Variable Names** (`Var_Name`):
   - Variable references in expressions
   - Function calls
   - Field access expressions

**Special Cases**:

1. **`self` Parameter**:
   - Only valid in impl block methods (currently parser allows it anywhere - see `rules.cpp:385`)
   - Resolve to the type being impl'd

2. **Generic Parameters**:
   - Create scope for generic params in functions/structs/traits
   - Resolve type params to their declarations

3. **Path Resolution**:
   - Multi-segment paths: `Std.Math.sqrt`, `Result.Ok`
   - Module resolution (future)

**Implementation**:
```cpp
class Semantic_Analyzer {
  auto resolve_names(ast::Module const&) -> void;
  auto resolve_type_name(ast::Type_Name const&) -> Symbol*;
  auto resolve_var_name(ast::Var_Name const&) -> Symbol*;
  auto resolve_expr(ast::Expr const&) -> void;  // Recursively resolve in expressions
};
```

**Error Detection**:
- Undefined variable/type references
- `self` used outside impl block methods
- Accessing undeclared struct fields
- Using type names as values (or vice versa)

**Test Cases**:
- Resolve variable in same scope
- Resolve variable in outer scope
- Report undefined variable error
- Report `self` outside impl block
- Resolve generic type parameters
- Resolve multi-segment paths

**Validation**: After this step, every `Type_Name` and `Var_Name` in the AST should have a corresponding symbol or an error should be reported.

---

## Phase 2: Type System

**Goal**: Implement type representation and type checking.

### Step 2.1: Type Representation

**Files to Create**:
- `src/type_system.hpp`
- `src/type_system.cpp`

**Core Types**:

```cpp
// Internal representation of types (distinct from AST Type_Name)
struct Type {
  enum class Kind {
    Primitive,  // I32, F64, Bool, Char, String, Unit
    Struct,
    Enum,
    Tuple,
    Array,
    Function,
    Generic,    // Uninstantiated generic T
    Concrete,   // Instantiated generic Vec<I32>
  };
  Kind kind;
  // ... type-specific data
};

// Maps AST types to internal type representations
class Type_Checker {
  auto resolve_type(ast::Type_Name const&, Symbol_Table const&) -> Type;
  auto instantiate_generic(Type const& generic, std::vector<Type> const& args) -> Type;
};
```

**Primitive Types**:
- `I32`, `I64`, `U32`, `U64`
- `F32`, `F64`
- `Bool`, `Char`, `String`
- `()` (unit type)

**Test Cases**:
- Resolve primitive types
- Build struct types from definitions
- Build enum types from definitions
- Represent tuple types

---

### Step 2.2: Type Inference & Checking

**Purpose**: Compute types of expressions and verify type correctness.

**What to Check**:

1. **Expression Types**:
   - Literals: `42` → `I32`, `"hello"` → `String`
   - Binary operations: `I32 + I32` → `I32`, `String + I32` → **error**
   - Function calls: argument types match parameters
   - If expressions: both branches have same type
   - Match expressions: all arms have same type
   - Block expressions: type of final expression (or `()` if none)

2. **Statement Type Rules**:
   - Let bindings: infer type from initializer or check against annotation
   - Return statements: type matches function return type
   - Assignment: RHS type matches LHS type

3. **Special Checks**:
   - **Assignment mutability** (deferred from parser - see `ast.hpp:329`):
     - Currently all bindings are immutable
     - Future: add `mut` keyword, check assignment targets are mutable
   - **Pattern exhaustiveness**: Match covers all enum variants

**Implementation**:
```cpp
class Type_Checker {
  auto infer_expr_type(ast::Expr const&, Symbol_Table const&) -> Type;
  auto check_expr_type(ast::Expr const&, Type const& expected, Symbol_Table const&) -> void;
  auto check_function_body(ast::Func_Def const&, Symbol_Table const&) -> void;
  
  // Type compatibility
  auto types_compatible(Type const& lhs, Type const& rhs) -> bool;
  auto can_coerce(Type const& from, Type const& to) -> bool;  // For numeric widening
};
```

**Error Detection**:
- Type mismatch in binary operations
- Type mismatch in function call arguments
- Type mismatch in if/match branches
- Return type doesn't match declaration
- Assignment to immutable binding
- Non-exhaustive match patterns

**Test Cases**:
- Infer literal types
- Check binary operation types
- Check function call argument types
- Detect type mismatch in if branches
- Detect type mismatch in match arms
- Detect return type mismatch
- Detect non-exhaustive patterns (enum without wildcard)

---

## Phase 3: Trait Resolution

**Goal**: Validate trait system correctness.

### Step 3.1: Trait Bound Checking

**Purpose**: Verify that generic type parameters satisfy their trait bounds.

**What to Check**:

1. **Trait Bounds in Declarations**:
   ```rust
   fn process<T: Display>(value: T): String { ... }
   //           ^^^^^^^^ Check T satisfies Display
   ```

2. **Where Clauses**:
   ```rust
   trait Processor<T> 
   where T: Display + Clone { ... }
   //    ^^^^^^^^^^^^^^^^^^^ Check all bounds
   ```

3. **Trait Bounds in Implementations**:
   ```rust
   impl<T: Display> Container<T> { ... }
   //     ^^^^^^^^ Check constraint
   ```

**Implementation**:
```cpp
class Trait_Checker {
  auto check_trait_bounds(std::vector<ast::Type_Param> const&) -> void;
  auto check_where_clause(ast::Where_Clause const&) -> void;
  auto type_satisfies_bound(Type const&, ast::Trait_Bound const&) -> bool;
};
```

**Test Cases**:
- Validate simple trait bounds
- Validate multiple bounds (`T: Display + Clone`)
- Validate where clause constraints
- Detect missing trait implementations

---

### Step 3.2: Trait Implementation Validation

**Purpose**: Ensure trait implementations match trait declarations.

**What to Validate**:

1. **Method Signatures Match**:
   ```rust
   trait Display {
     fn to_string(self): String;
   }
   
   impl Display for Point {
     fn to_string(self): String { ... }  // ✓ Matches
     fn to_string(self): I32 { ... }     // ✗ Wrong return type
   }
   ```

2. **All Required Methods Implemented**:
   - Detect missing methods
   - Detect extra methods not in trait

3. **Generic Constraints Satisfied**:
   ```rust
   impl<T: Display> Container<T> { ... }
   // Verify T actually satisfies Display when used
   ```

**Implementation**:
```cpp
class Trait_Checker {
  auto check_trait_impl(ast::Trait_Impl const&, Symbol_Table const&) -> void;
  auto verify_method_signature(ast::Func_Def const& impl_method, 
                                ast::Func_Def const& trait_method) -> void;
  auto check_all_methods_present(ast::Trait_Impl const&) -> void;
};
```

**Error Detection**:
- Method signature mismatch (parameters, return type)
- Missing required methods
- Methods not in trait definition
- Generic constraint violations

**Test Cases**:
- Valid trait implementation
- Detect method signature mismatch
- Detect missing required method
- Detect extra methods not in trait

---

### Step 3.3: Method Resolution

**Purpose**: Resolve method calls through impl blocks.

**What to Resolve**:

1. **Direct Method Calls**:
   ```rust
   point.distance()  // Find `distance` in impl blocks for Point
   ```

2. **Trait Method Calls**:
   ```rust
   point.to_string()  // Find via `impl Display for Point`
   ```

3. **Generic Method Calls**:
   ```rust
   fn print<T: Display>(val: T) {
     val.to_string()  // Resolve via trait bound
   }
   ```

**Implementation**:
```cpp
class Method_Resolver {
  auto resolve_method(Type const& receiver_type, 
                      std::string const& method_name,
                      Symbol_Table const&) -> Symbol*;
  auto find_in_impl_blocks(Type const&, std::string const&) -> Symbol*;
  auto find_in_trait_impls(Type const&, std::string const&) -> Symbol*;
};
```

**Test Cases**:
- Resolve method from impl block
- Resolve method from trait impl
- Resolve method through generic bound
- Detect method not found
- Detect ambiguous method (multiple impls - future)

---

## Phase 4: Integration & Testing

### Step 4.1: Integrate with Main Compiler Pipeline

**Current Flow**:
```
Source → Parser → AST → JSON Output
```

**New Flow**:
```
Source → Parser → AST → Semantic Analysis → Typed AST → JSON Output
                              ↓
                         Error Reporting
```

**Changes Needed**:
- Update `main.cpp` to run semantic analysis
- Modify AST to optionally store resolved types/symbols
- Extend JSON output with semantic information

---

### Step 4.2: Comprehensive Testing

**Test Structure**:
```
tests/
  semantic/
    test_symbol_table.cpp           # Unit tests for symbol table
    test_name_resolution.cpp        # Name resolution tests
    test_type_checking.cpp          # Type inference/checking tests
    test_trait_resolution.cpp       # Trait system tests
    integration/
      test_valid_programs.cpp       # End-to-end valid programs
      test_semantic_errors.cpp      # End-to-end error detection
```

**Coverage Goals**:
- 100% of error message code paths
- All combinations of valid language constructs
- Edge cases: shadowing, generics, nested scopes

---

## Implementation Order Summary

1. **Phase 1.1**: Symbol table infrastructure (1-2 days)
2. **Phase 1.2**: Declaration collection (2-3 days)
3. **Phase 1.3**: Name resolution (3-4 days)
4. **Phase 2.1**: Type representation (2-3 days)
5. **Phase 2.2**: Type checking (5-7 days)
6. **Phase 3.1**: Trait bound checking (2-3 days)
7. **Phase 3.2**: Trait implementation validation (3-4 days)
8. **Phase 3.3**: Method resolution (2-3 days)
9. **Phase 4**: Integration & comprehensive testing (3-5 days)

**Total Estimated Time**: 23-38 days (depending on complexity encountered)

---

## Dependencies on Future Work

Some semantic checks are deferred pending language design decisions:

1. **Mutability System**: Currently all bindings are immutable
   - Need `mut` keyword design
   - Assignment validation (`ast.hpp:329`)

2. **Module System**: Path resolution currently simplified
   - Need module import/export semantics
   - Multi-file compilation

3. **Borrow Checking**: Not yet designed
   - Lifetime analysis
   - Ownership rules

4. **Advanced Type Features**:
   - Associated types in traits
   - Higher-ranked trait bounds
   - Type inference improvements

---

## Success Criteria

Semantic analysis phase is complete when:

✅ All names resolve to declarations or produce clear errors  
✅ All expressions have inferred types  
✅ Type mismatches are detected and reported  
✅ Trait bounds are validated  
✅ Trait implementations match declarations  
✅ Method calls resolve correctly  
✅ Comprehensive test suite passes  
✅ Integration with main compiler pipeline works  
✅ Error messages are clear and actionable  

This enables the next phase: **Code Generation** or **IR lowering**.
