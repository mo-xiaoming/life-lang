# Life-lang Module System Design

**Implementation Status:**

- ✅ Parser: `pub` keyword for top-level items, struct fields, and impl methods
- ✅ AST: Module, Item, Import_Statement, Import_Item, field/method visibility
- ❌ Semantic analysis: Not yet implemented (name resolution, visibility checking, etc.)

**Current Phase:** Parser complete, semantic analysis next

---

## Overview

Life-lang uses a **folder-based module system** with explicit visibility control via the `pub` keyword. The design prioritizes:

- **Simplicity**: Folder = module, file = organization
- **Explicit imports**: No implicit dependencies
- **Clear visibility**: Two-level system (module-internal vs public)
- **Scalability**: Nested modules for large projects

---

## Core Concepts

### 0. Project Structure Convention

**Rule:** All source files must be under the `src/` directory

```
project_root/
  src/                       ← All .life files go here
    main.life                → Module "Main"
    geometry/
      point.life             → Module "Geometry"
    std/
      math/
        trig.life            → Module "Std.Math"
  build/                     ← Build artifacts
  tests/                     ← Test files (optional)
  doc/                       ← Documentation
  README.md
```

**Module path derivation:**

- Everything relative to `src/` directory
- `src/geometry/` → `Geometry`
- `src/std/math/` → `Std.Math`
- `src/user_profile/settings/` → `User_Profile.Settings`

**Rationale:**

- Clear separation of source code from other project files
- Industry standard (Rust, many others use `src/`)
- Makes module discovery deterministic and unambiguous
- No configuration needed - convention over configuration

---

### 1. Module Definition

**Rule:** A module is defined by the filesystem structure

```
repo/
  main.life                    → file-module "Main"
  utils.life                   → file-module "Utils"
  geometry/
    point.life                 → part of folder-module "Geometry"
    circle.life                → part of folder-module "Geometry"
    shapes/
      polygon.life             → part of folder-module "Geometry.Shapes"
      triangle.life            → part of folder-module "Geometry.Shapes"
```

**Module types:**

- **File-module**: Top-level `.life` file = single-file module
- **Folder-module**: Folder containing `.life` files = multi-file module
  - All files in the same folder share the same module namespace
  - Files are purely organizational, not encapsulation boundaries
  - Nested folders create nested modules (independent from parent)

**Module naming:**

- Follows `Camel_Snake_Case` convention (same as type names)
- Module path uses `.` separator: `Geometry.Shapes.Polygon`

**Filesystem-to-module-name mapping:**

- Filesystem paths use **lowercase with underscores** (portable, works on all systems)
- Module names in code use **Camel_Snake_Case** (language convention)
- Mapping is deterministic: `geometry/shapes/` → `Geometry.Shapes`
- Conversion: each path component `lowercase_snake` → `Camel_Snake_Case`
  - Split on `_`, capitalize first letter of each word, rejoin with `_`
  - Example: `user_profile` → `User_Profile`, `std` → `Std`

**Examples:**

```
Filesystem                    Module Path           Import Syntax
──────────────────────────────────────────────────────────────────
geometry/                     Geometry              import Geometry.{Point};
std/collections/              Std.Collections       import Std.Collections.{Vec};
user_profile/settings/        User_Profile.Settings import User_Profile.Settings.{Theme};
```

**Rationale:**

- Lowercase filesystem paths avoid case-sensitivity issues (Windows/macOS are case-insensitive)
- Deterministic mapping ensures one source of truth
- Code uses language conventions while filesystem follows OS best practices

---

### 2. Visibility Levels

**Two visibility levels:**

| Visibility | Syntax | Scope |
|------------|--------|-------|
| **Module-internal** | No `pub` keyword | Visible only within the module (all files in same folder) |
| **Public** | `pub` keyword | Exported from module, accessible to importers |

**Examples:**

