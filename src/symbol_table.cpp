#include "symbol_table.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <algorithm>
#include <ranges>
#include <sstream>

namespace life_lang {

// ============================================================================
// Symbol_Kind
// ============================================================================

std::string to_string(Symbol_Kind a_kind) {
  switch (a_kind) {
    case Symbol_Kind::Variable:
      return "variable";
    case Symbol_Kind::Function:
      return "function";
    case Symbol_Kind::Parameter:
      return "parameter";
    case Symbol_Kind::Type:
      return "type";
    case Symbol_Kind::Trait:
      return "trait";
    case Symbol_Kind::Type_Param:
      return "type_parameter";
    case Symbol_Kind::Field:
      return "field";
    case Symbol_Kind::Variant:
      return "variant";
    case Symbol_Kind::Module:
      return "module";
  }
  return "unknown";
}

// ============================================================================
// Source_Location
// ============================================================================

std::string to_string(Source_Location const& a_loc) {
  return fmt::format("{}:{}:{}", a_loc.file, a_loc.line, a_loc.column);
}

// ============================================================================
// Symbol
// ============================================================================

std::string to_string(Symbol const& a_symbol) {
  auto result = fmt::format("{} '{}': {}", to_string(a_symbol.kind), a_symbol.name, a_symbol.type_annotation);
  if (!a_symbol.generic_params.empty()) {
    result += fmt::format("<{}>", fmt::join(a_symbol.generic_params, ", "));
  }
  return result;
}

// ============================================================================
// Scope_Kind
// ============================================================================

std::string to_string(Scope_Kind a_kind) {
  switch (a_kind) {
    case Scope_Kind::Global:
      return "global";
    case Scope_Kind::Function:
      return "function";
    case Scope_Kind::Block:
      return "block";
    case Scope_Kind::Impl:
      return "impl";
    case Scope_Kind::Trait:
      return "trait";
    case Scope_Kind::Match_Arm:
      return "match_arm";
    case Scope_Kind::Loop:
      return "loop";
  }
  return "unknown";
}

// ============================================================================
// Scope
// ============================================================================

Scope::Scope(Scope_Kind a_kind, Scope* a_parent) : m_kind(a_kind), m_parent(a_parent) {}

bool Scope::insert(Symbol const& a_symbol) {
  auto const [iter, inserted] = m_symbols.try_emplace(a_symbol.name, a_symbol);
  return inserted;
}

Symbol const* Scope::lookup_local(std::string const& a_name) const {
  auto const iter = m_symbols.find(a_name);
  if (iter != m_symbols.end()) {
    return &iter->second;
  }
  return nullptr;
}

bool Scope::contains_local(std::string const& a_name) const {
  return m_symbols.contains(a_name);
}

// ============================================================================
// Symbol_Table
// ============================================================================

Symbol_Table::Symbol_Table() {
  // Create global scope
  auto global = std::make_unique<Scope>(Scope_Kind::Global);
  m_scope_stack.push_back(global.get());
  m_scopes.push_back(std::move(global));
}

void Symbol_Table::enter_scope(Scope_Kind a_kind) {
  auto* parent = m_scope_stack.back();
  auto new_scope = std::make_unique<Scope>(a_kind, parent);
  m_scope_stack.push_back(new_scope.get());
  m_scopes.push_back(std::move(new_scope));
}

void Symbol_Table::exit_scope() {
  if (m_scope_stack.size() <= 1) {
    // Cannot exit global scope
    return;
  }
  m_scope_stack.pop_back();
}

Scope const* Symbol_Table::current_scope() const {
  return m_scope_stack.back();
}

Scope_Kind Symbol_Table::current_scope_kind() const {
  return current_scope()->kind();
}

bool Symbol_Table::insert(Symbol const& a_symbol) {
  return m_scope_stack.back()->insert(a_symbol);
}

Symbol const* Symbol_Table::lookup(std::string const& a_name) const {
  // Search from current scope up to global scope
  for (auto const* scope : m_scope_stack | std::views::reverse) {
    if (auto const* symbol = scope->lookup_local(a_name)) {
      return symbol;
    }
  }
  return nullptr;
}

bool Symbol_Table::is_type_symbol(Symbol const& a_symbol) {
  return a_symbol.kind == Symbol_Kind::Type || a_symbol.kind == Symbol_Kind::Trait ||
         a_symbol.kind == Symbol_Kind::Type_Param;
}

bool Symbol_Table::is_value_symbol(Symbol const& a_symbol) {
  return a_symbol.kind == Symbol_Kind::Variable || a_symbol.kind == Symbol_Kind::Function ||
         a_symbol.kind == Symbol_Kind::Parameter;
}

Symbol const* Symbol_Table::lookup_type(std::string const& a_name) const {
  auto const* symbol = lookup(a_name);
  if (symbol != nullptr && is_type_symbol(*symbol)) {
    return symbol;
  }
  return nullptr;
}

Symbol const* Symbol_Table::lookup_value(std::string const& a_name) const {
  auto const* symbol = lookup(a_name);
  if (symbol != nullptr && is_value_symbol(*symbol)) {
    return symbol;
  }
  return nullptr;
}

bool Symbol_Table::in_impl_scope() const {
  return std::ranges::any_of(m_scope_stack, [](auto const* a_scope) { return a_scope->kind() == Scope_Kind::Impl; });
}

bool Symbol_Table::in_function_scope() const {
  return std::ranges::any_of(m_scope_stack, [](auto const* a_scope) {
    return a_scope->kind() == Scope_Kind::Function;
  });
}

bool Symbol_Table::in_loop_scope() const {
  return std::ranges::any_of(m_scope_stack, [](auto const* a_scope) { return a_scope->kind() == Scope_Kind::Loop; });
}

std::string to_string(Symbol_Table const& a_table) {
  std::ostringstream oss;
  oss << "Symbol Table:\n";

  for (std::size_t i = 0; i < a_table.m_scope_stack.size(); ++i) {
    auto const* scope = a_table.m_scope_stack[i];
    auto const indent = std::string(i * 2, ' ');
    oss << indent << to_string(scope->kind()) << " scope:\n";

    for (auto const& [name, symbol] : scope->symbols()) {
      oss << indent << "  " << to_string(symbol) << " @ " << to_string(symbol.location) << "\n";
    }
  }

  return oss.str();
}

}  // namespace life_lang
