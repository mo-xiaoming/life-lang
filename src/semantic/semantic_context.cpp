#include "semantic_context.hpp"

#include "../diagnostics.hpp"
#include "module_loader.hpp"

#include <algorithm>
#include <format>
#include <functional>
#include <map>
#include <set>

namespace life_lang::semantic {

// ============================================================================
// Semantic_Context::Impl - Private implementation
// ============================================================================

struct Semantic_Context::Impl {
  Diagnostic_Manager* diagnostics = nullptr;

  // Module path (dot-separated like "Std.Collections") -> parsed AST
  std::map<std::string, ast::Module> modules;

  // Import resolution: module_path -> (local_name -> (source_module, item_name))
  // Example: For "import Geometry.{ Point, Circle as C }" in module "Main"
  //   import_maps["Main"]["Point"] = ("Geometry", "Point")
  //   import_maps["Main"]["C"] = ("Geometry", "Circle")
  std::map<std::string, std::map<std::string, std::pair<std::string, std::string>>> import_maps;

  // Name-to-item indices for O(1) lookups (built after modules loaded)
  // Key: (module_path, item_name), Value: pointer to Item in module
  std::map<std::pair<std::string, std::string>, ast::Item const*> type_index;
  std::map<std::pair<std::string, std::string>, ast::Item const*> func_index;

  // Build import map for all loaded modules
  void build_import_maps();

  // Build name indices for fast lookups
  void build_name_indices();

  // Check for circular import dependencies between modules
  // Returns true if a cycle is detected (and reports error)
  [[nodiscard]] bool check_circular_imports() const;

  // Report a semantic error with source position
  // The span.file contains the File_Id for proper error location
  void error(Source_Range span_, std::string message_) const { diagnostics->add_error(span_, std::move(message_)); }

  // Helper: Check if a name matches an item
  [[nodiscard]] static bool item_matches_name(ast::Item const& item_, std::string_view name_);

  // Helper: Get the name of an item (function name, struct name, etc.)
  [[nodiscard]] static std::optional<std::string> get_item_name(ast::Item const& item_);

