#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {

// Variable_Name expressions
constexpr auto k_simple_variable_name_should_succeed = true;
constexpr auto k_simple_variable_name_input = "hello";
inline auto const k_simple_variable_name_expected = test_json::var_name("hello");

constexpr auto k_dotted_path_should_succeed = true;
constexpr auto k_dotted_path_input = "a.b.c";
inline auto const k_dotted_path_expected =
    test_json::field_access(test_json::field_access(test_json::var_name("a"), "b"), "c");

// Integer literals
constexpr auto k_integer_should_succeed = true;
constexpr auto k_integer_input = "42";
inline auto const k_integer_expected = test_json::integer(42);

constexpr auto k_zero_should_succeed = true;
constexpr auto k_zero_input = "0";
inline auto const k_zero_expected = test_json::integer(0);

// String literals
constexpr auto k_string_should_succeed = true;
constexpr auto k_string_input = R"("hello")";
inline auto const k_string_expected = test_json::string(R"(\"hello\")");

// Function calls - no arguments
constexpr auto k_function_call_should_succeed = true;
constexpr auto k_function_call_input = "hello()";
inline auto const k_function_call_expected = test_json::function_call(test_json::var_name("hello"), {});

constexpr auto k_function_call_with_path_should_succeed = true;
constexpr auto k_function_call_with_path_input = "hello.a.b()";
inline auto const k_function_call_with_path_expected =
    test_json::function_call(test_json::var_name_path({"hello", "a", "b"}), {});

constexpr auto k_function_call_with_namespace_should_succeed = true;
constexpr auto k_function_call_with_namespace_input = "A.B.hello()";
inline auto const k_function_call_with_namespace_expected =
    test_json::function_call(test_json::var_name_path({"A", "B", "hello"}), {});

// Function calls - with arguments
constexpr auto k_function_call_with_args_should_succeed = true;
constexpr auto k_function_call_with_args_input = "hello(a, b, c)";
inline auto const k_function_call_with_args_expected = test_json::function_call(
    test_json::var_name("hello"), {test_json::var_name("a"), test_json::var_name("b"), test_json::var_name("c")}
);

constexpr auto k_function_call_with_path_args_should_succeed = true;
constexpr auto k_function_call_with_path_args_input = "hello(a, b.c.world, c.world)";
inline auto const k_function_call_with_path_args_expected = test_json::function_call(
    test_json::var_name("hello"),
    {test_json::var_name("a"), test_json::field_access(test_json::field_access(test_json::var_name("b"), "c"), "world"),
     test_json::field_access(test_json::var_name("c"), "world")}
);

// Function calls - nested
constexpr auto k_nested_function_calls_should_succeed = true;
constexpr auto k_nested_function_calls_input = "hello(A.B.a.d(), c.world(a))";
inline auto const k_nested_function_calls_expected = test_json::function_call(
    test_json::var_name("hello"),
    {test_json::function_call(test_json::var_name_path({"A", "B", "a", "d"}), {}),
     test_json::function_call(test_json::var_name_path({"c", "world"}), {test_json::var_name("a")})}
);

constexpr auto k_with_trailing_text_should_succeed = true;
constexpr auto k_with_trailing_text_input = "hello )";
inline auto const k_with_trailing_text_expected = test_json::var_name("hello");

// Invalid cases
constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";
inline auto const k_invalid_empty_expected = test_json::var_name_path({});

}  // namespace

TEST_CASE("Parse Expr", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          {"simple variable_name", k_simple_variable_name_input, k_simple_variable_name_expected,
           k_simple_variable_name_should_succeed},
          {"dotted path", k_dotted_path_input, k_dotted_path_expected, k_dotted_path_should_succeed},
          {"integer", k_integer_input, k_integer_expected, k_integer_should_succeed},
          {"zero", k_zero_input, k_zero_expected, k_zero_should_succeed},
          {"string", k_string_input, k_string_expected, k_string_should_succeed},
          {"function call", k_function_call_input, k_function_call_expected, k_function_call_should_succeed},
          {"function call with path", k_function_call_with_path_input, k_function_call_with_path_expected,
           k_function_call_with_path_should_succeed},
          {"function call with namespace", k_function_call_with_namespace_input,
           k_function_call_with_namespace_expected, k_function_call_with_namespace_should_succeed},
          {"function call with args", k_function_call_with_args_input, k_function_call_with_args_expected,
           k_function_call_with_args_should_succeed},
          {"function call with path args", k_function_call_with_path_args_input,
           k_function_call_with_path_args_expected, k_function_call_with_path_args_should_succeed},
          {"nested function calls", k_nested_function_calls_input, k_nested_function_calls_expected,
           k_nested_function_calls_should_succeed},
          {"with trailing text", k_with_trailing_text_input, k_with_trailing_text_expected,
           k_with_trailing_text_should_succeed},
          {"invalid - empty", k_invalid_empty_input, k_invalid_empty_expected, k_invalid_empty_should_succeed},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}