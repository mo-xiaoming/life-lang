# Memory Model and Mutation Strategy

**Status**: Design document for life-lang's approach to value semantics and mutation

## Core Principles

1. **Value semantics**: Everything appears as a value to the user
2. **Immutable by default**: Safe, simple, enables easy concurrency
3. **Automatic reference counting**: Compiler manages sharing transparently
4. **Explicit mutation when needed**: Users opt-in to mutation where required

---

## Current Implementation (MVP)

### What We Have Now

```rust
// Immutable by default
let x = 42;
let name = "Alice";
let point = Point { x: 1, y: 2 };

// Value semantics - copies are independent
let y = x;  // y is a copy of x

// Large types automatically ref-counted (transparent to user)
let dept = Department { 
  name: "Engineering", 
  budget: 1_000_000,
  employees: [/* large array */]
};
let alice = Person { dept: dept };  // Shares dept via ref-counting
let bob = Person { dept: dept };    // Shares same dept instance
```

### Memory Strategy

| Type Size | Implementation | User View | Example |
|-----------|---------------|-----------|---------|
| Small (≤64 bytes) | Copy by value | Value | `I32`, `Bool`, `Point { x, y }` |
| Large (>64 bytes) | Automatic ref-counting | Value (shared immutable) | `String`, `[I32]`, large structs |
| Recursive | Automatic ref-counting | Value (shared immutable) | Trees, graphs |

**Key insight**: Users write value semantics, compiler provides efficiency.

---

## Future: Mutation Support

### Phase 1: Mutable Bindings (`mut` keyword)

**Purpose**: Allow rebinding and mutation of local variables

```rust
// Current: Immutable binding (error to reassign)
let counter = 0;
counter = counter + 1;  // ❌ ERROR: counter is immutable

// Future: Mutable binding
let mut counter = 0;
counter = counter + 1;  // ✅ OK

// Mutable function parameters
fn increment(mut value: I32): I32 {
  value = value + 1;
  return value;
}

// Mutable struct fields (future)
struct Counter {
  mut count: I32,
}

let counter = Counter { count: 0 };
counter.count = counter.count + 1;  // ✅ OK if field is mut
```

**Design decision**: `mut` is a property of the **binding**, not the type.

**Use case**: Local mutable state (counters, accumulators, parser state)

---

### Phase 2: Shared Mutable State (Library Types)

**Purpose**: Allow multiple owners to share mutable data

#### Option A: `Cell<T>` - Interior Mutability (Single-threaded)

**Purpose**: Mutate contents even when binding is immutable

```rust
struct Parser {
  source: String,
  position: Cell<I32>,  // Can mutate position field
}

fn parse(parser: Parser) {  // parser parameter is immutable
  parser.position.set(parser.position.get() + 1);  // ✅ Works
}
```

**Semantics**:
- `Cell<T>` is copyable (if T is copyable)
- Each copy has its **own independent** Cell
- Mutations don't affect other copies
- **Key difference from `mut`**: Mutability is in the **type**, not the binding

**Comparison with `mut`**:

```rust
// mut: Mutability is in the BINDING
let mut a = 3;
let b = a;      // b is NOT mutable (just a copy of the value)
a = 5;          // ✅ OK - a is mut
b = 10;         // ❌ ERROR - b is not mut

// Cell<T>: Mutability is in the TYPE
let a = Cell::new(3);
let b = a;      // b IS mutable (Cell is copyable, copy is also Cell)
a.set(5);       // ✅ OK - Cell allows mutation
b.set(10);      // ✅ OK - b is also a Cell
// a = 5, b = 10 (independent copies)
```

