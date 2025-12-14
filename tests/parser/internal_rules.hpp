#ifndef LIFE_LANG_TESTS_PARSER_INTERNAL_RULES_HPP
#define LIFE_LANG_TESTS_PARSER_INTERNAL_RULES_HPP

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
PARSE_FN_DECL(Func_Def, func_def);
PARSE_FN_DECL(Struct_Def, struct_def);
PARSE_FN_DECL(Enum_Def, enum_def);
PARSE_FN_DECL(Impl_Block, impl_block);
PARSE_FN_DECL(Trait_Def, trait_def);
PARSE_FN_DECL(Trait_Impl, trait_impl);
PARSE_FN_DECL(Type_Alias, type_alias);

// Statement-level constructs
PARSE_FN_DECL(Statement, statement);
PARSE_FN_DECL(Block, block);

// Expression-level constructs
PARSE_FN_DECL(Expr, expr);

// Type references
PARSE_FN_DECL(Tuple_Type, tuple_type);
PARSE_FN_DECL(Type_Name, type_name);
PARSE_FN_DECL(Function_Type, function_type);

// Literal building blocks
PARSE_FN_DECL(Integer, integer);
PARSE_FN_DECL(Float, float);
PARSE_FN_DECL(String, string);
PARSE_FN_DECL(Char, char);

#undef PARSE_FN_DECL

}  // namespace life_lang::internal

#endif  // LIFE_LANG_TESTS_PARSER_INTERNAL_RULES_HPP
