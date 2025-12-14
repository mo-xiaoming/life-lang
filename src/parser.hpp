#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "ast.hpp"
#include "diagnostics.hpp"
#include "expected.hpp"

namespace life_lang::parser {

// ============================================================================
// Parser Class - Recursive Descent Parser
// ============================================================================

class Parser {
public:
  explicit Parser(std::string_view source_, std::string filename_ = "<input>");

  // Main entry point - parse a complete module
  Expected<ast::Module, Diagnostic_Engine> parse_module();

  // Get accumulated diagnostics (for moving out after parsing)
  [[nodiscard]] Diagnostic_Engine get_diagnostics() && { return std::move(m_diagnostics); }

  // Check if there are any accumulated errors
  [[nodiscard]] bool has_errors() const { return m_diagnostics.has_errors(); }

  // Check if all input has been consumed (for testing)
  [[nodiscard]] bool is_at_end() const {
    // Skip any trailing whitespace/comments
    auto pos = m_pos;
    // Track line/column for potential future use
    [[maybe_unused]] auto line = m_line;
    [[maybe_unused]] auto column = m_column;
    while (pos < m_source.size()) {
      char const ch = m_source[pos];
      if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
        ++pos;
        if (ch == '\n') {
          ++line;
          column = 1;
        } else {
          ++column;
        }
        continue;
      }
      // Check for comments
      if (ch == '/' && pos + 1 < m_source.size()) {
        if (m_source[pos + 1] == '/') {
          pos += 2;
          while (pos < m_source.size() && m_source[pos] != '\n') {
            ++pos;
          }
          continue;
        }
        if (m_source[pos + 1] == '*') {
          pos += 2;
          int nesting = 1;
          while (nesting > 0 && pos < m_source.size()) {
            if (m_source[pos] == '/' && pos + 1 < m_source.size() && m_source[pos + 1] == '*') {
              pos += 2;
              ++nesting;
            } else if (m_source[pos] == '*' && pos + 1 < m_source.size() && m_source[pos + 1] == '/') {
              pos += 2;
              --nesting;
            } else {
              ++pos;
            }
          }
          continue;
        }
      }
      return false;  // Non-whitespace, non-comment found
    }
    return true;  // Reached end
  }

  // ============================================================================
  // Parsing Functions - Public API for testing individual constructs
  // ============================================================================

  // Phase 2: Simple nodes
  std::optional<ast::Integer> parse_integer();
  std::optional<ast::Float> parse_float();
  std::optional<ast::String> parse_string();
  std::optional<ast::Char> parse_char();
  std::optional<ast::Unit_Literal> parse_unit_literal();
  std::optional<ast::Struct_Literal> parse_struct_literal();
  std::optional<ast::Var_Name> parse_variable_name();
  std::optional<ast::Var_Name> parse_qualified_variable_name();  // Multi-segment paths for function calls
  std::optional<ast::Type_Name> parse_type_name();

  // Phase 3: Type system
  std::optional<ast::Path_Type> parse_path_type();
  std::optional<ast::Function_Type> parse_function_type();
  std::optional<ast::Type_Param> parse_type_param();
  std::optional<ast::Where_Clause> parse_where_clause();

  // Phase 4: Expressions
  std::optional<ast::Expr> parse_expr();
  std::optional<ast::Expr> parse_primary_expr();
  std::optional<ast::Expr> parse_postfix_expr();
  std::optional<ast::Expr> parse_unary_expr();
  std::optional<ast::Expr> parse_binary_expr(int min_precedence_);
  std::optional<ast::Expr> parse_field_access();
  std::optional<ast::Expr> parse_func_call();
  std::optional<ast::If_Expr> parse_if_expr();
  std::optional<ast::Match_Expr> parse_match_expr();
  std::optional<ast::For_Expr> parse_for_expr();
  std::optional<ast::While_Expr> parse_while_expr();
  std::optional<ast::Range_Expr> parse_range_expr();
  std::optional<ast::Block> parse_block();

  // Phase 5: Statements
  std::optional<ast::Pattern> parse_pattern();
  std::optional<ast::Statement> parse_statement();
  std::optional<ast::Let_Statement> parse_let_statement();
  std::optional<ast::Assignment_Expr> parse_assignment();
  std::optional<ast::Return_Statement> parse_return_statement();
  std::optional<ast::Break_Statement> parse_break_statement();
  std::optional<ast::Continue_Statement> parse_continue_statement();

  // Phase 6: Declarations
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
  // ============================================================================
  // Lexer State
  // ============================================================================
  std::string_view m_source;
  std::size_t m_pos{0};
  std::size_t m_line{1};
  std::size_t m_column{1};

  // Error accumulation
  Diagnostic_Engine m_diagnostics;

  // ============================================================================
  // Lexer Utilities
  // ============================================================================

  // Peek at current character without advancing
  [[nodiscard]] char peek() const;

  // Peek at character at offset from current position
  [[nodiscard]] char peek(std::size_t offset_) const;

  // Advance position and return current character
  char advance();

  // Skip whitespace and comments
  void skip_whitespace_and_comments();

  // Get current source position
  [[nodiscard]] Source_Position current_position() const;

  // Create source range from start position to current position
  [[nodiscard]] Source_Range make_range(Source_Position start_) const;

  // ============================================================================
  // Error Reporting
  // ============================================================================

  // Report an error at the given range
  void error(std::string message_, Source_Range range_);

  // Report an error at current position
  void error(std::string message_);

  // ============================================================================
  // Token Matching
  // ============================================================================

  // Expect a specific character, consume if matches, error if not
  bool expect(char ch_);

  // Expect a specific string, consume if matches, error if not
  bool expect(std::string_view str_);

  // Try to match a keyword (must be followed by non-identifier character)
  bool match_keyword(std::string_view keyword_);

  // Try to match an operator
  bool match_operator(std::string_view op_);

  // Check if current position matches string without consuming
  [[nodiscard]] bool lookahead(std::string_view str_) const;

  // Check if character is valid identifier start
  [[nodiscard]] static bool is_identifier_start(char ch_);

  // Check if character is valid identifier continuation
  [[nodiscard]] static bool is_identifier_continue(char ch_);

  // ============================================================================
  // Expression Parsing Helpers
  // ============================================================================

  // Get operator precedence (higher number = higher precedence)
  [[nodiscard]] static int get_precedence(ast::Binary_Op op_);

  // Try to parse a binary operator at current position
  [[nodiscard]] std::optional<ast::Binary_Op> try_parse_binary_op();

  // Try to parse a unary operator at current position
  [[nodiscard]] std::optional<ast::Unary_Op> try_parse_unary_op();
};

}  // namespace life_lang::parser
