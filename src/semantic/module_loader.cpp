// Implementation of module discovery and loading

#include "module_loader.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <sstream>

namespace life_lang::semantic {

std::string Module_Descriptor::path_string() const {
  if (path.empty()) {
    return "";
  }
  std::ostringstream oss;
  for (size_t i = 0; i < path.size(); ++i) {
    if (i > 0) {
      oss << '.';
    }
    oss << path[i];
  }
  return oss.str();
}

std::string Module_Descriptor::to_string() const {
  return std::format(
      "Module(name='{}', path='{}', dir='{}', {} files)",
      name(),
      path_string(),
      directory.string(),
      files.size()
  );
}

std::string Module_Loader::dir_name_to_module_name(std::string_view dir_name_) {
  if (dir_name_.empty()) {
    return "";
  }

  std::string result;
  result.reserve(dir_name_.size());
  bool capitalize_next = true;

  for (char const c: dir_name_) {
    if (c == '_') {
      result += '_';
      capitalize_next = true;
    } else if (capitalize_next) {
      result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      capitalize_next = false;
    } else {
      result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
  }

  return result;
}

std::vector<std::string>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Module_Loader::derive_module_path(std::filesystem::path const& src_root_, std::filesystem::path const& module_dir_) {
  // Canonicalize both paths to ensure they're comparable
  // (Both must be in same resolved space for lexically_relative to work)
  auto canonical_src_root = std::filesystem::canonical(src_root_);

  // Check if module_dir is a symlink - reject it to avoid confusion
  // (Module path would use target name, but user sees symlink name)
  if (std::filesystem::is_symlink(module_dir_)) {
    // Return empty path to signal error
    return {};
  }

  auto canonical_module_dir = std::filesystem::canonical(module_dir_);

  // Get path relative to src/ root
  auto relative = canonical_module_dir.lexically_relative(canonical_src_root);

  // Build module path from directory structure
  std::vector<std::string> path_components;
  for (auto const& component: relative) {
    if (component != ".") {
      path_components.push_back(dir_name_to_module_name(component.string()));
    }
  }

  return path_components;
}

std::vector<Module_Descriptor> Module_Loader::discover_modules(std::filesystem::path const& src_root_) {
  std::vector<Module_Descriptor> modules;

  if (!std::filesystem::exists(src_root_) || !std::filesystem::is_directory(src_root_)) {
    return modules;
  }

  // Canonicalize src_root to absolute path for consistent comparisons
  auto canonical_src_root = std::filesystem::canonical(src_root_);

  // Recursively scan src/ directory
  for (auto const& entry: std::filesystem::recursive_directory_iterator(canonical_src_root)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    // Check if it's a .life file
    if (entry.path().extension() != ".life") {
      continue;
    }

    auto const& file_path = entry.path();
    auto const parent_dir = file_path.parent_path();

    // Check if we already have a module for this directory
    auto it = std::ranges::find_if(modules, [&](auto const& mod_) { return mod_.directory == parent_dir; });

    if (it != modules.end()) {
      // Add file to existing module
      it->files.push_back(file_path);
    } else {
      // Create new module descriptor
      // Derive module path components from directory relative to src/
      auto path_components = derive_module_path(canonical_src_root, parent_dir);

      modules.push_back(
          Module_Descriptor{
              .path = std::move(path_components),
              .directory = parent_dir,
              .files = {file_path},
          }
      );
    }
  }

  return modules;
}

}  // namespace life_lang::semantic
