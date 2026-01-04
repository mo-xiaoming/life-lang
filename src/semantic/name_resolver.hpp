// Name resolution for semantic analysis
// Resolves identifiers to their declarations in the symbol table

#pragma once

#include "../diagnostics.hpp"
#include "../expected.hpp"
#include "../parser/ast.hpp"
#include "symbol_table.hpp"
#include "type.hpp"

namespace life_lang::semantic {

// Name resolver - walks AST and verifies all names resolve to declarations
class Name_Resolver {
public:
  explicit Name_Resolver(Symbol_Table& symtab_, Diagnostic_Engine& diag_) : m_symtab(&symtab_), m_diag(&diag_) {}

  // Resolve a variable/function reference
  [[nodiscard]] Expected<Symbol, Diagnostic_Engine> resolve_var_name(ast::Var_Name const& name_);

  // Resolve a type reference
  [[nodiscard]] Expected<Type, Diagnostic_Engine> resolve_type_name(ast::Type_Name const& name_);

private:
  Symbol_Table* m_symtab;     // Non-owning pointer (guaranteed valid during Name_Resolver lifetime)
  Diagnostic_Engine* m_diag;  // Non-owning pointer (guaranteed valid during Name_Resolver lifetime)
};

}  // namespace life_lang::semantic
