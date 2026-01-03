#ifndef LIFE_LANG_TESTS_PARSER_INTERNAL_RULES_HPP
#define LIFE_LANG_TESTS_PARSER_INTERNAL_RULES_HPP

// Internal Parser Wrapper - FOR TESTING ONLY
//
// This header provides a convenient wrapper around Parser for unit testing.
// Tests can use these helpers to parse specific constructs without manually
// managing Parser instances and checking for errors.
//
// Production code should use Parser class directly.

#include "diagnostics.hpp"
#include "expected.hpp"
#include "parser/parser.hpp"

#include <string_view>

namespace life_lang::internal {

// Helper function to parse a construct using Parser
// Returns parsed AST on success, or Diagnostic_Engine with errors on failure
template <typename Ast, typename Parse_Method>
Expected<Ast, Diagnostic_Engine> parse_with_parser(std::string_view source_, Parse_Method parse_method_) {
  Diagnostic_Engine diagnostics{"<test>", source_};
  parser::Parser parser{diagnostics};

  auto result = parse_method_(parser);

  if (!result.has_value()) {
    return Unexpected(diagnostics);
  }

  // Check if all input was consumed
  // Note: parse_module() enforces this, but other parse_* methods don't
  if (!parser.all_input_consumed()) {
    return Unexpected(diagnostics);
  }

  return std::move(*result);
}

// Parse functions for unit tests
#define PARSE_FN_DECL(ast_type, fn_name)                                                             \
  inline Expected<ast::ast_type, Diagnostic_Engine> parse_##fn_name(std::string_view source_) {      \
    return parse_with_parser<ast::ast_type>(source_, [](auto& p_) { return p_.parse_##fn_name(); }); \
  }

// Top-level entry point
PARSE_FN_DECL(Module, module)

// Complete declarations
PARSE_FN_DECL(Func_Def, func_def)
PARSE_FN_DECL(Struct_Def, struct_def)
PARSE_FN_DECL(Enum_Def, enum_def)
PARSE_FN_DECL(Impl_Block, impl_block)
PARSE_FN_DECL(Trait_Def, trait_def)
PARSE_FN_DECL(Trait_Impl, trait_impl)
PARSE_FN_DECL(Type_Alias, type_alias)

// Statement-level constructs
PARSE_FN_DECL(Statement, statement)
PARSE_FN_DECL(Block, block)

// Expression-level constructs
PARSE_FN_DECL(Expr, expr)

// Type references
PARSE_FN_DECL(Type_Name, type_name)
PARSE_FN_DECL(Function_Type, function_type)
PARSE_FN_DECL(Array_Type, array_type)

// Literal building blocks
PARSE_FN_DECL(Integer, integer)
PARSE_FN_DECL(Float, float)
PARSE_FN_DECL(Bool_Literal, bool_literal)
PARSE_FN_DECL(String, string)
PARSE_FN_DECL(Char, char)

#undef PARSE_FN_DECL

}  // namespace life_lang::internal

#endif  // LIFE_LANG_TESTS_PARSER_INTERNAL_RULES_HPP
