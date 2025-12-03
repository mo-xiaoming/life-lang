#include "utils.hpp"

using life_lang::ast::Block;
using life_lang::ast::make_block;
using life_lang::ast::make_expr;
using life_lang::ast::make_function_call_expr;
using life_lang::ast::make_function_call_statement;
using life_lang::ast::make_integer;
using life_lang::ast::make_path;
using life_lang::ast::make_return_statement;
using life_lang::ast::make_statement;

PARSE_TEST(Block, block)

namespace {
// Empty block
constexpr auto k_empty_block_input = "{}";

// Single statement blocks
constexpr auto k_single_return_input = "{return hello;}";
auto make_single_return_expected() {
  return make_block({make_statement(make_return_statement(make_expr(make_path("hello"))))});
}

constexpr auto k_single_function_call_input = "{foo();}";
auto make_single_function_call_expected() {
  return make_block({make_statement(make_function_call_statement(make_function_call_expr(make_path("foo"), {})))});
}

// Multiple statements
constexpr auto k_two_statements_input = "{hello.a(); return world;}";
auto make_two_statements_expected() {
  return make_block(
      {make_statement(make_function_call_statement(make_function_call_expr(make_path("hello", "a"), {}))),
       make_statement(make_return_statement(make_expr(make_path("world"))))}
  );
}

constexpr auto k_multiple_statements_input = "{foo(); bar(); return 0;}";
auto make_multiple_statements_expected() {
  return make_block(
      {make_statement(make_function_call_statement(make_function_call_expr(make_path("foo"), {}))),
       make_statement(make_function_call_statement(make_function_call_expr(make_path("bar"), {}))),
       make_statement(make_return_statement(make_expr(make_integer("0"))))}
  );
}

// Nested blocks
constexpr auto k_nested_block_input = "{hello(b); {return world;}}";
auto make_nested_block_expected() {
  return make_block(
      {make_statement(
           make_function_call_statement(make_function_call_expr(make_path("hello"), {make_expr(make_path("b"))}))
       ),
       make_statement(make_block({make_statement(make_return_statement(make_expr(make_path("world"))))}))}
  );
}

// Whitespace handling
constexpr auto k_with_spaces_input = "{  foo(  )  ;  }";
auto make_with_spaces_expected() {
  return make_block({make_statement(make_function_call_statement(make_function_call_expr(make_path("foo"), {})))});
}

// Trailing content
constexpr auto k_with_trailing_code_input = "{return x;} y";
auto make_with_trailing_code_expected() {
  return make_block({make_statement(make_return_statement(make_expr(make_path("x"))))});
}

// Invalid cases
constexpr auto k_invalid_no_closing_brace_input = "{return x;";
constexpr auto k_invalid_no_opening_brace_input = "return x;}";
constexpr auto k_invalid_empty_input = "";
}  // namespace

TEST_CASE("Parse Block", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Block_Params>({
          // Empty block
          {"empty block", k_empty_block_input, make_block({}), true, ""},

          // Single statement blocks
          {"single return", k_single_return_input, make_single_return_expected(), true, ""},
          {"single function call", k_single_function_call_input, make_single_function_call_expected(), true, ""},

          // Multiple statements
          {"two statements", k_two_statements_input, make_two_statements_expected(), true, ""},
          {"multiple statements", k_multiple_statements_input, make_multiple_statements_expected(), true, ""},

          // Nested blocks
          {"nested block", k_nested_block_input, make_nested_block_expected(), true, ""},

          // Whitespace handling
          {"with spaces", k_with_spaces_input, make_with_spaces_expected(), true, ""},

          // Trailing content
          {"with trailing code", k_with_trailing_code_input, make_with_trailing_code_expected(), true, "y"},

          // Invalid cases
          {"invalid - no closing brace", k_invalid_no_closing_brace_input, make_block({}), false, ""},
          {"invalid - no opening brace", k_invalid_no_opening_brace_input, make_block({}), false, "return x;}"},
          {"invalid - empty", k_invalid_empty_input, make_block({}), false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}