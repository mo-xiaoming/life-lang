#pragma once

#include <memory>
#include <optional>

#include "ast.hpp"

namespace life_lang {
struct Diagnostic_Engine;
}  // namespace life_lang

namespace life_lang::parser {

// ============================================================================
// Parser Class - Recursive Descent Parser
// ============================================================================

class Parser {
public:
  explicit Parser(Diagnostic_Engine& diagnostics_);

  Parser(Parser const&) = delete;
  Parser(Parser&&) = delete;
  Parser& operator=(Parser const&) = delete;
  Parser& operator=(Parser&&) = delete;

  ~Parser();

  // ============================================================================
  // Production API
  // ============================================================================

  // Parse a complete module (imports + items)
  // This is the main entry point for production use - validates entire input
  std::optional<ast::Module> parse_module();

  // ============================================================================
  // Testing API
  // ============================================================================
  // The methods below are public for unit testing individual grammar rules.
  // Production code should use parse_module() instead.
  //
  // Note: These methods parse only what they need and don't validate EOF.
  // Use all_input_consumed() in tests to ensure no trailing content.

  // Check if all meaningful input has been consumed (ignoring trailing whitespace/comments)
  [[nodiscard]] bool all_input_consumed() const;

  std::optional<ast::Import_Statement> parse_import_statement();

  // Simple nodes
  std::optional<ast::Integer> parse_integer();
  std::optional<ast::Float> parse_float();
  std::optional<ast::Bool_Literal> parse_bool_literal();
  std::optional<ast::String> parse_string();
  std::optional<ast::String_Interpolation> parse_string_interpolation();
  std::optional<ast::String> parse_raw_string();
  std::optional<ast::Char> parse_char();
  std::optional<ast::Unit_Literal> parse_unit_literal();
  std::optional<ast::Struct_Literal> parse_struct_literal();
  std::optional<ast::Array_Literal> parse_array_literal();
  std::optional<ast::Var_Name> parse_variable_name();
  std::optional<ast::Var_Name> parse_qualified_variable_name();
  std::optional<ast::Type_Name> parse_type_name();

  // Type system
  std::optional<ast::Path_Type> parse_path_type();
  std::optional<ast::Function_Type> parse_function_type();
  std::optional<ast::Array_Type> parse_array_type();
  std::optional<ast::Type_Param> parse_type_param();
  std::optional<ast::Where_Clause> parse_where_clause();
  std::optional<std::vector<ast::Trait_Bound>> parse_trait_bounds();

  // Expressions
  std::optional<ast::Expr> parse_expr();
  std::optional<ast::Expr> parse_primary_expr();
  std::optional<ast::Expr> parse_postfix_expr();
  std::optional<ast::Expr> parse_unary_expr();
  std::optional<ast::Expr> parse_binary_expr(int min_precedence_);
  std::optional<ast::If_Expr> parse_if_expr();
  std::optional<ast::Match_Expr> parse_match_expr();
  std::optional<ast::For_Expr> parse_for_expr();
  std::optional<ast::While_Expr> parse_while_expr();
  std::optional<ast::Block> parse_block();

  // Statements
  std::optional<ast::Pattern> parse_pattern();
  std::optional<ast::Pattern> parse_single_pattern();
  std::optional<ast::Statement> parse_statement();
  std::optional<ast::Let_Statement> parse_let_statement();
  std::optional<ast::Assignment_Statement> parse_assignment_statement();
  std::optional<ast::Return_Statement> parse_return_statement();
  std::optional<ast::Break_Statement> parse_break_statement();
  std::optional<ast::Continue_Statement> parse_continue_statement();

  // Declarations
  std::optional<ast::Func_Param> parse_func_param();
  std::optional<ast::Func_Decl> parse_func_decl();
  std::optional<ast::Func_Def> parse_func_def();
  std::optional<ast::Struct_Field> parse_struct_field();
  std::optional<ast::Struct_Def> parse_struct_def();
  std::optional<ast::Enum_Variant> parse_enum_variant();
  std::optional<ast::Enum_Def> parse_enum_def();
  std::optional<ast::Assoc_Type_Decl> parse_assoc_type_decl();
  std::optional<ast::Trait_Def> parse_trait_def();
  std::optional<ast::Type_Alias> parse_type_alias();
  std::optional<ast::Impl_Block> parse_impl_block();
  std::optional<ast::Assoc_Type_Impl> parse_assoc_type_impl();
  std::optional<ast::Trait_Impl> parse_trait_impl();

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

}  // namespace life_lang::parser
