# Semantic Analysis - Implementation Roadmap

**Current Status**: Minimal foundation complete (Module_Loader, Semantic_Context)
**Next Priority**: Import resolution for cross-module name lookup

---

## Overview

This roadmap follows the **"lightweight solutions first"** philosophy:
- Work directly with AST - no duplication
- Implement features incrementally as needed
- Avoid premature abstraction
- Measure before optimizing

See `doc/SEMANTIC_ARCHITECTURE.md` for current design.

---

## Current Implementation ✓

### Phase 0: Module Loading (Complete)

**What we have:**
```cpp
// Module discovery from filesystem
Module_Loader::discover_modules(src_root) → vector<Module_Descriptor>
Module_Loader::load_module(descriptor) → ast::Module

// Semantic context for name resolution
Semantic_Context::load_modules(src_root) → bool
Semantic_Context::find_type_def(module, name) → ast::Item const*
Semantic_Context::find_func_def(module, name) → ast::Item const*
```

**Key decisions:**
- ✅ Folder = module (all `.life` files merged)
- ✅ Direct AST storage (no symbol table duplication)
- ✅ Simple name lookup by scanning `ast::Module.items`

**Test coverage:**
- `tests/semantic/test_module_loader.cpp` - Unit tests
- `tests/integration/test_module_discovery.cpp` - Filesystem discovery
- `tests/integration/test_module_loading.cpp` - Multi-file modules
- `tests/integration/test_semantic_context.cpp` - Full API

---

## Next Steps

### Phase 1: Import Resolution (Next Priority)

**Goal**: Enable cross-module name lookup via `import` statements

**Current limitation:**
```cpp
// TODO: Implement import resolution
std::optional<std::pair<std::string, ast::Item const*>>
resolve_type_name(std::string const& current_module, 
                 ast::Type_Name const& name) const;
```

**Implementation plan:**

1. **Parse imports into lookup table** (~1 day)
   ```cpp
   // For module "Main" with: import Geometry.{ Point, Circle as C };
   // Build map: "Point" → ("Geometry", "Point")
   //            "C" → ("Geometry", "Circle")
   std::map<std::string, std::pair<std::string, std::string>> import_map;
   ```

2. **Implement resolve_type_name()** (~1 day)
   ```cpp
   // 1. Check if name is in local module (find_type_def)
   // 2. Check import_map for imported name
   // 3. Follow to source module and lookup
   // 4. Verify is_pub visibility
   ```

3. **Implement resolve_var_name()** (~1 day)
   - Same logic as type names
   - Handle both functions and imported symbols

4. **Add error diagnostics** (~1 day)
   - "Type 'Foo' not found in current module or imports"
   - "Cannot import 'Bar' - not marked pub"
   - "Ambiguous import: 'Point' found in multiple modules"

5. **Test import resolution** (~1 day)
   - Simple imports: `import Module.{ Item }`
   - Aliased imports: `import Module.{ Item as Alias }`
   - Multi-level imports: `import A.B.C.{ Item }`
   - Error cases: non-pub, not found, circular

**Success criteria:**
- Cross-module type/function references work
- Visibility enforced (only `pub` items importable)
- Clear error messages for resolution failures

---

### Phase 2: Basic Type Checking (Future)

**Goal**: Validate type compatibility in expressions

**When to implement:**
- After import resolution is stable
- When we have realistic life-lang programs to test against

**Scope:**
1. Expression type checking (binary ops, function calls)
2. Assignment compatibility
3. Function signature matching
4. Return type validation

**What we DON'T need yet:**
- ❌ Type inference (explicit types required)
- ❌ Generic instantiation (parser handles syntax)
- ❌ Trait constraint solving (defer until needed)

**Implementation approach:**
- Add `get_expr_type()` helper that traverses AST
- Work directly with `ast::Type_Name` - no separate type system
- Only build type representations if inference is added

---

### Phase 3: Visibility Checking (Future)

**Goal**: Prevent private types leaking through public APIs

**Example violations to catch:**
```rust
struct Internal { x: I32 }  // Not pub

pub fn get_internal(): Internal { ... }  // ERROR: pub function returns non-pub type
pub struct Public { pub field: Internal }  // ERROR: pub field uses non-pub type
```

**Implementation:**
- Traverse public item signatures
- Check all referenced types are also `pub`
- Report visibility leak errors

---

### Phase 4: Advanced Features (Deferred)

Only implement when language design requires them:

- **Trait constraint checking**: When trait bounds are used in code
- **Generic instantiation validation**: When generic code is written
- **Type inference**: If we decide to support `let x = ...` without types
- **Borrow checking**: Not planned (using ref counting instead)

---

## Non-Goals

**What we deliberately avoid:**

1. **Full type system before needed**: AST contains type names, sufficient for now
2. **Symbol table duplication**: AST already has all symbols with visibility
3. **Complex dependency graphs**: Simple module→imports map is enough
4. **Premature optimization**: Direct AST traversal is fast enough
5. **Speculative features**: Only implement what the language actually uses

---

## Success Metrics

**How we know we're on track:**

- ✅ **Compilation speed**: Debug builds < 5 seconds (currently ~2s)
- ✅ **Test coverage**: All public APIs have integration tests
- ✅ **Code size**: Semantic analysis < 500 lines total
- ✅ **Error quality**: Clear messages with source locations
- ✅ **Incrementality**: Each phase adds value independently

---

## Timeline Estimates

| Phase | Effort | Dependencies | Status |
|-------|--------|--------------|--------|
| Module Loading | 3 days | Parser | ✅ Complete |
| Import Resolution | 5 days | Module Loading | 🔜 Next |
| Basic Type Checking | 7 days | Import Resolution | 📋 Future |
| Visibility Checking | 3 days | Type Checking | 📋 Future |

**Total estimated**: ~3 weeks for core semantic analysis

**Note**: Estimates assume incremental implementation with tests at each step.

---

## Design Principles

Following project philosophy throughout:

1. **Lightweight first**: Start simple, add complexity only when proven necessary
2. **Single source of truth**: AST contains all information, avoid duplication
3. **Fast iteration**: Keep compile times low, minimize template metaprogramming
4. **Test-driven**: Write tests before/during implementation, not after
5. **Pragmatic**: Solve actual problems, not theoretical ones

---

## References

- **Architecture**: `doc/SEMANTIC_ARCHITECTURE.md` - Current design
- **Module System**: `doc/MODULE_SYSTEM.md` - Import/export semantics
- **Grammar**: `doc/GRAMMAR.md` - Import statement syntax
- **Implementation**: `src/semantic/semantic_context.{hpp,cpp}`
