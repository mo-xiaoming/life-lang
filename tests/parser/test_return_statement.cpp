#include "utils.hpp"

using life_lang::ast::make_expr;
using life_lang::ast::make_function_call_expr;
using life_lang::ast::make_integer;
using life_lang::ast::make_path;
using life_lang::ast::make_path_segment;
using life_lang::ast::make_return_statement;
using life_lang::ast::make_string;
using life_lang::ast::Return_Statement;

PARSE_TEST(Return_Statement, return_statement)

namespace {
// Simple paths
constexpr auto k_simple_identifier_input = "return hello;";
auto make_simple_identifier_expected() { return make_return_statement(make_expr(make_path("hello"))); }

constexpr auto k_dotted_path_input = "return hello.a.b;";
auto make_dotted_path_expected() { return make_return_statement(make_expr(make_path("hello", "a", "b"))); }

// Paths with templates
constexpr auto k_with_template_input = "return A.B.Hello<Int>.a;";
auto make_with_template_expected() {
  return make_return_statement(make_expr(make_path("A", "B", make_path_segment("Hello", {make_path("Int")}), "a")));
}

constexpr auto k_long_path_with_template_input = "return A.B.Hello<Int>.a.b.c;";
auto make_long_path_with_template_expected() {
  return make_return_statement(
      make_expr(make_path("A", "B", make_path_segment("Hello", {make_path("Int")}), "a", "b", "c"))
  );
}

// Function calls as return values
constexpr auto k_function_call_input = "return foo();";
auto make_function_call_expected() {
  return make_return_statement(make_expr(make_function_call_expr(make_path("foo"), {})));
}

constexpr auto k_function_call_with_arg_input = "return foo(x);";
auto make_function_call_with_arg_expected() {
  return make_return_statement(make_expr(make_function_call_expr(make_path("foo"), {make_expr(make_path("x"))})));
}

constexpr auto k_complex_function_call_input = "return A.B.Hello<Int>.a.c(b);";
auto make_complex_function_call_expected() {
  return make_return_statement(make_expr(make_function_call_expr(
      make_path("A", "B", make_path_segment("Hello", {make_path("Int")}), "a", "c"), {make_expr(make_path("b"))}
  )));
}

// Literal values
constexpr auto k_return_integer_input = "return 42;";
auto make_return_integer_expected() { return make_return_statement(make_expr(make_integer("42"))); }

constexpr auto k_return_string_input = R"(return "hello";)";
auto make_return_string_expected() { return make_return_statement(make_expr(make_string(R"("hello")"))); }

// Trailing content
constexpr auto k_with_trailing_code_input = "return x; y";
auto make_with_trailing_code_expected() { return make_return_statement(make_expr(make_path("x"))); }

// Invalid cases
constexpr auto k_invalid_no_semicolon_input = "return x";
constexpr auto k_invalid_no_expression_input = "return;";
constexpr auto k_invalid_empty_input = "";
auto make_invalid_expected() { return make_return_statement(make_expr(make_path())); }
}  // namespace

TEST_CASE("Parse Return_Statement", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Return_Statement_Params>({
          // Simple paths
          {"simple identifier", k_simple_identifier_input, make_simple_identifier_expected(), true, ""},
          {"dotted path", k_dotted_path_input, make_dotted_path_expected(), true, ""},

          // Paths with templates
          {"with template", k_with_template_input, make_with_template_expected(), true, ""},
          {"long path with template", k_long_path_with_template_input, make_long_path_with_template_expected(), true,
           ""},

          // Function calls as return values
          {"function call", k_function_call_input, make_function_call_expected(), true, ""},
          {"function call with arg", k_function_call_with_arg_input, make_function_call_with_arg_expected(), true, ""},
          {"complex function call", k_complex_function_call_input, make_complex_function_call_expected(), true, ""},

          // Literal values
          {"return integer", k_return_integer_input, make_return_integer_expected(), true, ""},
          {"return string", k_return_string_input, make_return_string_expected(), true, ""},

          // Trailing content
          {"with trailing code", k_with_trailing_code_input, make_with_trailing_code_expected(), true, "y"},

          // Invalid cases
          {"invalid - no semicolon", k_invalid_no_semicolon_input, make_invalid_expected(), false, ""},
          {"invalid - no expression", k_invalid_no_expression_input, make_invalid_expected(), false, ";"},
          {"invalid - empty", k_invalid_empty_input, make_invalid_expected(), false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}