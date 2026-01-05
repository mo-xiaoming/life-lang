# Semantic Analysis Architecture

**Current Implementation**: Minimal, working directly with AST - single source of truth

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
│  │                      │  filesystem, parses .life files   │
│  └──────────┬───────────┘                                   │
│             │                                               │
│             │ provides parsed ast::Module                   │
│             │                                               │
│  ┌──────────▼───────────────────────────────────────────┐   │
│  │   Semantic_Context                                   │   │
│  │   - Stores loaded modules (map<string, ast::Module>) │   │
│  │   - find_type_def() / find_func_def()                │   │
│  │   - resolve_type_name() / resolve_var_name()         │   │
│  │   - Works directly with AST nodes                    │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Component Responsibilities

### Module_Loader

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

### Semantic_Context

**Purpose**: Manage loaded modules and provide name resolution

**Interface**:

```cpp
class Semantic_Context {
  // Load all modules from src/ directory
  bool load_modules(fs::path const& src_root);

  // Get a loaded module
  ast::Module const* get_module(std::string const& module_path) const;

  // Direct AST queries (no symbol table needed)
  ast::Item const* find_type_def(std::string const& module_path,
                                  std::string_view type_name) const;
  ast::Item const* find_func_def(std::string const& module_path,
                                  std::string_view func_name) const;

  // Name resolution (follows imports) - TODO: implement import resolution
  std::optional<std::pair<std::string, ast::Item const*>>
    resolve_type_name(std::string const& current_module,
                     ast::Type_Name const& name) const;
  std::optional<std::pair<std::string, ast::Item const*>>
    resolve_var_name(std::string const& current_module,
                    ast::Var_Name const& name) const;
};
```

**Key Design**:

- Direct AST traversal - no separate symbol table
- Simple map storage: `std::map<std::string, ast::Module>`
- Non-owning pointers to AST items for lookup results
- Add features incrementally as needed

---

## What We Don't Have (Yet)

**Deliberately omitted until proven necessary:**

- ❌ **Symbol Table**: AST already contains all symbols with visibility
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

1. **Import Resolution** (next priority)
   - Resolve names across module boundaries
   - Check for circular imports
   - Validate `pub` visibility at module boundaries

2. **Type Checking**
   - Validate type compatibility in expressions
   - Check function signatures
   - Ensure trait bounds are satisfied

3. **Visibility Checking**
   - Verify `pub` types don't leak through private APIs
   - Check field access permissions

4. **Type Inference** (if added to language)
   - Would require constraint solver
   - Only then build separate type representations

---

## Benefits of Current Design

✅ **Minimal code**: ~200 lines vs. ~1000+ for full symbol table approach
✅ **Fast compilation**: No complex template instantiations
✅ **Easy to debug**: Single source of truth, straightforward logic
✅ **Flexible**: Easy to extend when requirements are clear
✅ **Testable**: Simple API, focused unit tests

This architecture follows the project's "lightweight solutions first" philosophy.
