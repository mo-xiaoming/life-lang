// Symbol table for semantic analysis
// Tracks all declarations (variables, functions, types) across scopes

#pragma once

#include "../diagnostics.hpp"
#include "type.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace life_lang::semantic {

// ============================================================================
// Symbol Source Location
// ============================================================================

// Combines filename with position for symbol declarations
struct Symbol_Location {
  std::string filename;
  Source_Position position;

  [[nodiscard]] bool operator==(Symbol_Location const& other_) const = default;
};

// ============================================================================
// Symbol Visibility
// ============================================================================

enum class Visibility : std::uint8_t {
  Module_Internal,  // No 'pub' keyword - visible within module only
  Public,           // 'pub' keyword - exported from module
};

// ============================================================================
// Symbol Kinds
// ============================================================================

enum class Symbol_Kind : std::uint8_t {
  Variable,  // let bindings, function parameters
  Function,  // Function declarations
  Type,      // Struct, enum, trait, type alias names
  Module,    // Module names (for imports)
  Field,     // Struct fields (for field access checking)
  Variant,   // Enum variants
};

// ============================================================================
// Symbol
// ============================================================================

// Represents a declared symbol (variable, function, type, etc.)
struct Symbol {
  std::string name;
  Symbol_Kind kind{Symbol_Kind::Module};
  Type type;  // Type of the symbol
  Visibility visibility{Visibility::Module_Internal};
  Symbol_Location location;  // Where declared

  // Additional metadata per symbol kind
  // For functions: parameter count, generic params
  // For types: fields, variants, etc.
  // TODO(mx): Extend with metadata as needed

  [[nodiscard]] bool operator==(Symbol const& other_) const = default;
};

// ============================================================================
// Scope
// ============================================================================

enum class Scope_Kind : std::uint8_t {
  Module,    // Top-level module scope
  Block,     // Block scope (let bindings)
  Function,  // Function scope (parameters, local vars)
};

// Represents a namespace for symbols
struct Scope {
  explicit Scope(Scope_Kind kind_, Scope const* parent_ = nullptr) : m_kind(kind_), m_parent(parent_) {}

  // Declare a symbol in this scope
  // Returns error message on failure, std::nullopt on success
  [[nodiscard]] std::optional<std::string> declare(std::string name_, Symbol symbol_);

  // Lookup symbol in this scope only (no parent search)
  [[nodiscard]] std::optional<Symbol> lookup_local(std::string_view name_) const;

  // Lookup symbol in this scope and parent chain
  [[nodiscard]] std::optional<Symbol> lookup(std::string_view name_) const;

  // Check if symbol exists in this scope (local only)
  [[nodiscard]] bool contains(std::string_view name_) const;

  [[nodiscard]] Scope_Kind kind() const { return m_kind; }
  [[nodiscard]] Scope const* parent() const { return m_parent; }
  [[nodiscard]] std::unordered_map<std::string, Symbol> const& symbols() const { return m_symbols; }

private:
  Scope_Kind m_kind;
  Scope const* m_parent;
  std::unordered_map<std::string, Symbol> m_symbols;
};

// ============================================================================
// Symbol_Table
// ============================================================================

// Top-level symbol table manager
// Manages module scopes and scope stack for nested scopes
struct Symbol_Table {
  // Module management
  [[nodiscard]] Scope* create_module_scope(std::string const& module_path_);
  [[nodiscard]] Scope* get_module_scope(std::string const& module_path_);
  [[nodiscard]] Scope const* get_module_scope(std::string const& module_path_) const;

  // Scope stack management (for traversing nested scopes)
  void enter_scope(Scope_Kind kind_);
  void exit_scope();
  [[nodiscard]] Scope* current_scope();
  [[nodiscard]] Scope const* current_scope() const;

  // Symbol operations on current scope
  // Returns error message on failure, std::nullopt on success
  [[nodiscard]] std::optional<std::string> declare(std::string name_, Symbol symbol_);
  [[nodiscard]] std::optional<Symbol> lookup(std::string_view name_) const;
  [[nodiscard]] std::optional<Symbol> lookup_local(std::string_view name_) const;

private:
  // Module-level scopes (key = module path like "Geometry.Shapes")
  std::unordered_map<std::string, std::unique_ptr<Scope>> m_modules;

  // Current scope stack (for nested scopes during AST traversal)
  std::vector<Scope*> m_scope_stack;

  // Storage for non-module scopes (blocks, functions)
  // These are owned by the symbol table and referenced by scope stack
  std::vector<std::unique_ptr<Scope>> m_scope_storage;
};

// ============================================================================
// Helper Functions
// ============================================================================

[[nodiscard]] inline Symbol make_symbol(
    std::string name_,
    Symbol_Kind kind_,
    Type type_,
    Visibility visibility_ = Visibility::Module_Internal,
    Symbol_Location location_ = {}
) {
  return Symbol{
      .name = std::move(name_),
      .kind = kind_,
      .type = std::move(type_),
      .visibility = visibility_,
      .location = std::move(location_)
  };
}

[[nodiscard]] inline Symbol_Location make_symbol_location(std::string filename_, Source_Position position_) {
  return Symbol_Location{.filename = std::move(filename_), .position = position_};
}

}  // namespace life_lang::semantic
