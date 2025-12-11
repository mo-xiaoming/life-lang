# life-lang Feature Roadmap

## Current State (December 2025)

### Implemented Features ✅
- Value semantics & immutability by default
- Structs with named fields and struct literals
- ~~Functions with UFCS (Uniform Function Call Syntax)~~ **DECISION: Removing UFCS** - methods only via `impl` blocks
- Pattern matching infrastructure (match expressions, patterns)
- Control flow (if, while, for expressions)
- Basic expressions and operators (binary, unary, assignment)
- Type annotations with template/generic syntax parsing
- Field access and method chaining
- Break/continue statements
- Comments
- Literals (integer, float, string, char)
- **Enums/Sum Types**: Unit, tuple, and struct variants with generics (Dec 2025)
- **Unit type `()`**: For functions with no return value (Dec 2025)
- **Impl blocks**: Basic parsing for `impl Type { methods }` (Dec 2025)

### Partially Implemented
- **Template/Generic types**: Syntax parsing exists (`Array<Int>`, `Map<String, Int>`) but no semantic analysis or type checking yet

## Priority 1: Essential Foundation (Q1 2026)

### ~~1.1 Enums/Sum Types~~ ✅ COMPLETED (Dec 2025)
**Status**: ✅ Implemented

Algebraic data types essential for error handling and type-safe variants.

**Syntax**:
```rust
enum Option<T> {
    Some(T),
    None
}

enum Result<T, E> {
    Ok(T),
    Err(E)
}

enum Color {
    Red,
    Green,
    Blue,
    Rgb(I32, I32, I32)
}
```

**Implemented**:
- ✅ Enum declaration parsing (`enum Name { variants }`)
- ✅ Variant types: unit variants, tuple variants, struct variants
- ✅ Generic enum support (`<T>`, `<T, E>`)
- ✅ Pattern matching integration (already have patterns, enum support added)
- ✅ AST nodes: `Enum_Definition`, `Enum_Variant` with explicit `Variant_Kind` discriminator
- ✅ JSON serialization for enums
- ✅ 17 comprehensive test cases covering all variant types

**Rationale**: Pattern matching exists but has nothing meaningful to match on. Enums complete the ADT story and enable proper error handling without exceptions.

### ✅ 1.2 Remove UFCS Functions (Completed Dec 2025)

**Decision**: ✅ Removed standalone UFCS functions in favor of coherent design.

**Language Design Philosophy**:
- **Methods**: Only inside `impl Type` blocks
  - Scoped to type: `Type::method`
  - Callable with dot syntax: `value.method()`
  - Must be defined in same module as type (single impl per type initially)
  
- **Free functions**: Everything else
  - Global scope (or module-scoped)
  - Called with function syntax: `function_name(value)`
  - Can be defined anywhere
  
- **Extension via traits** (future): `impl Trait for Type` allows controlled extension

**Tasks**:
1. ✅ Remove `self` as special keyword for UFCS
2. ✅ Parser allows `self` parameter but semantic validation will enforce impl-only rule
3. ✅ Removed UFCS test file (`test_ufcs_function.cpp`)
4. ✅ Updated documentation and copilot instructions
5. ✅ Reserved `self` keyword for impl blocks (semantic validation pending)

**Rationale**: 
- Eliminates "two ways to do the same thing" confusion
- Clearer scoping and ownership semantics
- Prepares clean foundation for trait system
- No need to distinguish UFCS vs impl methods - only impl methods exist

### 1.3 Complete Impl Blocks (HIGH - 1 week)

**Status**: Basic parsing exists, needs semantic analysis and single-impl-per-type enforcement.

**Syntax**:
```rust
impl Point {
    fn distance(self): F64 { ... }  // self type optional, inferred as Point
    fn translate(self, dx: I32, dy: I32): Point { ... }
}

impl<T> Array<T> {
    fn len(self): I32 { ... }  // self type inferred as Array<T>
    fn get(self, idx: I32): Option<T> { ... }
}
```

