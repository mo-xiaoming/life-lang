#include "utils.hpp"

using life_lang::ast::Function_Definition;
using life_lang::ast::make_block;
using life_lang::ast::make_expr;
using life_lang::ast::make_function_call_expr;
using life_lang::ast::make_function_call_statement;
using life_lang::ast::make_function_declaration;
using life_lang::ast::make_function_definition;
using life_lang::ast::make_function_parameter;
using life_lang::ast::make_integer;
using life_lang::ast::make_path;
using life_lang::ast::make_path_segment;
using life_lang::ast::make_return_statement;
using life_lang::ast::make_statement;
using life_lang::ast::make_string;

PARSE_TEST(Function_Definition, function_definition)

namespace {
// Simple function definitions
constexpr auto k_empty_body_input = "fn hello(): Int {}";
auto make_empty_body_expected() {
  return make_function_definition(make_function_declaration("hello", {}, make_path("Int")), make_block({}));
}

// Functions with parameters
constexpr auto k_with_parameters_input = "fn hello(a: Int, b: Double): Int {}";
auto make_with_parameters_expected() {
  return make_function_definition(
      make_function_declaration(
          "hello", {make_function_parameter("a", make_path("Int")), make_function_parameter("b", make_path("Double"))},
          make_path("Int")
      ),
      make_block({})
  );
}

// Functions with statements
constexpr auto k_with_return_input = "fn hello(): Int {return world;}";
auto make_with_return_expected() {
  return make_function_definition(
      make_function_declaration("hello", {}, make_path("Int")),
      make_block({make_statement(make_return_statement(make_expr(make_path("world"))))})
  );
}

constexpr auto k_with_statements_input = "fn hello(): Int {foo(); return 0;}";
auto make_with_statements_expected() {
  return make_function_definition(
      make_function_declaration("hello", {}, make_path("Int")),
      make_block(
          {make_statement(make_function_call_statement(make_function_call_expr(make_path("foo"), {}))),
           make_statement(make_return_statement(make_expr(make_integer("0"))))}
      )
  );
}

// Nested constructs
constexpr auto k_nested_block_input = R"(fn hello(a: Int): Int {
    hello();
    {
        return world;
    }
})";
auto make_nested_block_expected() {
  return make_function_definition(
      make_function_declaration("hello", {make_function_parameter("a", make_path("Int"))}, make_path("Int")),
      make_block(
          {make_statement(make_function_call_statement(make_function_call_expr(make_path("hello"), {}))),
           make_statement(make_block({make_statement(make_return_statement(make_expr(make_path("world"))))}))}
      )
  );
}

constexpr auto k_nested_function_input = R"(fn hello(): Int {
    fn world(): Int {
        return 0;
    }
    return world();
})";
auto make_nested_function_expected() {
  return make_function_definition(
      make_function_declaration("hello", {}, make_path("Int")),
      make_block(
          {make_statement(make_function_definition(
               make_function_declaration("world", {}, make_path("Int")),
               make_block({make_statement(make_return_statement(make_expr(make_integer("0"))))})
           )),
           make_statement(make_return_statement(make_expr(make_function_call_expr(make_path("world"), {}))))}
      )
  );
}

// Complex real-world examples
constexpr auto k_hello_world_input = R"(fn main(args: Std.Array<Std.String>): I32 {
    Std.print("Hello, world!");
    return 0;
})";
auto make_hello_world_expected() {
  return make_function_definition(
      make_function_declaration(
          "main",
          {make_function_parameter("args", make_path("Std", make_path_segment("Array", {make_path("Std", "String")})))},
          make_path("I32")
      ),
      make_block(
          {make_statement(make_function_call_statement(
               make_function_call_expr(make_path("Std", "print"), {make_expr(make_string("\"Hello, world!\""))})
           )),
           make_statement(make_return_statement(make_expr(make_integer("0"))))}
      )
  );
}

// Trailing content
constexpr auto k_with_trailing_code_input = "fn foo(): Int {} bar";
auto make_with_trailing_code_expected() {
  return make_function_definition(make_function_declaration("foo", {}, make_path("Int")), make_block({}));
}

// Invalid cases
constexpr auto k_invalid_no_fn_keyword_input = "hello(): Int {}";
constexpr auto k_invalid_empty_input = "";
auto make_invalid_expected() {
  return make_function_definition(make_function_declaration("", {}, make_path()), make_block({}));
}

}  // namespace

TEST_CASE("Parse Function_Definition", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Function_Definition_Params>({
          // Simple function definitions
          {"empty body", k_empty_body_input, make_empty_body_expected(), true, ""},

          // Functions with parameters
          {"with parameters", k_with_parameters_input, make_with_parameters_expected(), true, ""},

          // Functions with statements
          {"with return", k_with_return_input, make_with_return_expected(), true, ""},
          {"with statements", k_with_statements_input, make_with_statements_expected(), true, ""},

          // Nested constructs
          {"nested block", k_nested_block_input, make_nested_block_expected(), true, ""},
          {"nested function", k_nested_function_input, make_nested_function_expected(), true, ""},

          // Complex real-world examples
          {"hello world", k_hello_world_input, make_hello_world_expected(), true, ""},

          // Trailing content
          {"with trailing code", k_with_trailing_code_input, make_with_trailing_code_expected(), true, "bar"},

          // Invalid cases
          {"invalid - no fn keyword", k_invalid_no_fn_keyword_input, make_invalid_expected(), false, "hello(): Int {}"},
          {"invalid - empty", k_invalid_empty_input, make_invalid_expected(), false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}
