#pragma once

#include "../parser/ast.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace life_lang::semantic {

// Semantic analysis context - manages all loaded modules and provides name resolution
class Semantic_Context {
public:
  // src_root_: Filesystem path to source directory
  // Returns false if any module fails to parse
  bool load_modules(std::filesystem::path const& src_root_);

  // Get a loaded module by dot-separated module path (e.g., "Std.Collections")
  [[nodiscard]] ast::Module const* get_module(std::string const& module_path_) const;

  // Get all loaded module paths (dot-separated strings like "Std.Collections")
  [[nodiscard]] std::vector<std::string> module_paths() const;

  // Find a type definition (struct/enum/trait/type alias) in a specific module
  // Returns nullptr if not found or not a type definition
  // Only searches module-level items, not nested definitions
  [[nodiscard]] ast::Item const* find_type_def(std::string const& module_path_, std::string_view type_name_) const;

  // Find a function definition in a specific module
  // Returns nullptr if not found or not a function
  // Only searches module-level items, not methods in impl blocks
  [[nodiscard]] ast::Item const* find_func_def(std::string const& module_path_, std::string_view func_name_) const;

  // Resolve a type name within a module's context
  // current_module_: Dot-separated module path (e.g., "Geometry")
  // name_: Type name from AST to resolve
  // Returns (module_path, Item*) pair if found, where module_path is dot-separated
  // Examples:
  //   - "I32" -> built-in type (no module path, nullptr)
  //   - "Point" -> local definition or imported
  //   - "Std.Collections.Vec" -> fully qualified import
  [[nodiscard]] std::optional<std::pair<std::string, ast::Item const*>>
  resolve_type_name(std::string const& current_module_, ast::Type_Name const& name_) const;

  // Resolve a variable/function name within a module's context
  // current_module_: Dot-separated module path (e.g., "Geometry")
  // name_: Variable/function name from AST to resolve
  // Returns (module_path, Item*) pair if found, where module_path is dot-separated
  [[nodiscard]] std::optional<std::pair<std::string, ast::Item const*>>
  resolve_var_name(std::string const& current_module_, ast::Var_Name const& name_) const;

private:
  // Module path (dot-separated like "Std.Collections") -> parsed AST
  std::map<std::string, ast::Module> m_modules;

  // Helper: Check if a name matches an item
  [[nodiscard]] static bool item_matches_name(ast::Item const& item_, std::string_view name_);

  // Helper: Get the name of an item (function name, struct name, etc.)
  [[nodiscard]] static std::optional<std::string> get_item_name(ast::Item const& item_);
};

}  // namespace life_lang::semantic
