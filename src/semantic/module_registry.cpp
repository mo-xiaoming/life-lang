// Implementation of module registry

#include "module_registry.hpp"

#include <unordered_set>

namespace life_lang::semantic {

bool Module_Registry::register_module(std::string const& path_, ast::Module const* ast_) {
  // Check if already exists
  if (m_modules.contains(path_)) {
    return false;
  }

  m_modules[path_] = Module_Info{
      .path = path_,
      .ast = ast_,
      .dependencies = {},
  };

  return true;
}

Module_Info const* Module_Registry::get_module(std::string const& path_) const {
  auto it = m_modules.find(path_);
  if (it == m_modules.end()) {
    return nullptr;
  }
  return &it->second;
}

std::vector<std::string> Module_Registry::all_module_paths() const {
  std::vector<std::string> paths;
  paths.reserve(m_modules.size());
  for (auto const& [path, _]: m_modules) {
    paths.push_back(path);
  }
  return paths;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Module_Registry::add_dependency(std::string const& from_module_, std::string const& to_module_) {
  auto it = m_modules.find(from_module_);
  if (it != m_modules.end()) {
    it->second.dependencies.push_back(to_module_);
  }
}

bool Module_Registry::has_module(std::string const& path_) const {
  return m_modules.contains(path_);
}

std::vector<std::string> Module_Registry::topological_sort() const {
  // Kahn's algorithm for topological sorting
  std::unordered_map<std::string, std::unordered_set<std::string>> adj_list;
  std::unordered_map<std::string, int> in_degree;

  // Build adjacency list and in-degree map
  for (auto const& [module_path, info]: m_modules) {
    in_degree[module_path] = 0;  // Initialize all nodes
    adj_list[module_path];       // Create empty adjacency list
  }

  for (auto const& [module_path, info]: m_modules) {
    for (auto const& dep: info.dependencies) {
      adj_list[dep].insert(module_path);  // dep -> module_path edge
      in_degree[module_path]++;
    }
  }

  // Find all nodes with in-degree 0
  std::vector<std::string> queue;
  for (auto const& [module_path, degree]: in_degree) {
    if (degree == 0) {
      queue.push_back(module_path);
    }
  }

  std::vector<std::string> result;
  result.reserve(m_modules.size());

  while (!queue.empty()) {
    std::string const current = queue.back();
    queue.pop_back();
    result.push_back(current);

    // Reduce in-degree for all neighbors
    for (auto const& neighbor: adj_list[current]) {
      in_degree[neighbor]--;
      if (in_degree[neighbor] == 0) {
        queue.push_back(neighbor);
      }
    }
  }

  // If we didn't process all nodes, there's a cycle
  if (result.size() != m_modules.size()) {
    return {};  // Circular dependency detected
  }

  return result;
}

}  // namespace life_lang::semantic
