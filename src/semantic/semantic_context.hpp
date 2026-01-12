#pragma once

#include "../parser/ast.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace life_lang {
class Diagnostic_Manager;
}  // namespace life_lang

namespace life_lang::semantic {

// Semantic analysis context - manages loaded modules and provides name resolution
// Uses pimpl idiom to hide implementation details.
class Semantic_Context {
public:
  // Takes a reference to Diagnostic_Manager for error reporting
  explicit Semantic_Context(Diagnostic_Manager& diagnostics_);

  // Non-copyable, movable
  Semantic_Context(Semantic_Context const&) = delete;
  Semantic_Context& operator=(Semantic_Context const&) = delete;
  Semantic_Context(Semantic_Context&&) noexcept;
  Semantic_Context& operator=(Semantic_Context&&) noexcept;
  ~Semantic_Context();

  // Load all modules from src/ directory
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

  // Find a method definition within impl blocks for a specific type
  // type_name_: Simple type name (e.g., "Point") - not fully qualified
  // method_name_: Method name to find (e.g., "distance")
  // Returns nullptr if not found
  [[nodiscard]] ast::Func_Def const*
  find_method_def(std::string const& module_path_, std::string_view type_name_, std::string_view method_name_) const;

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
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

}  // namespace life_lang::semantic
