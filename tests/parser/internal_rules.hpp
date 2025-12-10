#ifndef TESTS_PARSER_INTERNAL_RULES_HPP__
#define TESTS_PARSER_INTERNAL_RULES_HPP__

// Internal Parser Rule Declarations - FOR TESTING ONLY
//
// This header exposes semantic boundary rules for unit testing.
// Only complete, meaningful language constructs are exposed.
// Production code should use the public API: parser::parse_module()
//
// Design principle: Expose rules that users would write as standalone code.
// Don't expose: helper rules, implementation details, grammar fragments.

#include <rules.hpp>

namespace life_lang::internal {

#define PARSE_FN_DECL(ast_type, fn_name) \
  parser::Parse_Result<ast::ast_type> parse_##fn_name(parser::Iterator_Type& a_begin, parser::Iterator_Type a_end)

// Top-level entry point
PARSE_FN_DECL(Module, module);

// Complete declarations
PARSE_FN_DECL(Function_Definition, function_definition);
PARSE_FN_DECL(Struct_Definition, struct_definition);
PARSE_FN_DECL(Enum_Definition, enum_definition);

// Statement-level constructs
PARSE_FN_DECL(Statement, statement);
PARSE_FN_DECL(Block, block);

// Expression-level constructs
PARSE_FN_DECL(Expr, expr);

// Type references
PARSE_FN_DECL(Type_Name, type_name);

// Literal building blocks
PARSE_FN_DECL(Integer, integer);
PARSE_FN_DECL(Float, float);
PARSE_FN_DECL(String, string);
PARSE_FN_DECL(Char, char);

#undef PARSE_FN_DECL

}  // namespace life_lang::internal

#endif