**Critical insight**: 
- `mut` is a property of the **binding** (doesn't transfer on copy/assignment)
- `Cell<T>` is a property of the **type** (transfers on copy/assignment)

**Use case**: Interior mutability in immutable structs, caching

#### Option B: `Rc<Cell<T>>` - Shared Mutable State (Single-threaded)

**Purpose**: Multiple owners share the same mutable data

```rust
let dept_data = Rc::new(Cell::new(Department { 
  name: "Engineering", 
  budget: 1_000_000 
}));

let alice = Person { dept: dept_data.clone() };
let bob = Person { dept: dept_data.clone() };

// Update shared department
let old_dept = dept_data.get();
dept_data.set(Department { 
  name: old_dept.name,
  budget: 2_000_000  // Updated budget
});

// Both alice and bob see new budget
```

**Semantics**:
- `Rc<T>` is reference-counted shared ownership
- `clone()` increments ref count (cheap)
- All clones point to the **same data**
- Mutations visible to all owners

**Use case**: Shared state (configuration, caches, shared resources)

#### Option C: `Weak<T>` - Break Reference Cycles

**Purpose**: Reference data without keeping it alive

```rust
struct Node {
  value: I32,
  parent: Weak<Node>,      // Doesn't prevent parent from being freed
  children: [Rc<Node>],    // Strong references keep children alive
}

// Parent owns children (strong), children reference parent (weak)
// No cycle, no memory leak
```

**Semantics**:
- `Weak<T>` doesn't prevent deallocation
- Must upgrade to `Rc<T>` to access data
- Upgrade returns `Option<Rc<T>>` (None if data freed)

**Use case**: Parent-child relationships, observer patterns, caches

---

## Future: Multi-threading Support

### `Arc<T>` - Atomic Reference Counting (Thread-safe)

**Purpose**: Share immutable data across threads

```rust
let shared_data = Arc::new(expensive_computation());

// Spawn threads that share data
spawn_thread(|| {
  process(shared_data.clone());
});

spawn_thread(|| {
  analyze(shared_data.clone());
});
```

**Semantics**: Like `Rc<T>` but thread-safe (atomic operations)

### `Arc<Atomic<T>>` or `Arc<Mutex<T>>` - Shared Mutable State (Thread-safe)

**Purpose**: Share mutable data across threads

```rust
// Option A: Atomic operations (lock-free)
let counter = Arc::new(Atomic::new(0));
counter.fetch_add(1);  // Thread-safe increment

// Option B: Mutex (general-purpose)
let data = Arc::new(Mutex::new(HashMap::new()));
data.lock().insert(key, value);  // Acquire lock, mutate, release
```

---

## Design Decision Summary

### What the User Sees

| Pattern | Syntax | Sharing | Mutation | Thread-safe |
|---------|--------|---------|----------|-------------|
| Immutable value | `let x = T` | ❌ Copies independent | ❌ No | ✅ Yes (immutable) |
| Mutable binding | `let mut x = T` | ❌ Copies independent | ✅ Yes (binding-level) | ❌ No |
| Interior mutability | `Cell<T>` | ❌ Copies independent | ✅ Yes (type-level) | ❌ No |
| Shared immutable | `Rc<T>` (implicit) | ✅ Shared | ❌ No | ❌ No |
| Shared mutable | `Rc<Cell<T>>` | ✅ Shared | ✅ Yes | ❌ No |
| Weak reference | `Weak<T>` | ✅ Shared (weak) | ❌ No | ❌ No |
| Thread-safe shared | `Arc<T>` | ✅ Shared | ❌ No | ✅ Yes |
| Thread-safe mutable | `Arc<Mutex<T>>` | ✅ Shared | ✅ Yes | ✅ Yes |

---

## Implementation Phases

### Phase 0: Current MVP ✅
- Immutable by default
- Automatic ref-counting (implicit)
- No explicit mutation

### Phase 1: Local Mutation ✅ **Parser Complete**
- ✅ `mut` keyword for bindings (binding-level mutability)
- ✅ `mut` for function parameters
- **Parser status**: Fully implemented in grammar, AST, and parser
- **Semantic status**: ❌ Not yet enforced (requires semantic analysis phase)
- **Note**: No `mut` for struct fields - use `Cell<T>` instead for field-level mutability
- **Key property**: `mut` doesn't transfer on assignment/copy

**Parser implementation details**:
- Grammar: `let_statement = "let" [ "mut" ] pattern ...` and `func_param = [ "mut" ] var_name ...`
- AST: `Let_Statement::is_mut` and `Func_Param::is_mut` fields
- Parser: `parse_let_statement()` and `parse_func_param()` handle `mut` keyword
- S-expression: `(let true ...)` and `(param true ...)` for mutable bindings/parameters
- Tests: Comprehensive test coverage in `test_let_statement.cpp` and trait tests

**Design decision**: `mut` applies to **bindings**, not fields. This avoids ambiguity:
```rust
struct Foo { a: I32 }  // No mut on fields

let foo = Foo { a: 10 };
foo.a = 20;  // ❌ ERROR - foo binding is immutable

let mut bar = Foo { a: 10 };
bar.a = 20;  // ✅ OK - bar binding is mutable, so all fields are mutable
```

### Phase 2: Shared Mutation (P4)
- Add `Cell<T>` library type (type-level mutability)
- Add explicit `Rc<T>` (currently implicit)
- Add `Weak<T>` for cycle breaking
- Combine as `Rc<Cell<T>>` for shared mutable state
- **Key property**: `Cell<T>` mutability transfers on copy

### Phase 3: Multi-threading (P5)
- Add `Arc<T>` (atomic ref-counting)
- Add `Atomic<T>` (atomic operations)
- Add `Mutex<T>` or `RwLock<T>` (synchronization)

---

## Comparison with Other Languages

### Swift
```swift
// Swift: class (reference) vs struct (value)
struct Point { var x: Int, var y: Int }  // Value type
class Person { var name: String }         // Reference type
```

### Rust
```rust
// Rust: Explicit Rc/Arc/Cell
let data = Rc::new(RefCell::new(value));  // Shared mutable
```

### Kotlin
```kotlin
// Kotlin: val (immutable) vs var (mutable)
val x = 42        // Immutable binding
var y = 42        // Mutable binding
```

### life-lang (Our Approach)
```rust
// Start simple: immutable by default
let x = 42

// Local mutation: mut is binding-level (doesn't transfer)
let mut a = 10;
let b = a;  // b is immutable (copy of value)

// Type-level mutation: Cell is type-level (transfers)
let a = Cell::new(10);
let b = a;  // b is mutable (copy of Cell)

// Explicit sharing when needed (future)
let shared = Rc::new(Cell::new(value))
```

**Philosophy**: Start simple (immutable), add complexity only when needed.

**Key insight**: `mut` (binding-level) for 90% of cases, `Cell<T>` (type-level) for special cases.

---

## FAQ

### Q: Why immutable by default?
**A**: Simplicity, safety, easier to reason about, enables concurrency.

### Q: How to handle the "Department" problem (many People share one Department)?
**A**: Automatic ref-counting handles this transparently. All People share the same Department instance (ref-counted), but it appears as a value.

### Q: What if I need to update shared data?
**A**: Phase 1 (MVP): Create new versions (functional updates). Phase 2: Use `Rc<Cell<T>>` for explicit shared mutable state.

### Q: Will I have memory leaks with ref-counting?
**A**: Cycles are rare in practice. When needed, use `Weak<T>` to break cycles. Most programs don't need this.

### Q: Why not garbage collection?
**A**: Predictable performance, deterministic cleanup, no GC pauses. Ref-counting with cycle detection is a sweet spot for systems programming.

### Q: What's the difference between `mut` on bindings vs `Cell<T>`?
**A**: 
- **`mut` (binding-level)**: Mutability doesn't transfer on copy
  ```rust
  let mut a = 10;
  let b = a;  // b is immutable
  ```
- **`Cell<T>` (type-level)**: Mutability is part of the type, transfers on copy
  ```rust
  let a = Cell::new(10);
  let b = a;  // b is also a Cell, can mutate
  ```

### Q: Why not `mut` on struct fields like `struct Foo { mut a: I32 }`?
**A**: This creates ambiguity. Instead:
- Use `let mut foo = Foo { a: 10 }` - entire struct is mutable when binding is mut
- Use `struct Foo { a: Cell<I32> }` - specific field has interior mutability

```rust
// ❌ CONFUSING: mut on field
struct Foo { mut a: I32 }
let foo = Foo { a: 10 };
foo.a = 20;  // Should this work? foo is immutable!

// ✅ CLEAR: mut on binding
struct Foo { a: I32 }
let mut foo = Foo { a: 10 };
foo.a = 20;  // Works because foo binding is mut

// ✅ CLEAR: Cell for interior mutability
struct Foo { a: Cell<I32> }
let foo = Foo { a: Cell::new(10) };
foo.a.set(20);  // Works - Cell provides interior mutability
foo.a = Cell::new(30);  // ❌ ERROR - can't reassign the Cell itself
```

### Q: How does `Cell<T>` work internally?
**A**: `Cell<T>` uses `unsafe` code internally (compiler magic) to provide safe interior mutability. The `Cell` binding is immutable, but the value **inside** can be mutated via `get()` and `set()` methods. This is safe because `Cell<T>` is only allowed for types that implement `Copy` (no references that could be invalidated).

---

## References

- **Rust**: `Rc`, `Arc`, `Cell`, `RefCell` - https://doc.rust-lang.org/std/rc/
- **Swift**: Value vs Reference semantics - https://developer.apple.com/swift/blog/?id=10
- **Kotlin**: Immutability - https://kotlinlang.org/docs/basic-syntax.html#variables
