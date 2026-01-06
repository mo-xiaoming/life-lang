#include "semantic_context.hpp"
#include <algorithm>
#include "module_loader.hpp"

namespace life_lang::semantic {

bool Semantic_Context::load_modules(std::filesystem::path const& src_root_) {
  // Discover all modules in src/ directory
  auto const descriptors = Module_Loader::discover_modules(src_root_);

  // Load and parse each module
  for (auto const& desc: descriptors) {
    auto module_opt = Module_Loader::load_module(desc);
    if (!module_opt.has_value()) {
      return false;  // Parse error
    }
    m_modules[desc.module_path_string()] = std::move(*module_opt);
  }

  // Build import maps for cross-module name resolution
  build_import_maps();

  return true;
}

ast::Module const* Semantic_Context::get_module(std::string const& module_path_) const {
  auto const it = m_modules.find(module_path_);
  if (it == m_modules.end()) {
    return nullptr;
  }
  return &it->second;
}

std::vector<std::string> Semantic_Context::module_paths() const {
  std::vector<std::string> paths;
  paths.reserve(m_modules.size());
  std::ranges::transform(m_modules, std::back_inserter(paths), [](auto const& pair_) { return pair_.first; });
  return paths;
}

ast::Item const* Semantic_Context::find_type_def(std::string const& module_path_, std::string_view type_name_) const {
  auto const* module = get_module(module_path_);
  if (module == nullptr) {
    return nullptr;
  }

  // Search module-level items for matching type definition
  for (auto const& item: module->items) {
    // Check if this is a type definition (struct, enum, trait, type alias)
    // These are stored as shared_ptr in the Statement variant
    bool const is_type_def = std::visit(
        [](auto const& stmt_) -> bool {
          using T = std::decay_t<decltype(stmt_)>;
          return std::same_as<T, std::shared_ptr<ast::Struct_Def>> || std::same_as<T, std::shared_ptr<ast::Enum_Def>> ||
                 std::same_as<T, std::shared_ptr<ast::Trait_Def>> || std::same_as<T, std::shared_ptr<ast::Type_Alias>>;
        },
        item.item
    );

    if (is_type_def && item_matches_name(item, type_name_)) {
      return &item;
    }
  }

  return nullptr;
}

ast::Item const* Semantic_Context::find_func_def(std::string const& module_path_, std::string_view func_name_) const {
  auto const* module = get_module(module_path_);
  if (module == nullptr) {
    return nullptr;
  }

  // Search module-level items for matching function definition
  for (auto const& item: module->items) {
    // Check if this is a function definition (stored as shared_ptr)
    bool const is_func = std::holds_alternative<std::shared_ptr<ast::Func_Def>>(item.item);

    if (is_func && item_matches_name(item, func_name_)) {
      return &item;
    }
  }

  return nullptr;
}

std::optional<std::pair<std::string, ast::Item const*>>
Semantic_Context::resolve_type_name(std::string const& current_module_, ast::Type_Name const& name_) const {
  // Type_Name is a variant: Path_Type, Function_Type, Array_Type, Tuple_Type
  // For now, only resolve Path_Type (simple and parameterized type names)
  // Function types, array types, and tuple types don't need resolution - they're built-in syntax

  if (!std::holds_alternative<ast::Path_Type>(name_)) {
    return std::nullopt;  // Not a path-based type
  }

  auto const& path_type = std::get<ast::Path_Type>(name_);
  if (path_type.segments.empty()) {
    return std::nullopt;  // Invalid type name
  }

  // Get the first segment's value (e.g., "Point" in "Point" or "Point<I32>")
  auto const& first_segment = path_type.segments[0].value;

  // Case 1: Single-segment name (e.g., "Point", "Vec<T>")
  if (path_type.segments.size() == 1) {
    // Try local module first
    if (auto const* item = find_type_def(current_module_, first_segment)) {
      return std::make_pair(current_module_, item);
    }

    // Try imports
    auto const module_it = m_import_maps.find(current_module_);
    if (module_it != m_import_maps.end()) {
      auto const& import_map = module_it->second;
      auto const import_it = import_map.find(first_segment);
      if (import_it != import_map.end()) {
        // Found in imports: (source_module, item_name)
        auto const& [source_module, item_name] = import_it->second;
        if (auto const* item = find_type_def(source_module, item_name)) {
          // Verify visibility: must be pub
          if (item->is_pub) {
            return std::make_pair(source_module, item);
          }
          // TODO(mx): Add diagnostic - cannot import non-pub item
        }
      }
    }
  }
  // Case 2: Multi-segment name (e.g., "Std.Collections.Vec")
  else {
    // Build module path from all segments except the last
    std::string module_path;
    for (size_t i = 0; i < path_type.segments.size() - 1; ++i) {
      if (i > 0) {
        module_path += '.';
      }
      module_path += path_type.segments[i].value;
    }

    // Last segment is the type name
    auto const& type_name = path_type.segments.back().value;

    // Direct lookup in the specified module
    if (auto const* item = find_type_def(module_path, type_name)) {
      // Verify visibility: must be pub for cross-module access
      if (module_path != current_module_ && !item->is_pub) {
        // TODO(mx): Add diagnostic - cannot access non-pub item from other module
        return std::nullopt;
      }
      return std::make_pair(module_path, item);
    }
  }

  return std::nullopt;  // Not found
}

std::optional<std::pair<std::string, ast::Item const*>>
Semantic_Context::resolve_var_name(std::string const& current_module_, ast::Var_Name const& name_) const {
  // Var_Name has segments (std::vector<Var_Name_Segment>)
  if (name_.segments.empty()) {
    return std::nullopt;  // Invalid name
  }

  auto const& first_segment = name_.segments[0].value;

  // Case 1: Single-segment name (e.g., "calculate", "println")
  if (name_.segments.size() == 1 && name_.segments[0].type_params.empty()) {
    // Try local module first
    if (auto const* item = find_func_def(current_module_, first_segment)) {
      return std::make_pair(current_module_, item);
    }

    // Try imports
    auto const module_it = m_import_maps.find(current_module_);
    if (module_it != m_import_maps.end()) {
      auto const& import_map = module_it->second;
      auto const import_it = import_map.find(first_segment);
      if (import_it != import_map.end()) {
        // Found in imports: (source_module, item_name)
        auto const& [source_module, item_name] = import_it->second;
        if (auto const* item = find_func_def(source_module, item_name)) {
          // Verify visibility: must be pub
          if (item->is_pub) {
            return std::make_pair(source_module, item);
          }
          // TODO(mx): Add diagnostic - cannot import non-pub item
        }
      }
    }
  }
  // Case 2: Multi-segment name (e.g., "Std.IO.println")
  else if (name_.segments[0].type_params.empty()) {
    // Build module path from all segments except the last
    std::string module_path;
    for (size_t i = 0; i < name_.segments.size() - 1; ++i) {
      if (i > 0) {
        module_path += '.';
      }
      module_path += name_.segments[i].value;
    }

    // Last segment is the function name
    auto const& func_name = name_.segments.back().value;

    // Direct lookup in the specified module
    if (auto const* item = find_func_def(module_path, func_name)) {
      // Verify visibility: must be pub for cross-module access
      if (module_path != current_module_ && !item->is_pub) {
        // TODO(mx): Add diagnostic - cannot access non-pub item from other module
        return std::nullopt;
      }
      return std::make_pair(module_path, item);
    }
  }

  return std::nullopt;  // Not found or has type params (method call, not function)
}

void Semantic_Context::build_import_maps() {
  // Build import map for each loaded module
  for (auto const& [module_path, module]: m_modules) {
    auto& import_map = m_import_maps[module_path];

    for (auto const& import_stmt: module.imports) {
      // Build source module path string (e.g., ["Geometry", "Shapes"] -> "Geometry.Shapes")
      std::string source_module;
      for (size_t i = 0; i < import_stmt.module_path.size(); ++i) {
        if (i > 0) {
          source_module += '.';
        }
        source_module += import_stmt.module_path[i];
      }

      // Add each imported item to the map
      for (auto const& item: import_stmt.items) {
        // Use alias if provided, otherwise use original name
        std::string const local_name = item.alias.value_or(item.name);
        import_map[local_name] = {source_module, item.name};
      }
    }
  }
}

// Helper implementations

bool Semantic_Context::item_matches_name(ast::Item const& item_, std::string_view name_) {
  auto const item_name = get_item_name(item_);
  return item_name.has_value() && item_name.value() == name_;
}

std::optional<std::string> Semantic_Context::get_item_name(ast::Item const& item_) {
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