  // Helper methods for resolve_type_name - handle each Type_Name variant
  // These take the parent context to call public methods like find_type_def()
  [[nodiscard]] std::optional<std::pair<std::string, ast::Item const*>> resolve_path_type(
      Semantic_Context const& ctx_,
      std::string const& current_module_,
      ast::Path_Type const& type_
  ) const;
  static void resolve_function_type(
      Semantic_Context const& ctx_,
      std::string const& current_module_,
      ast::Function_Type const& type_
  );
  static void
  resolve_array_type(Semantic_Context const& ctx_, std::string const& current_module_, ast::Array_Type const& type_);
  static void
  resolve_tuple_type(Semantic_Context const& ctx_, std::string const& current_module_, ast::Tuple_Type const& type_);
};

// ============================================================================
// Semantic_Context - Public interface
// ============================================================================

Semantic_Context::Semantic_Context(Diagnostic_Manager& diagnostics_) : m_impl(std::make_unique<Impl>()) {
  m_impl->diagnostics = &diagnostics_;
}

Semantic_Context::Semantic_Context(Semantic_Context&&) noexcept = default;
Semantic_Context& Semantic_Context::operator=(Semantic_Context&&) noexcept = default;
Semantic_Context::~Semantic_Context() = default;

bool Semantic_Context::load_modules(std::filesystem::path const& src_root_) {
  // Discover all modules in src/ directory
  auto const descriptors = Module_Loader::discover_modules(src_root_);

  // Load and parse each module (files are registered with the shared registry)
  for (auto const& desc: descriptors) {
    auto module_opt = Module_Loader::load_module(desc, *m_impl->diagnostics);
    if (!module_opt.has_value()) {
      return false;  // Parse error or duplicate definition
    }

    std::string const module_path = desc.module_path_string();

    // Store the module
    m_impl->modules[module_path] = std::move(*module_opt);
  }

  // Check for circular imports before building import maps
  if (m_impl->check_circular_imports()) {
    return false;  // Circular import detected
  }

  // Build import maps for cross-module name resolution
  m_impl->build_import_maps();

  // Build name indices for O(1) lookups
  m_impl->build_name_indices();

  return true;
}

ast::Module const* Semantic_Context::get_module(std::string const& module_path_) const {
  auto const it = m_impl->modules.find(module_path_);
  if (it == m_impl->modules.end()) {
    return nullptr;
  }
  return &it->second;
}

std::vector<std::string> Semantic_Context::module_paths() const {
  std::vector<std::string> paths;
  paths.reserve(m_impl->modules.size());
  std::ranges::transform(m_impl->modules, std::back_inserter(paths), [](auto const& pair_) { return pair_.first; });
  return paths;
}

ast::Item const* Semantic_Context::find_type_def(std::string const& module_path_, std::string_view type_name_) const {
  // O(1) lookup using pre-built index
  auto const key = std::make_pair(module_path_, std::string(type_name_));
  auto const it = m_impl->type_index.find(key);
  if (it != m_impl->type_index.end()) {
    return it->second;
  }
  return nullptr;
}

ast::Item const* Semantic_Context::find_func_def(std::string const& module_path_, std::string_view func_name_) const {
  // O(1) lookup using pre-built index
  auto const key = std::make_pair(module_path_, std::string(func_name_));
  auto const it = m_impl->func_index.find(key);
  if (it != m_impl->func_index.end()) {
    return it->second;
  }
  return nullptr;
}

ast::Func_Def const* Semantic_Context::find_method_def(
    std::string const& module_path_,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - semantically distinct: type vs method
    std::string_view type_name_,
    std::string_view method_name_
) const {
  auto const* module = get_module(module_path_);
  if (module == nullptr) {
    return nullptr;
  }

  // Search module-level items for impl blocks
  for (auto const& item: module->items) {
    // Check if this is an impl block
    auto const* impl_ptr = std::get_if<std::shared_ptr<ast::Impl_Block>>(&item.item);
    if (impl_ptr == nullptr) {
      continue;
    }

    auto const& impl_block = **impl_ptr;

    // Check if impl block is for the requested type
    // The type_name in Impl_Block is a Type_Name variant - for simple types it's Path_Type
    auto const* path_type =
        std::get_if<ast::Path_Type>(&static_cast<ast::Type_Name::Base_Type const&>(impl_block.type_name));
    if (path_type == nullptr || path_type->segments.empty()) {
      continue;  // Not a simple path type (e.g., impl for function type - unlikely)
    }

    // For simple impl like "impl Point", segments[0].value is "Point"
    // For generic impl like "impl Array<T>", segments[0].value is "Array"
    if (path_type->segments[0].value != type_name_) {
      continue;
    }

    // Found matching impl block - search for the method
    for (auto const& method: impl_block.methods) {
      if (method.declaration.name == method_name_) {
        return &method;
      }
    }
  }

  return nullptr;
}

std::optional<std::pair<std::string, ast::Item const*>>
Semantic_Context::resolve_type_name(std::string const& current_module_, ast::Type_Name const& name_) const {
  // Type_Name is a variant: Path_Type, Function_Type, Array_Type, Tuple_Type
  // Path_Type resolves to a type definition (struct, enum, etc.)
  // Compound types (Function_Type, Array_Type, Tuple_Type) recursively validate inner types
  // and return nullopt (they're structural, not named definitions)

  return std::visit(
      [this, &current_module_](auto const& type_variant_) -> std::optional<std::pair<std::string, ast::Item const*>> {
        using T = std::decay_t<decltype(type_variant_)>;

        if constexpr (std::is_same_v<T, ast::Path_Type>) {
          return m_impl->resolve_path_type(*this, current_module_, type_variant_);
        } else if constexpr (std::is_same_v<T, ast::Function_Type>) {
          Impl::resolve_function_type(*this, current_module_, type_variant_);
          return std::nullopt;
        } else if constexpr (std::is_same_v<T, ast::Array_Type>) {
          Impl::resolve_array_type(*this, current_module_, type_variant_);
          return std::nullopt;
        } else if constexpr (std::is_same_v<T, ast::Tuple_Type>) {
          Impl::resolve_tuple_type(*this, current_module_, type_variant_);
          return std::nullopt;
        }
      },
      static_cast<ast::Type_Name::Base_Type const&>(name_)
  );
}

std::optional<std::pair<std::string, ast::Item const*>> Semantic_Context::Impl::resolve_path_type(
    Semantic_Context const& ctx_,
    std::string const& current_module_,
    ast::Path_Type const& path_type_
) const {
  if (path_type_.segments.empty()) {
    return std::nullopt;  // Invalid type name
  }

  auto const& first_segment = path_type_.segments[0].value;

  // Case 1: Single-segment name (e.g., "Point", "Vec<T>")
  if (path_type_.segments.size() == 1) {
    // Also resolve type parameters recursively
    for (auto const& type_param: path_type_.segments[0].type_params) {
      // Recursively resolve each type parameter - validate they resolve
      (void)ctx_.resolve_type_name(current_module_, type_param);
    }

    // Try local module first
    if (auto const* item = ctx_.find_type_def(current_module_, first_segment)) {
      return std::make_pair(current_module_, item);
    }

    // Try imports
    auto const module_it = import_maps.find(current_module_);
    if (module_it != import_maps.end()) {
      auto const& import_map = module_it->second;
      auto const import_it = import_map.find(first_segment);
      if (import_it != import_map.end()) {
        auto const& [source_module, item_name] = import_it->second;
        if (auto const* item = ctx_.find_type_def(source_module, item_name)) {
          if (item->is_pub) {
            return std::make_pair(source_module, item);
          }
          error(
              path_type_.span,
              std::format("cannot import '{}' from module '{}' - not marked pub", item_name, source_module)
          );
          return std::nullopt;
        }
      }
    }
  }
  // Case 2: Multi-segment name (e.g., "Std.Collections.Vec")
  else {
    // Build module path from all segments except the last
    std::string module_path;
    for (size_t i = 0; i < path_type_.segments.size() - 1; ++i) {
      if (i > 0) {
        module_path += '.';
      }
      module_path += path_type_.segments[i].value;
    }

    auto const& type_name = path_type_.segments.back().value;

    // Also resolve type parameters recursively on the last segment
    for (auto const& type_param: path_type_.segments.back().type_params) {
      (void)ctx_.resolve_type_name(current_module_, type_param);
    }

    if (auto const* item = ctx_.find_type_def(module_path, type_name)) {
      if (module_path != current_module_ && !item->is_pub) {
        error(
            path_type_.span,
            std::format("cannot access type '{}' from module '{}' - not marked pub", type_name, module_path)
        );
        return std::nullopt;
      }
      return std::make_pair(module_path, item);
    }
  }

  // Type not found - report error
  error(path_type_.span, std::format("type '{}' not found in current module or imports", first_segment));
  return std::nullopt;
}

void Semantic_Context::Impl::resolve_function_type(
    Semantic_Context const& ctx_,
    std::string const& current_module_,
    ast::Function_Type const& type_
) {
  // Function types are structural - recursively validate inner types
  // fn(I32, String): Bool - validate I32, String, and Bool types

  for (auto const& param_type: type_.param_types) {
    if (param_type) {
      (void)ctx_.resolve_type_name(current_module_, *param_type);
    }
  }

  if (type_.return_type) {
    (void)ctx_.resolve_type_name(current_module_, *type_.return_type);
  }
}

void Semantic_Context::Impl::resolve_array_type(
    Semantic_Context const& ctx_,
    std::string const& current_module_,
    ast::Array_Type const& type_
) {
  // Array types are structural - validate the element type
  // [I32; 5] - validate I32 type

  if (type_.element_type) {
    (void)ctx_.resolve_type_name(current_module_, *type_.element_type);
  }
}

void Semantic_Context::Impl::resolve_tuple_type(
    Semantic_Context const& ctx_,
    std::string const& current_module_,
    ast::Tuple_Type const& type_
) {
  // Tuple types are structural - validate all element types
  // (I32, String, Bool) - validate each element type

  for (auto const& element_type: type_.element_types) {
    (void)ctx_.resolve_type_name(current_module_, element_type);
  }
}

std::optional<std::pair<std::string, ast::Item const*>>
Semantic_Context::resolve_var_name(std::string const& current_module_, ast::Var_Name const& name_) const {
  if (name_.segments.empty()) {
    return std::nullopt;  // Invalid name
  }

  auto const& first_segment = name_.segments[0].value;

  // Validate type parameters on all segments
  for (auto const& segment: name_.segments) {
    for (auto const& type_param: segment.type_params) {
      (void)resolve_type_name(current_module_, type_param);
    }
  }

  // Case 1: Single-segment name (e.g., "calculate", "println", "Vec::<I32>::new")
  if (name_.segments.size() == 1) {
    // Try local module first
    if (auto const* item = find_func_def(current_module_, first_segment)) {
      return std::make_pair(current_module_, item);
    }

    // Try imports
    auto const module_it = m_impl->import_maps.find(current_module_);
    if (module_it != m_impl->import_maps.end()) {
      auto const& import_map = module_it->second;
      auto const import_it = import_map.find(first_segment);
      if (import_it != import_map.end()) {
        auto const& [source_module, item_name] = import_it->second;
        if (auto const* item = find_func_def(source_module, item_name)) {
          if (item->is_pub) {
            return std::make_pair(source_module, item);
          }
          m_impl->error(
              name_.span,
              std::format("cannot import function '{}' from module '{}' - not marked pub", item_name, source_module)
          );
          return std::nullopt;
        }
      }
    }
  }
  // Case 2: Multi-segment name (e.g., "Std.IO.println")
  else {
    std::string module_path;
    for (size_t i = 0; i < name_.segments.size() - 1; ++i) {
      if (i > 0) {
        module_path += '.';
      }
      module_path += name_.segments[i].value;
    }

    auto const& func_name = name_.segments.back().value;

    if (auto const* item = find_func_def(module_path, func_name)) {
      if (module_path != current_module_ && !item->is_pub) {
        m_impl->error(
            name_.span,
            std::format("cannot access function '{}' from module '{}' - not marked pub", func_name, module_path)
        );
        return std::nullopt;
      }
      return std::make_pair(module_path, item);
    }
  }

  // Function not found - report error
  m_impl->error(name_.span, std::format("function '{}' not found in current module or imports", first_segment));
  return std::nullopt;
}

// ============================================================================
// Impl helpers
// ============================================================================

bool Semantic_Context::Impl::check_circular_imports() const {
  // Build dependency graph: module -> set of modules it imports from
  std::map<std::string, std::set<std::string>> dependencies;

  for (auto const& [module_path, module]: modules) {
    auto& deps = dependencies[module_path];  // Create entry even if no imports

    for (auto const& import_stmt: module.imports) {
      // Build source module path string
      std::string source_module;
      for (size_t i = 0; i < import_stmt.module_path.size(); ++i) {
        if (i > 0) {
          source_module += '.';
        }
        source_module += import_stmt.module_path[i];
      }
      deps.insert(source_module);
    }
  }

  // DFS to detect cycles
  // States: 0 = unvisited, 1 = visiting (in current path), 2 = visited (done)
  std::map<std::string, int> visit_state;
  std::vector<std::string> current_path;

  // Recursive DFS lambda
  std::function<bool(std::string const&)> has_cycle = [&](std::string const& module_) -> bool {
    auto const state_it = visit_state.find(module_);
    if (state_it != visit_state.end()) {
      if (state_it->second == 1) {
        // Found cycle - build cycle path for error message
        current_path.push_back(module_);
        return true;
      }
      if (state_it->second == 2) {
        return false;  // Already fully explored, no cycle through this node
      }
    }

    visit_state[module_] = 1;  // Mark as visiting
    current_path.push_back(module_);

    // Visit dependencies
    auto const deps_it = dependencies.find(module_);
    if (deps_it != dependencies.end()) {
      for (auto const& dep: deps_it->second) {
        if (has_cycle(dep)) {
          return true;
        }
      }
    }

    current_path.pop_back();
    visit_state[module_] = 2;  // Mark as fully visited
    return false;
  };

  // Check all modules
  for (auto const& [module_path, _]: modules) {
    current_path.clear();
    if (has_cycle(module_path)) {
      // Build cycle description
      // Find where the cycle starts in current_path
      auto const& cycle_start = current_path.back();
      std::string cycle_desc;
      bool in_cycle = false;
      for (auto const& m: current_path) {
        if (m == cycle_start) {
          in_cycle = true;
        }
        if (in_cycle) {
          if (!cycle_desc.empty()) {
            cycle_desc += " -> ";
          }
          cycle_desc += m;
        }
      }

      // Report error using the first import statement in the cycle
      auto const& module = modules.at(module_path);
      if (!module.imports.empty()) {
        error(module.imports[0].span, std::format("circular import detected: {}", cycle_desc));
      } else {
        // Fallback: use module span
        error(module.span, std::format("circular import detected: {}", cycle_desc));
      }
      return true;
    }
  }

  return false;
}

void Semantic_Context::Impl::build_import_maps() {
  for (auto const& [module_path, module]: modules) {
    auto& import_map = import_maps[module_path];

    for (auto const& import_stmt: module.imports) {
      // Build source module path string
      std::string source_module;
      for (size_t i = 0; i < import_stmt.module_path.size(); ++i) {
        if (i > 0) {
          source_module += '.';
        }
        source_module += import_stmt.module_path[i];
      }

      // Add each imported item to the map
      for (auto const& item: import_stmt.items) {
        std::string const local_name = item.alias.value_or(item.name);
        import_map[local_name] = {source_module, item.name};
      }
    }
  }
}

void Semantic_Context::Impl::build_name_indices() {
  for (auto const& [module_path, module]: modules) {
    for (auto const& item: module.items) {
      auto const name_opt = get_item_name(item);
      if (!name_opt.has_value()) {
        continue;  // Skip items without names (e.g., impl blocks)
      }

      auto const& name = *name_opt;
      auto const key = std::make_pair(module_path, name);

      // Categorize by item type
      bool const is_type_def = std::visit(
          [](auto const& stmt_) -> bool {
            using T = std::decay_t<decltype(stmt_)>;
            return std::same_as<T, std::shared_ptr<ast::Struct_Def>> ||
                   std::same_as<T, std::shared_ptr<ast::Enum_Def>> ||
                   std::same_as<T, std::shared_ptr<ast::Trait_Def>> ||
                   std::same_as<T, std::shared_ptr<ast::Type_Alias>>;
          },
          item.item
      );

      bool const is_func_def = std::holds_alternative<std::shared_ptr<ast::Func_Def>>(item.item);

      if (is_type_def) {
        type_index[key] = &item;
      }
      if (is_func_def) {
        func_index[key] = &item;
      }
    }
  }
}

bool Semantic_Context::Impl::item_matches_name(ast::Item const& item_, std::string_view name_) {
  auto const item_name = get_item_name(item_);
  return item_name.has_value() && item_name.value() == name_;
}

std::optional<std::string> Semantic_Context::Impl::get_item_name(ast::Item const& item_) {
  return std::visit(
      [](auto const& stmt_) -> std::optional<std::string> {
        using T = std::decay_t<decltype(stmt_)>;
        if constexpr (std::same_as<T, std::shared_ptr<ast::Func_Def>>) {
          return stmt_->declaration.name;
        } else if constexpr (std::same_as<T, std::shared_ptr<ast::Struct_Def>> ||
                             std::same_as<T, std::shared_ptr<ast::Enum_Def>> ||
                             std::same_as<T, std::shared_ptr<ast::Trait_Def>> ||
                             std::same_as<T, std::shared_ptr<ast::Type_Alias>>) {
          return stmt_->name;
        } else {
          return std::nullopt;
        }
      },
      item_.item
  );
}

}  // namespace life_lang::semantic
