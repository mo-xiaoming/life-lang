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
Semantic_Context::resolve_type_name(std::string const& /*current_module_*/, ast::Type_Name const& /*name_*/) const {
  (void)this;
  // For now, simplified implementation:
  // - Type names are complex (variant of Path_Type, Function_Type, Array_Type, Tuple_Type)
  // - Need to extract the base type name from Path_Type
  // TODO(mx): Implement full type name resolution with imports
  return std::nullopt;
}

std::optional<std::pair<std::string, ast::Item const*>>
Semantic_Context::resolve_var_name(std::string const& current_module_, ast::Var_Name const& name_) const {
  // Var_Name has segments (std::vector<Var_Name_Segment>)
  // For now, simplified: only handle simple single-segment names
  if (name_.segments.size() == 1 && name_.segments[0].type_params.empty()) {
    auto const& var_name = name_.segments[0].value;

    // Try finding function in local module
    if (auto const* item = find_func_def(current_module_, var_name)) {
      return std::make_pair(current_module_, item);
    }

    // Try imports
    // TODO(mx): Full import resolution
  }

  // Multi-segment case: fully qualified path
  // TODO(mx): Implement fully qualified path resolution

  return std::nullopt;
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
