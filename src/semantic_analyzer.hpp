#ifndef LIFE_LANG_SEMANTIC_ANALYZER_HPP
#define LIFE_LANG_SEMANTIC_ANALYZER_HPP

#include <string>
#include "ast.hpp"
#include "diagnostics.hpp"
#include "symbol_table.hpp"

namespace life_lang {

// ============================================================================
// Semantic Analyzer
// ============================================================================

// Forward declare visitor for friend declaration
struct Declaration_Collector;

// Performs semantic analysis on the AST:
// - Builds symbol table
// - Resolves names
// - Validates semantic rules
// (Future: type checking, trait resolution)
class Semantic_Analyzer {
 public:
  explicit Semantic_Analyzer(Diagnostic_Engine& a_diagnostics);

  // Main entry point - analyzes a complete module
  // Returns true if analysis succeeded (no errors)
  [[nodiscard]] bool analyze(ast::Module const& a_module);

  // Access symbol table (for testing/debugging)
  [[nodiscard]] Symbol_Table const& symbol_table() const { return m_symbol_table; }

 private:
  friend struct Declaration_Collector;  // Needs access to collect_* methods
  // ========================================================================
  // Declaration Collection
  // ========================================================================

  void collect_declarations(ast::Module const& a_module);
  void collect_func_def(ast::Func_Def const& a_func);
  void collect_struct_def(ast::Struct_Def const& a_struct);
  void collect_enum_def(ast::Enum_Def const& a_enum);
  void collect_trait_def(ast::Trait_Def const& a_trait);
  void collect_type_alias(ast::Type_Alias const& a_alias);
  void collect_impl_block(ast::Impl_Block const& a_impl);
  void collect_trait_impl(ast::Trait_Impl const& a_impl);

  // ========================================================================
  // Name Resolution
  // ========================================================================

  void resolve_names(ast::Module const& a_module);
  void resolve_func_body(ast::Func_Def const& a_func);

  // Resolve type names
  static bool resolve_type_name(ast::Type_Name const& a_type);
  static bool resolve_path_type(ast::Path_Type const& a_type);
  static bool resolve_function_type(ast::Function_Type const& a_type);

  // Resolve expressions (validates variable names)
  void resolve_expr(ast::Expr const& a_expr);
  void resolve_var_name(ast::Var_Name const& a_name);
  void resolve_func_call(ast::Func_Call_Expr const& a_call);
  void resolve_binary_expr(ast::Binary_Expr const& a_expr);
  void resolve_if_expr(ast::If_Expr const& a_expr);
  void resolve_match_expr(ast::Match_Expr const& a_expr);
  void resolve_block(ast::Block const& a_block);

  // Resolve statements
  void resolve_stmt(ast::Statement const& a_stmt);
  void resolve_let_statement(ast::Let_Statement const& a_let);

  // Resolve patterns (creates bindings in current scope)
  void resolve_pattern(ast::Pattern const& a_pattern);

  // ========================================================================
  // Validation Helpers
  // ========================================================================

  // Naming convention validation (deferred from parser)
  [[nodiscard]] bool validate_type_name_convention(std::string const& a_name, Source_Location const& a_loc);
  [[nodiscard]] bool validate_value_name_convention(std::string const& a_name, Source_Location const& a_loc);
  [[nodiscard]] bool validate_self_usage(Source_Location const& a_loc);  // Must be in impl scope

  // Extract type name as string for symbol table (until we have proper type system)
  [[nodiscard]] static std::string type_to_string(ast::Type_Name const& a_type);

  // Convert AST position to Source_Location
  [[nodiscard]] static Source_Location get_location([[maybe_unused]] auto const& a_node) {
    // Spirit X3 position_tagged gives us iterator position
    // For now, just return placeholder - proper implementation needs source tracking
    return Source_Location{.file = "<input>", .line = 0, .column = 0};
  }

  // ========================================================================
  // Members
  // ========================================================================

  Symbol_Table m_symbol_table;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members) - Intentional: analyzer doesn't own diagnostics
  Diagnostic_Engine& m_diagnostics;
  bool m_has_errors{false};
};

}  // namespace life_lang

#endif  // LIFE_LANG_SEMANTIC_ANALYZER_HPP
