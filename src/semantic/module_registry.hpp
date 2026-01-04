// Module registry for tracking all loaded modules
// Manages module metadata, dependencies, and lookup

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace life_lang::ast {
struct Module;
}

namespace life_lang::semantic {

// Information about a loaded module
struct Module_Info {
  std::string path;                       // Dot-separated path: "Std.Collections"
  ast::Module const* ast{nullptr};        // Non-owning pointer to parsed AST
  std::vector<std::string> dependencies;  // Imported module paths
  // TODO(mx): Add Source_Location for first file when needed
};

// Registry for all discovered and loaded modules
struct Module_Registry {
  // Register a module with its AST
  // Returns false if module already exists
  [[nodiscard]] bool register_module(std::string const& path_, ast::Module const* ast_);

  // Look up a module by its dot-separated path
  // Returns nullptr if not found
  [[nodiscard]] Module_Info const* get_module(std::string const& path_) const;

  // Get all registered module paths
  [[nodiscard]] std::vector<std::string> all_module_paths() const;

  // Add a dependency edge (from_module imports to_module)
  void add_dependency(std::string const& from_module_, std::string const& to_module_);

  // Build topologically sorted list of modules (dependencies first)
  // Returns empty vector if circular dependency detected
  [[nodiscard]] std::vector<std::string> topological_sort() const;

  // Check if a module exists
  [[nodiscard]] bool has_module(std::string const& path_) const;

private:
  std::unordered_map<std::string, Module_Info> m_modules;
};

}  // namespace life_lang::semantic