```rust
// geometry/point.life (Geometry module)
pub struct Point { x: I32, y: I32 }      // Exported from Geometry
struct Internal_Helper { data: I32 }     // Internal to Geometry module

pub fn distance(p1: Point, p2: Point): F64 { /* ... */ }  // Exported
fn validate(p: Point): Bool { /* ... */ }                 // Internal

// geometry/circle.life (same Geometry module)
struct Circle {
    center: Point           // OK - Point visible (same module, pub doesn't matter)
    helper: Internal_Helper // OK - Internal_Helper visible (same module)
}

pub fn area(c: Circle): F64 {
    if validate(c.center) { /* ... */ }  // OK - validate visible (same module)
}
```

---

### 3. Import System

**Import syntax:**

```rust
import Module.Path.{Item1, Item2};
import Module.{Item as Alias};
```

**Rules:**

- Uses `.` as path separator
- Dot before braces: `.{items}` indicates "from this module, import these items"
- Imports are explicit (no wildcard imports in initial design)
- Only `pub` items can be imported
- Must import from different modules (same module items are automatically visible)
- **Name conflicts**: Importing the same name from multiple modules is a compile-time error
- **Renaming with `as`**: Use `Item as Alias` to resolve conflicts or improve clarity

**Name Conflict Resolution:**

Without `as`:

```rust
import Graphics.{Point};  // Graphics.Point
import Geometry.{Point};  // ERROR: 'Point' already imported from Graphics

// Compile error: Name conflict - 'Point' imported from both Graphics and Geometry
```

With `as` to resolve:

```rust
import Graphics.{Point as Graphics_Point};
import Geometry.{Point as Geometry_Point};

// Now both can be used
let p1 = Graphics_Point { x: 0, y: 0 };
let p2 = Geometry_Point { x: 1.0, y: 1.0 };
```

**Rationale**:

- `as` provides explicit renaming for conflict resolution
- Improves code clarity when importing similarly-named items

**Examples:**

```rust
// main.life
import Geometry.{Point, distance};
import Geometry.Shapes.{Polygon, Triangle};

fn main(): I32 {
    let p1 = Point { x: 0, y: 0 };
    let p2 = Point { x: 3, y: 4 };
    let d = distance(p1, p2);
    return 0;
}
```

```rust
// geometry/circle.life (Geometry module)
import Geometry.Shapes.{Polygon};  // Import from nested module

pub struct Circle {
    approx: Polygon  // Using imported type
}
```

```rust
// geometry/shapes/polygon.life (Geometry.Shapes module)
import Geometry.{Point};  // Import from parent module

pub struct Polygon {
    vertices: Vec<Point>  // Using imported type
}
```

---

### 4. Module Independence

**Key principle:** Each folder is an independent module, regardless of nesting.

**No special parent-child privileges:**

- Parent module cannot access child module internals without import
- Child module cannot access parent module internals without import
- Modules at same level (siblings) must import from each other

**Example:**

```rust
// geometry/point.life (Geometry module)
struct Helper { x: I32 }  // Internal to Geometry

// geometry/shapes/polygon.life (Geometry.Shapes module - DIFFERENT module)
import Geometry.{Point};  // REQUIRED - different module
// Cannot access Helper - not pub, not importable

pub struct Polygon { vertices: Vec<Point> }
```

---

## Struct Fields and Methods

**Implementation Status:**

- ✅ Parser: `pub` on struct fields and impl methods
- ✅ AST: `is_pub` flags on `Struct_Field` and `Func_Def`
- ❌ Semantic validation: Visibility leak checking not yet implemented

### Current Behavior: Field and Method-level `pub`

**Struct fields:**

```rust
pub struct Point {
    pub x: I32,  // Public field - accessible to importers
    y: I32       // Internal field - only accessible within module
}

// app.life
import Geometry.{Point};
let p = Point { x: 1, y: 2 };  // OK - can initialize all fields (constructor)
let x = p.x;                    // OK - x is pub (when field access implemented)
let y = p.y;                    // ERROR - y not pub (semantic check, not yet implemented)
```

**Impl methods:**

```rust
impl Point {
    pub fn new(x: I32, y: I32): Point {
        return Point { x: x, y: y };
    }

    fn internal_helper(self): I32 {
        return self.x;  // OK - internal method can access any field
    }

    pub fn distance(self): F64 {
        return self.internal_helper();  // OK - can call internal methods from same module
    }
}

// app.life
import Geometry.{Point};
let p = Point.new(1, 2);        // OK - new is pub
let d = p.distance();            // OK - distance is pub
let h = p.internal_helper();     // ERROR - internal_helper not pub (semantic check)
```

