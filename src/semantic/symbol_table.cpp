// Implementation of symbol table

#include "symbol_table.hpp"

namespace life_lang::semantic {

// ============================================================================
// Scope Implementation
// ============================================================================

std::optional<std::string> Scope::declare(std::string name_, Symbol symbol_) {
  if (contains(name_)) {
    return "Symbol '" + name_ + "' already declared in this scope at " + m_symbols.at(name_).location.filename + ":" +
           std::to_string(m_symbols.at(name_).location.position.line);
  }

  m_symbols[std::move(name_)] = std::move(symbol_);
  return std::nullopt;
}

std::optional<Symbol> Scope::lookup_local(std::string_view name_) const {
  if (auto it = m_symbols.find(std::string(name_)); it != m_symbols.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<Symbol> Scope::lookup(std::string_view name_) const {
  // Search current scope
  if (auto sym = lookup_local(name_)) {
    return sym;
  }

  // Search parent chain
  if (m_parent != nullptr) {
    return m_parent->lookup(name_);
  }

  return std::nullopt;
}

bool Scope::contains(std::string_view name_) const {
  return m_symbols.contains(std::string(name_));
}

// ============================================================================
// Symbol_Table Implementation
// ============================================================================

Scope* Symbol_Table::create_module_scope(std::string const& module_path_) {
  if (auto it = m_modules.find(module_path_); it != m_modules.end()) {
    return it->second.get();
  }

  auto scope = std::make_unique<Scope>(Scope_Kind::Module);
  auto* ptr = scope.get();
  m_modules[module_path_] = std::move(scope);
  return ptr;
}

Scope* Symbol_Table::get_module_scope(std::string const& module_path_) {
  if (auto it = m_modules.find(module_path_); it != m_modules.end()) {
    return it->second.get();
  }
  return nullptr;
}

Scope const* Symbol_Table::get_module_scope(std::string const& module_path_) const {
  if (auto it = m_modules.find(module_path_); it != m_modules.end()) {
    return it->second.get();
  }
  return nullptr;
}

void Symbol_Table::enter_scope(Scope_Kind kind_) {
  Scope const* const parent = m_scope_stack.empty() ? nullptr : m_scope_stack.back();
  auto scope = std::make_unique<Scope>(kind_, parent);
  m_scope_stack.push_back(scope.get());
  m_scope_storage.push_back(std::move(scope));
}

void Symbol_Table::exit_scope() {
  if (!m_scope_stack.empty()) {
    m_scope_stack.pop_back();
  }
}

Scope* Symbol_Table::current_scope() {
  if (m_scope_stack.empty()) {
    return nullptr;
  }
  return m_scope_stack.back();
}

Scope const* Symbol_Table::current_scope() const {
  if (m_scope_stack.empty()) {
    return nullptr;
  }
  return m_scope_stack.back();
}

std::optional<std::string> Symbol_Table::declare(std::string name_, Symbol symbol_) {
  auto* scope = current_scope();
  if (scope == nullptr) {
    return "No active scope for declaration";
  }
  return scope->declare(std::move(name_), std::move(symbol_));
}

std::optional<Symbol> Symbol_Table::lookup(std::string_view name_) const {
  auto const* scope = current_scope();
  if (scope == nullptr) {
    return std::nullopt;
  }
  return scope->lookup(name_);
}

std::optional<Symbol> Symbol_Table::lookup_local(std::string_view name_) const {
  auto const* scope = current_scope();
  if (scope == nullptr) {
    return std::nullopt;
  }
  return scope->lookup_local(name_);
}

}  // namespace life_lang::semantic
