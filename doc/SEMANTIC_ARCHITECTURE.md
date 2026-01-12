# Semantic Analysis Architecture

**Status**: Module loading ✅ | Import resolution ✅ | Type checking (next)

---

## Design Philosophy

**Work directly with AST - no duplication, no separate type representations.**

The AST (`ast::Module`) already contains all necessary information:

- `items` - all definitions (functions, structs, enums, etc.) with `.is_pub` visibility
- `imports` - import statements for cross-module references

This design prioritizes:

- **Simplicity**: Minimal code, easy to understand
- **Fast compilation**: No complex type hierarchies or template metaprogramming
- **Maintainability**: Single source of truth, no synchronization issues
- **Incrementality**: Add complexity only when actually needed

---

## Current Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Semantic Analysis                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────────────┐                                   │
│  │   Module_Loader      │  Discovers modules from           │
│  │   (internal)         │  filesystem, parses .life files   │
│  └──────────┬───────────┘                                   │
│             │                                               │
│             │ provides parsed ast::Module                   │
│             ▼                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │   Semantic_Context (public API)                      │   │
│  │   - Stores loaded modules (map<string, ast::Module>) │   │
│  │   - Import maps for O(1) cross-module lookup         │   │
│  │   - Name indices for O(1) item lookup                │   │
│  │   - find_type_def() / find_func_def()                │   │
│  │   - resolve_type_name() / resolve_var_name()         │   │
│  │   - Circular import detection                        │   │
│  └──────────────────────────────────────────────────────┘   │
│             │                                               │
│             ▼                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │   Diagnostic_Manager                                 │   │
│  │   - Collects errors across all modules               │   │
│  │   - Clang-style error output with source context     │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Component Responsibilities

### Module_Loader (internal)

**Purpose**: Discover and parse modules from filesystem

**Interface**:

```cpp
class Module_Loader {
  // Scan src/ directory recursively to find all modules
  static std::vector<Module_Descriptor> discover_modules(fs::path const& src_root);

  // Load and parse all files in a module, merge into single ast::Module
  static std::optional<ast::Module> load_module(Module_Descriptor const& desc);

  // Convert directory names to module names (geometry -> Geometry)
  static std::string dir_name_to_module_name(std::string_view dir_name);
};
```

**Key Design**:

- Folder = module (all `.life` files in folder form one module)
- Recursive directory scanning
- Case conversion: `snake_case` dirs → `Camel_Snake_Case` modules
- Returns parsed AST directly - no intermediate representation

### Semantic_Context (public API)

**Purpose**: Manage loaded modules and provide name resolution

**Interface**:

```cpp
class Semantic_Context {
  // Constructor takes Diagnostic_Manager for error reporting
  explicit Semantic_Context(Diagnostic_Manager& diagnostics);

  // Load all modules from src/ directory
  bool load_modules(fs::path const& src_root);

  // Get a loaded module
  ast::Module const* get_module(std::string const& module_path) const;

  // Direct AST queries - O(1) via internal indices
  ast::Item const* find_type_def(std::string const& module_path,
                                  std::string_view type_name) const;
  ast::Item const* find_func_def(std::string const& module_path,
                                  std::string_view func_name) const;
  ast::Func_Def const* find_method_def(std::string const& module_path,
                                        std::string_view type_name,
                                        std::string_view method_name) const;

  // Name resolution - follows imports, enforces visibility
  std::optional<std::pair<std::string, ast::Item const*>>
    resolve_type_name(std::string const& current_module,
                     ast::Type_Name const& name) const;
  std::optional<std::pair<std::string, ast::Item const*>>
    resolve_var_name(std::string const& current_module,
                    ast::Var_Name const& name) const;
};
```

**Key Design**:

- Pimpl idiom to hide implementation details
- Import maps built on load: `import A.{ X as Y }` → `Y → (A, X)`
- Name indices for O(1) lookups: `(module, name) → Item*`
- Non-owning pointers to AST items for lookup results
- Circular import detection with clear error messages

---

## Implemented Features

### ✅ Module Loading

- Folder = module (all `.life` files merged into single `ast::Module`)
- Direct AST storage (no symbol table duplication)
- Case conversion: `snake_case` dirs → `Camel_Snake_Case` modules

### ✅ Import Resolution

- Simple imports: `import Module.{ Item }`
- Aliased imports: `import Module.{ Item as Alias }`
- Multi-level imports: `import A.B.C.{ Item }`
- Visibility enforcement (only `pub` items importable)
- Circular import detection

### ✅ Error Reporting

- Clang-style diagnostics with source positions
- Clear error messages:
  - "cannot import '{name}' from module '{module}' - not marked pub"
  - "type/function '{name}' not found in current module or imports"
  - "circular import detected: A → B → C → A"

---

## What We Don't Have (Yet)

**Deliberately omitted until proven necessary:**

- ❌ **Type System**: Type names in AST are sufficient for now
- ❌ **Scope Stack**: Direct AST queries replace scope management
- ❌ **Type Inference**: Not needed yet (explicit types required)
- ❌ **Generic Instantiation**: Parser handles syntax, semantics deferred

**Add complexity only when:**

- Type inference is implemented
- Generic constraints need validation
- Advanced visibility checks beyond simple `is_pub` flags
- Performance profiling shows AST traversal is a bottleneck

---

## Future Extensions

When needed, these can be added incrementally:

1. **Type Checking** (next priority)
   - Validate type compatibility in expressions
   - Check function signatures
   - Ensure trait bounds are satisfied

2. **Visibility Leak Checking**
   - Verify `pub` types don't leak through private APIs
   - Check field access permissions

3. **Type Inference** (if added to language)
   - Would require constraint solver
   - Only then build separate type representations

---

## Benefits of Current Design

✅ **Minimal code**: Direct AST traversal, no duplication
✅ **Fast compilation**: No complex template instantiations
✅ **Easy to debug**: Single source of truth, straightforward logic
✅ **Flexible**: Easy to extend when requirements are clear
✅ **Testable**: Simple API, comprehensive test coverage

This architecture follows the project's "lightweight solutions first" philosophy.