**Requirements**:
- ✅ Basic `impl Type { methods }` parsing (done)
- ⏳ Generic impl blocks (`impl<T> Type<T>`)
- ⏳ Enforce single impl block per type per module
- ⏳ Link methods to types in semantic analysis
- ⏳ Method resolution for dot syntax (`value.method()`)
- ⏳ Qualified calls (`Type::method(value)`)

**Rationale**: Provides organized method grouping and prepares infrastructure for trait implementations.

## Priority 2: Trait System (Q1-Q2 2026)

### 2.1 Trait Declarations (HIGH - 2-3 weeks)

**Decision**: Use Rust-style traits (NOT Go interfaces or C++ concepts)

**Why Rust Traits**:
- ✅ Perfect fit for value semantics
- ✅ Explicit implementations (no implicit satisfaction confusion)
- ✅ Compile-time monomorphization (zero-cost abstraction)
- ✅ Extension traits (add methods to existing types)
- ✅ Coherence rules (prevent implementation conflicts)
- ✅ Associated types and default methods
- ✅ Proven design in production language

**Why NOT Go Interfaces**:
- ❌ Implicit satisfaction is confusing
- ❌ Designed for pointer-based types (poor fit for value semantics)
- ❌ Runtime dispatch overhead (vtables)
- ❌ No extension methods

**Why NOT C++ Concepts**:
- ❌ Purely compile-time constraints (no implementations)
- ❌ No default methods or code reuse
- ❌ Too complex for bootstrapping compiler
- ❌ No polymorphism patterns

**Syntax**:
```rust
trait Display {
    fn to_string(self): String;
}

trait Iterator<T> {
    fn next(mut self): Option<T>;
    fn has_next(self): Bool;
}

trait Add<T> {
    fn add(self, other: T): T;
}
```

**Requirements**:
- Trait declaration parsing (`trait Name { methods }`)
- Generic traits (`trait Iterator<T>`)
- Method signatures in traits
- Associated types (future extension)
- Default method implementations (future extension)

### 2.2 Trait Implementations (HIGH - 1-2 weeks)

**Syntax**:
```rust
impl Display for Point {
    fn to_string(self): String {
        return format("Point({}, {})", self.x, self.y);
    }
}

impl<T> Display for Array<T> where T: Display {
    fn to_string(self): String { ... }
}
```

**Requirements**:
- `impl Trait for Type { methods }` parsing
- Generic trait implementations
- Orphan rules enforcement (coherence)
- Method dispatch resolution

### 2.3 Trait Bounds (HIGH - 1-2 weeks)

**Syntax**:
```rust
fn print<T: Display>(item: T): I32 {
    Std.print(item.to_string());
    return 0;
}

fn compare<T: Eq + Ord>(a: T, b: T): Bool {
    return a == b;
}

// Where clause syntax
fn process<T>(items: Array<T>): I32 
where T: Display + Clone {
    ...
}
```

**Requirements**:
- Trait bound syntax parsing (`: Trait`, `+ Trait`)
- Where clause support
- Bound checking in semantic analysis
- Error messages for missing trait implementations

## Priority 3: Type System Completion (Q2 2026)

### 3.1 Generic Implementation (MEDIUM - 2-3 weeks)

Complete the generic system beyond parsing.

**Requirements**:
- Type parameter declaration and scoping
- Generic function instantiation (monomorphization)
- Generic struct instantiation
- Type inference for generic parameters
- Constraint checking (trait bounds)

**Rationale**: Template syntax is parsed but not semantically analyzed. This blocks standard library development.

### 3.2 Type Inference (MEDIUM - 2-3 weeks)

**Syntax**:
```rust
let x = 42;              // Infer I32
let items = vec![1, 2];  // Infer Array<I32>
```

**Requirements**:
- Local type inference (Hindley-Milner subset)
- Type unification algorithm
- Inference across function boundaries
- Error messages for ambiguous types

## Priority 4: Module System (Q2-Q3 2026)

### 4.1 Module Definitions (MEDIUM - 1-2 weeks)

Currently have qualified paths (`Std.IO.print`) but no module syntax.

**Syntax**:
```rust
mod io {
    pub fn print(s: String): I32 { ... }
    fn internal_helper(): I32 { ... }  // private
}

mod collections {
    pub struct Array<T> { ... }
    pub trait Iterator<T> { ... }
}
```