**Design note:** All fields can be initialized in struct literals currently (constructors). Field access visibility will be enforced during semantic analysis.

---

### Future: Visibility Leak Checking (Semantic Analysis Phase)

**Rule:** Public items cannot expose non-public types (will be enforced during semantic analysis).

```rust
struct Private_Type { data: I32 }  // NOT pub

pub struct Point {
    pub x: I32,                    // OK - I32 is always visible
    pub data: Private_Type         // ERROR: field is pub but type is not pub
}
```

This will be detected and reported during the semantic analysis phase.

---

## Visibility Consistency Rules

**Status:** Will be enforced during semantic analysis phase.

**Rule:** Public items cannot expose non-public types.

This prevents **visibility leaks** where internal types become indirectly accessible through public interfaces.

### Struct Fields (When field-level `pub` is implemented)

```rust
struct Private_Type { data: I32 }  // NOT pub

pub struct Point {
    pub x: I32,                    // OK - I32 is always visible
    pub data: Private_Type         // ERROR: field is pub but type is not pub
}
```

**Error message:**

```
error: cannot use non-public type in public field
  --> geometry/point.life:5:5
   |
 3 | struct Private_Type { data: I32 }
   | ----------- type 'Private_Type' is not public
 4 | pub struct Point {
 5 |     pub data: Private_Type
   |     ^^^^^^^^^^^^^^^^^^^^^^ field 'data' is public but type 'Private_Type' is not
   |
help: make the type public
   |
 3 | pub struct Private_Type { data: I32 }
   | +++
```

### Function Signatures

```rust
struct Private { x: I32 }

pub fn get_private(): Private {       // ERROR: return type not pub
    return Private { x: 42 };
}

pub fn take_private(p: Private): () { // ERROR: param type not pub
}
```

**Must be:**

```rust
pub struct Private { x: I32 }

pub fn get_private(): Private {       // OK - Private is pub
    return Private { x: 42 };
}

pub fn take_private(p: Private): () { // OK - Private is pub
}
```

### Generic Type Parameters

```rust
struct Private { x: I32 }

pub struct Container<T> {
    pub value: T  // T's visibility checked at instantiation site
}

pub struct MyContainer {
    pub data: Container<Private>  // ERROR: Private is not pub
}
```

**Enforcement:** During semantic analysis, check visibility transitively through generic instantiations.

---

## File Organization Guidelines

### Small Modules (Single File)

```rust
// math.life - all in one file
pub fn add(x: I32, y: I32): I32 { return x + y; }
pub fn multiply(x: I32, y: I32): I32 { return x * y; }
fn validate(x: I32): Bool { return x >= 0; }  // internal
```

### Growing Modules (Split into Folder)

When a file gets too long, split into folder **without changing API:**

```rust
// Before: math.life
pub fn add(x: I32, y: I32): I32 { /* ... */ }
pub fn multiply(x: I32, y: I32): I32 { /* ... */ }

// After: Create math/ folder
// math/arithmetic.life
pub fn add(x: I32, y: I32): I32 { /* ... */ }
pub fn multiply(x: I32, y: I32): I32 { /* ... */ }

// math/internal.life
fn helper(): I32 { /* ... */ }  // Still internal to Math module

// Imports don't change:
import Math.{add, multiply};  // Same as before
```

### Large Projects (Nested Modules)

```rust
// geometry/           → Geometry module
//   point.life        → Basic types
//   circle.life       → Circle implementation
//   shapes/           → Geometry.Shapes module (separate from Geometry)
//     polygon.life
//     triangle.life
//   algorithms/       → Geometry.Algorithms module
//     intersection.life
//     tessellation.life
```

**Guideline:** Create submodules when:

- Logical separation needed (shapes vs algorithms)
- Different visibility requirements
- Independent compilation desired

---

## Implementation Phases

### Phase 1: Parser (Current)

