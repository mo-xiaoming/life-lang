#ifndef RULES_HPP__
#define RULES_HPP__

#include <iosfwd>
#include <string>
#include <tl/expected.hpp>

#include "ast.hpp"

namespace life_lang::parser {
using Iterator_Type = std::string::const_iterator;

template <typename Ast>
using Parse_Result = tl::expected<Ast, std::string>;

// ============================================================================
// PUBLIC API
// ============================================================================

// Parse a complete module (compilation unit)
Parse_Result<ast::Module> parse(Iterator_Type& a_begin, Iterator_Type a_end, std::ostream& a_out);
}  // namespace life_lang::parser

// ============================================================================
// INTERNAL API - FOR TESTING ONLY
// ============================================================================
// These functions expose individual parsers for unit testing.
// Production code should ONLY use parser::parse() above.

namespace life_lang::internal {

#define PARSE_FN_DECL(ast_type, fn_name)                                               \
  parser::Parse_Result<ast::ast_type> parse_##fn_name(                                 \
      parser::Iterator_Type& a_begin, parser::Iterator_Type a_end, std::ostream& a_out \
  )

PARSE_FN_DECL(Path_Segment, path_segment);
PARSE_FN_DECL(Path, path);
PARSE_FN_DECL(String, string);
PARSE_FN_DECL(Integer, integer);
PARSE_FN_DECL(Function_Parameter, function_parameter);
PARSE_FN_DECL(Function_Declaration, function_declaration);
PARSE_FN_DECL(Function_Definition, function_definition);
PARSE_FN_DECL(Expr, expr);
PARSE_FN_DECL(Function_Call_Expr, function_call_expr);
PARSE_FN_DECL(Function_Call_Statement, function_call_statement);
PARSE_FN_DECL(Return_Statement, return_statement);
PARSE_FN_DECL(Statement, statement);
PARSE_FN_DECL(Block, block);
PARSE_FN_DECL(Module, module);

#undef PARSE_FN_DECL

}  // namespace life_lang::internal

#endif
