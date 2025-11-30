#include "utils.hpp"

using life_lang::ast::Expr;
using life_lang::ast::make_expr;
using life_lang::ast::make_function_call_expr;
using life_lang::ast::make_integer;
using life_lang::ast::make_path;
using life_lang::ast::make_path_segment;
using life_lang::ast::make_string;

PARSE_TEST(Expr, expr)

namespace {
// Path expressions
constexpr auto k_simple_path_input = "hello";
constexpr auto k_dotted_path_input = "a.b.c";

// Integer literals
constexpr auto k_integer_input = "42";
constexpr auto k_zero_input = "0";

// String literals
constexpr auto k_string_input = R"("hello")";

// Function calls - no arguments
constexpr auto k_function_call_input = "hello()";
constexpr auto k_function_call_with_path_input = "hello.a.b()";
constexpr auto k_function_call_with_namespace_input = "A.B.hello()";

// Function calls - with arguments
constexpr auto k_function_call_with_args_input = "hello(a, b, c)";
auto make_function_call_with_args_expected() {
  return make_expr(make_function_call_expr(
      make_path("hello"), {make_expr(make_path("a")), make_expr(make_path("b")), make_expr(make_path("c"))}
  ));
}

constexpr auto k_function_call_with_path_args_input = "hello(a, b.c.world, c.world)";
auto make_function_call_with_path_args_expected() {
  return make_expr(make_function_call_expr(
      make_path("hello"),
      {make_expr(make_path("a")), make_expr(make_path("b", "c", "world")), make_expr(make_path("c", "world"))}
  ));
}

// Function calls - nested
constexpr auto k_nested_function_calls_input = "hello(A.B.a.d(), c.world(a))";
auto make_nested_function_calls_expected() {
  return make_expr(make_function_call_expr(
      make_path("hello"), {make_expr(make_function_call_expr(make_path("A", "B", "a", "d"), {})),
                           make_expr(make_function_call_expr(make_path("c", "world"), {make_expr(make_path("a"))}))}
  ));
}

// Function calls - with templates
constexpr auto k_function_call_with_template_input = "A.B<Int, Double>.hello.a.b()";
auto make_function_call_with_template_expected() {
  return make_expr(make_function_call_expr(
      make_path("A", make_path_segment("B", {make_path("Int"), make_path("Double")}), "hello", "a", "b"), {}
  ));
}

constexpr auto k_with_trailing_text_input = "hello )";

// Invalid cases
constexpr auto k_invalid_empty_input = "";
}  // namespace

TEST_CASE("Parse Expr", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          // Path expressions
          {"simple path", k_simple_path_input, make_expr(make_path("hello")), true, ""},
          {"dotted path", k_dotted_path_input, make_expr(make_path("a", "b", "c")), true, ""},

          // Integer literals
          {"integer", k_integer_input, make_expr(make_integer("42")), true, ""},
          {"zero", k_zero_input, make_expr(make_integer("0")), true, ""},

          // String literals
          {"string", k_string_input, make_expr(make_string(R"("hello")")), true, ""},

          // Function calls - no arguments
          {"function call", k_function_call_input, make_expr(make_function_call_expr(make_path("hello"), {})), true,
           ""},
          {"function call with path", k_function_call_with_path_input,
           make_expr(make_function_call_expr(make_path("hello", "a", "b"), {})), true, ""},
          {"function call with namespace", k_function_call_with_namespace_input,
           make_expr(make_function_call_expr(make_path("A", "B", "hello"), {})), true, ""},

          // Function calls - with arguments
          {"function call with args", k_function_call_with_args_input, make_function_call_with_args_expected(), true,
           ""},
          {"function call with path args", k_function_call_with_path_args_input,
           make_function_call_with_path_args_expected(), true, ""},

          // Function calls - nested
          {"nested function calls", k_nested_function_calls_input, make_nested_function_calls_expected(), true, ""},

          // Function calls - with templates
          {"function call with template", k_function_call_with_template_input,
           make_function_call_with_template_expected(), true, ""},

          {"with trailing text", k_with_trailing_text_input, make_expr(make_path("hello")), true, ")"},

          // Invalid cases
          {"invalid - empty", k_invalid_empty_input, make_expr(make_path()), false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}