- ✅ No module system syntax needed yet
- ✅ Parse individual files as standalone modules
- ✅ No `pub` keyword in grammar
- ✅ No `import` statements

### Phase 2: Semantic Analysis (Next)

- Add `pub` keyword to grammar
- Add `import` statement to grammar
- Implement module discovery (scan filesystem)
- Build module dependency graph
- Implement name resolution across modules
- Enforce visibility rules
- Check visibility consistency (no leaks)

### Phase 3: Refinement (Later)

- Add method-level `pub` if needed
- Add field-level `pub` if needed
- Optimize module compilation (incremental builds)
- Add module-level documentation

---

## Comparison with Other Languages

| Feature | Life-lang | Rust | Go | Zig |
|---------|-----------|------|-----|-----|
| **Module = ?** | Folder | File/mod.rs | Package (folder) | File |
| **Separator** | `.` | `::` | `.` | `.` |
| **Visibility** | `pub` | `pub` | Capitalization | `pub` |
| **Import** | Explicit | Explicit | Explicit | `@import` path |
| **Nested modules** | Yes (independent) | Yes (hierarchical) | No | Yes (via @import) |
| **File = ?** | Organization | Module | Organization | Namespace |

**Design choice:** Life-lang follows Go's folder-as-module for simplicity, but with Zig-style explicit `pub` for clarity.

---

## Examples

### Complete Multi-Module Example

```rust
// geometry/point.life (Geometry module)
pub struct Point {
    x: I32,
    y: I32
}

pub fn origin(): Point {
    return Point { x: 0, y: 0 };
}

fn validate(p: Point): Bool {  // internal
    return p.x >= 0 && p.y >= 0;
}

// geometry/circle.life (Geometry module - same as point.life)
pub struct Circle {
    center: Point,     // Point visible (same module)
    radius: F64
}

impl Circle {
    pub fn new(center: Point, radius: F64): Circle {
        return Circle { center: center, radius: radius };
    }

    pub fn area(self): F64 {
        return 3.14159 * self.radius * self.radius;
    }
}

// geometry/shapes/polygon.life (Geometry.Shapes module - DIFFERENT)
import Geometry.{Point};  // Must import - different module

pub struct Polygon {
    vertices: Vec<Point>
}

impl Polygon {
    pub fn new(vertices: Vec<Point>): Polygon {
        return Polygon { vertices: vertices };
    }
}

// main.life (Main module)
import Geometry.{Point, Circle, origin};
import Geometry.Shapes.{Polygon};

fn main(): I32 {
    let p = origin();
    let c = Circle.new(p, 5.0);
    let area = c.area();

    let vertices = Vec.new();
    vertices.push(p);
    let poly = Polygon.new(vertices);

    return 0;
}
```

---

## Future Considerations

### Not Included (By Design)

- **Wildcard imports** (`import Geometry.*`) - Keep imports explicit
- **Re-exports** (`pub use X`) - May add later if needed
- **Conditional compilation** (`#[cfg]`) - Not planned
- **Existential types** (`opaque`) - Too complex, enums suffice
- **Module aliases** (`import Geometry as Geo`) - Keep it simple

### May Add Later

- **Selective imports with rename**: `import Geometry.{Point as Pt}` (implemented in parser)
- **Module-level attributes**: `@doc`, `@deprecated`
- **Package management**: External dependencies
- **Module privacy levels**: More granular than `pub`/private

---

## Questions & Answers

**Q: Can files in the same folder import from each other?**
A: No need - they share the same namespace. All items (pub or not) are visible to each other.

**Q: What if I want to hide something even within a module?**
A: Put it in a nested module (subfolder) and don't make it `pub`.

**Q: Can I have both `geometry.life` and `geometry/` folder?**
A: Not in initial design - one or the other. (May revisit if facade pattern needed)

**Q: How do I document what's public?**
A: The `pub` keyword IS the documentation. Grep for `pub` to see API surface.

**Q: What about circular dependencies?**
A: Modules can import from each other. Semantic analysis will detect cycles and error.

---

**Version:** 1.0
**Last Updated:** December 17, 2025
**Status:** Design complete, implementation pending
