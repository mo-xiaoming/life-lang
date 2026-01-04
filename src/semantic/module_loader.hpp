// Module discovery and loading from filesystem
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace life_lang {
struct Diagnostic_Engine;
namespace ast {
struct Module;
}
}  // namespace life_lang

namespace life_lang::semantic {

// Describes a module discovered from the filesystem
struct Module_Descriptor {
  std::vector<std::string> path;             // Module path components: {"Std", "Collections"}
  std::filesystem::path directory;           // Filesystem directory containing module files
  std::vector<std::filesystem::path> files;  // All .life files in the module

  // Get simple module name (last component of path)
  [[nodiscard]] std::string name() const { return path.empty() ? "" : path.back(); }

  // Get dot-separated path string for display/serialization
  [[nodiscard]] std::string path_string() const;

  // Helper for debugging/logging
  [[nodiscard]] std::string to_string() const;
};

// Module loader for filesystem-based module discovery
class Module_Loader {
public:
  // Convert lowercase_snake_case directory name to Camel_Snake_Case module name
  // Examples: "geometry" -> "Geometry", "user_profile" -> "User_Profile"
  [[nodiscard]] static std::string dir_name_to_module_name(std::string_view dir_name_);

  // Derive module path components from directory relative to src/ root
  // Both paths are canonicalized (absolute + symlinks resolved) to ensure comparability
  // Examples:
  //   src_root=/project/src, module_dir=/project/src/geometry -> {"Geometry"}
  //   src_root=/project/src, module_dir=/project/src/std/math -> {"Std", "Math"}
  [[nodiscard]] static std::vector<std::string>
  derive_module_path(std::filesystem::path const& src_root_, std::filesystem::path const& module_dir_);

  // Scan src/ directory recursively to find all modules
  // Returns list of all discovered modules (both file-modules and folder-modules)
  // src_root: Path to "src/" directory (can be relative or absolute, will be canonicalized)
  [[nodiscard]] static std::vector<Module_Descriptor> discover_modules(std::filesystem::path const& src_root_);

  // Load and parse all files in a module
  // Parses each .life file and merges all top-level items into a single Module AST
  // Returns the merged module on success, or std::nullopt if any file fails to parse
  [[nodiscard]] static std::optional<ast::Module> load_module(Module_Descriptor const& descriptor_);
};

}  // namespace life_lang::semantic
