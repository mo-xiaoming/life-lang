#ifndef LIFE_LANG_SYMBOL_TABLE_HPP
#define LIFE_LANG_SYMBOL_TABLE_HPP

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace life_lang {

// ============================================================================
// Symbol Kinds
// ============================================================================

enum class Symbol_Kind : std::uint8_t {
  Variable,    // let x = ...
  Function,    // fn foo() { ... }
  Parameter,   // fn foo(x: I32) - function parameter
  Type,        // struct Point { ... }, enum Option { ... }
  Trait,       // trait Display { ... }
  Type_Param,  // <T> in fn foo<T>() or struct Vec<T>
  Field,       // x in struct Point { x: I32 }
  Variant,     // Some in enum Option { Some(T), None }
  Module,      // (future) module Std { ... }
};

std::string to_string(Symbol_Kind a_kind);

// ============================================================================
// Source Location (for error reporting)
// ============================================================================

struct Source_Location {
  std::string file;    // Source file name
  std::size_t line;    // 1-based line number
  std::size_t column;  // 1-based column number
};

std::string to_string(Source_Location const& a_loc);

// ============================================================================
// Symbol
// ============================================================================

// Represents a declared symbol in the program
struct Symbol {
  std::string name;
  Symbol_Kind kind;
  Source_Location location;

  // Type information (TBD - will be filled in during type checking phase)
  // For now, store the raw AST type name as a string
  std::string type_annotation;  // e.g., "I32", "Vec<String>", "fn(I32): Bool"

  // Generic parameters (for functions, structs, enums, traits)
  std::vector<std::string> generic_params;  // e.g., ["T", "E"] for Result<T, E>

  // Additional metadata
  bool is_mutable{false};  // Future: for mut bindings
  bool is_public{true};    // Future: for visibility control
};

std::string to_string(Symbol const& a_symbol);

// ============================================================================
// Scope Kinds
// ============================================================================

enum class Scope_Kind : std::uint8_t {
  Global,     // Top-level module scope
  Function,   // Function body
  Block,      // { ... } block
  Impl,       // impl Block { ... } - has implicit 'self'
  Trait,      // trait Display { ... }
  Match_Arm,  // match x { Pattern => ... }
  Loop,       // for/while loop body
};

std::string to_string(Scope_Kind a_kind);

// ============================================================================
// Scope
// ============================================================================

// Represents a lexical scope in the program
class Scope {
 public:
  explicit Scope(Scope_Kind a_kind, Scope* a_parent = nullptr);

  // Symbol management
  [[nodiscard]] bool insert(Symbol const& a_symbol);  // Returns false if duplicate
  [[nodiscard]] Symbol const* lookup_local(std::string const& a_name) const;
  [[nodiscard]] bool contains_local(std::string const& a_name) const;

  // Accessors
  [[nodiscard]] Scope_Kind kind() const { return m_kind; }
  [[nodiscard]] Scope* parent() const { return m_parent; }
  [[nodiscard]] std::unordered_map<std::string, Symbol> const& symbols() const { return m_symbols; }

 private:
  Scope_Kind m_kind;
  Scope* m_parent;  // Nullptr for global scope
  std::unordered_map<std::string, Symbol> m_symbols;
};

// ============================================================================
// Symbol Table
// ============================================================================

// Manages scope stack and symbol resolution during semantic analysis
class Symbol_Table {
 public:
  Symbol_Table();

  // Scope management
  void enter_scope(Scope_Kind a_kind);
  void exit_scope();
  [[nodiscard]] Scope const* current_scope() const;
  [[nodiscard]] Scope_Kind current_scope_kind() const;

  // Symbol insertion (into current scope)
  [[nodiscard]] bool insert(Symbol const& a_symbol);  // Returns false if duplicate in current scope

  // Symbol lookup (searches up scope chain)
  [[nodiscard]] Symbol const* lookup(std::string const& a_name) const;

  // Specialized lookups
  [[nodiscard]] Symbol const* lookup_type(std::string const& a_name) const;   // Only types/traits
  [[nodiscard]] Symbol const* lookup_value(std::string const& a_name) const;  // Only vars/funcs

  // Scope queries
  [[nodiscard]] bool in_impl_scope() const;      // For validating 'self' usage
  [[nodiscard]] bool in_function_scope() const;  // For validating return statements
  [[nodiscard]] bool in_loop_scope() const;      // For validating break/continue

 private:
  friend std::string to_string(Symbol_Table const& a_table);
  std::vector<std::unique_ptr<Scope>> m_scopes;  // All scopes (owned)
  std::vector<Scope*> m_scope_stack;             // Current scope chain (non-owning)

  [[nodiscard]] static bool is_type_symbol(Symbol const& a_symbol);
  [[nodiscard]] static bool is_value_symbol(Symbol const& a_symbol);
};

std::string to_string(Symbol_Table const& a_table);

}  // namespace life_lang

#endif  // LIFE_LANG_SYMBOL_TABLE_HPP