**Requirements**:
- `mod name { items }` parsing
- Visibility modifiers (`pub`, default private)
- Nested modules
- Module path resolution

### 4.2 Import System (MEDIUM - 1 week)

**Syntax**:
```rust
use Std.IO.print;
use Std.Collections.{Array, HashMap};
use Std.Iter.* as iter;
```

**Requirements**:
- `use` statement parsing
- Glob imports (`*`)
- Aliasing (`as`)
- Name resolution with imports

## Future Priorities (Q3+ 2026)

### Error Handling
- Result/Option integration with `?` operator
- Panic/abort mechanism
- Error trait standard library

### Advanced Traits
- Associated types
- Default method implementations
- Trait objects (dynamic dispatch) - if needed
- Higher-ranked trait bounds (HRTB)

### Standard Library Core
- Collections (Array, HashMap, HashSet)
- Iterators with trait implementations
- String manipulation
- I/O operations
- Format/Display infrastructure

### Language Features
- Closure syntax and capture semantics
- Tuple types (already have tuple patterns)
- Array/slice types
- Reference types (if needed beyond value semantics)

## Explicitly Deferred

### Not in Bootstrap Phase
- **Lifetimes/Ownership**: Conflicts with "value semantics by default" design
- **Async/Await**: Needs runtime infrastructure
- **Macros**: Meta-programming can wait
- **Destructors**: Explicitly deferred to simplify memory model
- **Unsafe blocks**: Not needed for bootstrap
- **Foreign Function Interface (FFI)**: Wait for stable ABI

## Implementation Phases

### ~~Phase 1: Enums~~ ✅ COMPLETED (Dec 2025)
1. ✅ Add AST nodes for enum definitions and variants
2. ✅ Implement enum parsing rules
3. ✅ Add tests for all enum syntax variations
4. ✅ Integrate with pattern matching
5. ✅ JSON serialization

### Phase 2: Remove UFCS & Complete Impl Blocks (Current - Week 1-2)
1. ⏳ Remove UFCS function support (3-4 days)
2. ⏳ Complete impl block semantic analysis (1 week)
3. ⏳ Method resolution for dot syntax
4. ⏳ Tests and integration

### Phase 3: Traits (Week 3-5)
1. Trait declarations
2. Trait implementations
3. Trait bounds
4. Semantic analysis integration

### Phase 4: Generics Semantic Analysis (Week 7-9)
1. Type parameter scoping
2. Monomorphization
3. Constraint checking

### Phase 5: Modules (Week 10-12)
1. Module declarations
2. Visibility
3. Import system

## Success Criteria

### ~~Phase 1 Complete~~ ✅ DONE (Dec 2025)
- ✅ Can parse all enum syntax variations
- ✅ Enums integrate with pattern matching
- ✅ Standard library can define `Option<T>` and `Result<T, E>`
- ✅ Comprehensive test coverage

### Phase 2 Complete When:
- ⏳ UFCS functions removed from language
- ⏳ Only `impl` blocks can define methods
- ⏳ Method call syntax works: `value.method()`
- ⏳ Qualified syntax works: `Type::method(value)`
- ⏳ Single impl block per type enforced

### Phase 3 Complete When:
- ✅ Can define traits with methods
- ✅ Can implement traits for types
- ✅ Can use trait bounds in generic functions
- ✅ Can write `Display`, `Eq`, `Ord` traits for standard types

### Language Ready for Self-Hosting When:
- ✅ All Priority 1-3 features implemented
- ✅ Standard library has core collections and I/O
- ✅ Compiler can parse and analyze its own source
- ✅ Bootstrap compiler can build production compiler

## Notes

- **Test-Driven**: Every feature must have comprehensive parser tests following `tests/parser/utils.hpp` patterns
- **Incremental**: Each feature builds on previous infrastructure
- **Semantic Analysis**: Parser changes are just phase 1; semantic analysis and type checking come after
- **Documentation**: Update `.github/copilot-instructions.md` with new language features as they're added
