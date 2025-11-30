#include "utils.hpp"

using life_lang::ast::Function_Call_Statement;
using life_lang::ast::make_expr;
using life_lang::ast::make_function_call_expr;
using life_lang::ast::make_function_call_statement;
using life_lang::ast::make_integer;
using life_lang::ast::make_path;
using life_lang::ast::make_path_segment;
using life_lang::ast::make_string;

PARSE_TEST(Function_Call_Statement, function_call_statement)

namespace {
// No arguments
constexpr auto k_no_arguments_input = "hello();";
auto make_no_arguments_expected() {
  return make_function_call_statement(make_function_call_expr(make_path("hello"), {}));
}

// With arguments
constexpr auto k_one_argument_input = "foo(x);";
auto make_one_argument_expected() {
  return make_function_call_statement(make_function_call_expr(make_path("foo"), {make_expr(make_path("x"))}));
}

constexpr auto k_two_arguments_input = "foo(x, y);";
auto make_two_arguments_expected() {
  return make_function_call_statement(
      make_function_call_expr(make_path("foo"), {make_expr(make_path("x")), make_expr(make_path("y"))})
  );
}

constexpr auto k_dotted_path_args_input = "foo(a, b.c);";
auto make_dotted_path_args_expected() {
  return make_function_call_statement(
      make_function_call_expr(make_path("foo"), {make_expr(make_path("a")), make_expr(make_path("b", "c"))})
  );
}

// Qualified paths
constexpr auto k_namespace_call_input = "Std.print(x);";
auto make_namespace_call_expected() {
  return make_function_call_statement(make_function_call_expr(make_path("Std", "print"), {make_expr(make_path("x"))}));
}

// Template parameters
constexpr auto k_with_template_input = "A.B<Double>.hello.world(a, b.c);";
auto make_with_template_expected() {
  return make_function_call_statement(make_function_call_expr(
      make_path("A", make_path_segment("B", {make_path("Double")}), "hello", "world"),
      {make_expr(make_path("a")), make_expr(make_path("b", "c"))}
  ));
}

// Different argument types
constexpr auto k_integer_argument_input = "foo(42);";
auto make_integer_argument_expected() {
  return make_function_call_statement(make_function_call_expr(make_path("foo"), {make_expr(make_integer("42"))}));
}

constexpr auto k_string_argument_input = R"(print("hello");)";
auto make_string_argument_expected() {
  return make_function_call_statement(
      make_function_call_expr(make_path("print"), {make_expr(make_string(R"("hello")"))})
  );
}

// Trailing content
constexpr auto k_with_trailing_code_input = "foo(); bar";
auto make_with_trailing_code_expected() {
  return make_function_call_statement(make_function_call_expr(make_path("foo"), {}));
}

// Invalid cases
constexpr auto k_invalid_no_semicolon_input = "foo()";
constexpr auto k_invalid_no_parentheses_input = "foo;";
constexpr auto k_invalid_empty_input = "";
auto make_invalid_expected() { return make_function_call_statement(make_function_call_expr(make_path(), {})); }
}  // namespace

TEST_CASE("Parse Function_Call_Statement", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Function_Call_Statement_Params>({
          // No arguments
          {"no arguments", k_no_arguments_input, make_no_arguments_expected(), true, ""},

          // With arguments
          {"one argument", k_one_argument_input, make_one_argument_expected(), true, ""},
          {"two arguments", k_two_arguments_input, make_two_arguments_expected(), true, ""},
          {"dotted path args", k_dotted_path_args_input, make_dotted_path_args_expected(), true, ""},

          // Qualified paths
          {"namespace call", k_namespace_call_input, make_namespace_call_expected(), true, ""},

          // Template parameters
          {"with template", k_with_template_input, make_with_template_expected(), true, ""},

          // Different argument types
          {"integer argument", k_integer_argument_input, make_integer_argument_expected(), true, ""},
          {"string argument", k_string_argument_input, make_string_argument_expected(), true, ""},

          // Trailing content
          {"with trailing code", k_with_trailing_code_input, make_with_trailing_code_expected(), true, "bar"},

          // Invalid cases
          {"invalid - no semicolon", k_invalid_no_semicolon_input, make_invalid_expected(), false, ""},
          {"invalid - no parentheses", k_invalid_no_parentheses_input, make_invalid_expected(), false, "foo;"},
          {"invalid - empty", k_invalid_empty_input, make_invalid_expected(), false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}