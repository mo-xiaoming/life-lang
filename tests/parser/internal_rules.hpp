#ifndef TESTS_PARSER_INTERNAL_RULES_HPP__
#define TESTS_PARSER_INTERNAL_RULES_HPP__

// Internal Parser Rule Declarations - FOR TESTING ONLY
//
// This header exposes individual parser rules for unit testing.
// Production code should use the public API: parser::parse_module()
//
// These are the same declarations that were previously in src/rules.hpp
// but moved here to keep the public API clean.

#include <rules.hpp>

namespace life_lang::internal {

#define PARSE_FN_DECL(ast_type, fn_name) \
  parser::Parse_Result<ast::ast_type> parse_##fn_name(parser::Iterator_Type& a_begin, parser::Iterator_Type a_end)

PARSE_FN_DECL(Variable_Name, variable_name);
PARSE_FN_DECL(Variable_Name_Segment, variable_name_segment);
PARSE_FN_DECL(Type_Name_Segment, type_name_segment);
PARSE_FN_DECL(Type_Name, type_name);
PARSE_FN_DECL(String, string);
PARSE_FN_DECL(Integer, integer);
PARSE_FN_DECL(Function_Parameter, function_parameter);
PARSE_FN_DECL(Function_Declaration, function_declaration);
PARSE_FN_DECL(Function_Definition, function_definition);
PARSE_FN_DECL(Struct_Field, struct_field);
PARSE_FN_DECL(Struct_Definition, struct_definition);
PARSE_FN_DECL(Expr, expr);
PARSE_FN_DECL(Function_Call_Expr, function_call_expr);
PARSE_FN_DECL(Field_Initializer, field_initializer);
PARSE_FN_DECL(Struct_Literal, struct_literal);
PARSE_FN_DECL(Function_Call_Statement, function_call_statement);
PARSE_FN_DECL(Return_Statement, return_statement);
PARSE_FN_DECL(Statement, statement);
PARSE_FN_DECL(Block, block);
PARSE_FN_DECL(Module, module);

#undef PARSE_FN_DECL

}  // namespace life_lang::internal

#endif
