#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {

// Var_Name expressions
constexpr auto k_simple_var_name_should_succeed = true;
constexpr auto k_simple_var_name_input = "hello";
inline auto const k_simple_var_name_expected = test_sexp::var_name("hello");

constexpr auto k_dotted_path_should_succeed = true;
constexpr auto k_dotted_path_input = "a.b.c";
inline auto const k_dotted_path_expected =
    test_sexp::field_access(test_sexp::field_access(test_sexp::var_name("a"), "b"), "c");

// Integer literals
constexpr auto k_integer_should_succeed = true;
constexpr auto k_integer_input = "42";
inline auto const k_integer_expected = test_sexp::integer(42);

constexpr auto k_zero_should_succeed = true;
constexpr auto k_zero_input = "0";
inline auto const k_zero_expected = test_sexp::integer(0);

// String literals
constexpr auto k_string_should_succeed = true;
constexpr auto k_string_input = R"("hello")";
inline auto const k_string_expected = test_sexp::string(R"("hello")");

// Function calls - no arguments
constexpr auto k_func_call_should_succeed = true;
constexpr auto k_func_call_input = "hello()";
inline auto const k_func_call_expected = test_sexp::function_call(test_sexp::var_name("hello"), {});

constexpr auto k_func_call_with_path_should_succeed = true;
constexpr auto k_func_call_with_path_input = "hello.a.b()";
inline auto const k_func_call_with_path_expected =
    test_sexp::function_call(test_sexp::var_name_path({"hello", "a", "b"}), {});

constexpr auto k_func_call_with_namespace_should_succeed = true;
constexpr auto k_func_call_with_namespace_input = "A.B.hello()";
inline auto const k_func_call_with_namespace_expected =
    test_sexp::function_call(test_sexp::var_name_path({"A", "B", "hello"}), {});

// Function calls - with arguments
constexpr auto k_func_call_with_args_should_succeed = true;
constexpr auto k_func_call_with_args_input = "hello(a, b, c)";
inline auto const k_func_call_with_args_expected = test_sexp::function_call(
    test_sexp::var_name("hello"),
    {test_sexp::var_name("a"), test_sexp::var_name("b"), test_sexp::var_name("c")}
);

constexpr auto k_func_call_with_path_args_should_succeed = true;
constexpr auto k_func_call_with_path_args_input = "hello(a, b.c.world, c.world)";
inline auto const k_func_call_with_path_args_expected = test_sexp::function_call(
    test_sexp::var_name("hello"),
    {test_sexp::var_name("a"),
     test_sexp::field_access(test_sexp::field_access(test_sexp::var_name("b"), "c"), "world"),
     test_sexp::field_access(test_sexp::var_name("c"), "world")}
);

// Function calls - nested
constexpr auto k_nested_func_calls_should_succeed = true;
constexpr auto k_nested_func_calls_input = "hello(A.B.a.d(), c.world(a))";
inline auto const k_nested_func_calls_expected = test_sexp::function_call(
    test_sexp::var_name("hello"),
    {test_sexp::function_call(test_sexp::var_name_path({"A", "B", "a", "d"}), {}),
     test_sexp::function_call(test_sexp::var_name_path({"c", "world"}), {test_sexp::var_name("a")})}
);

constexpr auto k_with_trailing_text_should_succeed = false;
constexpr auto k_with_trailing_text_input = "hello )";
inline auto const k_with_trailing_text_expected = test_sexp::var_name("hello");

// Invalid cases
constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";
inline auto const k_invalid_empty_expected = test_sexp::var_name_path({});

}  // namespace

TEST_CASE("Parse Expr") {
  std::vector<Expr_Params> const params_list = {
      {.name = "simple variable_name",
       .input = k_simple_var_name_input,
       .expected = k_simple_var_name_expected,
       .should_succeed = k_simple_var_name_should_succeed},
      {.name = "dotted path",
       .input = k_dotted_path_input,
       .expected = k_dotted_path_expected,
       .should_succeed = k_dotted_path_should_succeed},
      {.name = "integer",
       .input = k_integer_input,
       .expected = k_integer_expected,
       .should_succeed = k_integer_should_succeed},
      {.name = "zero", .input = k_zero_input, .expected = k_zero_expected, .should_succeed = k_zero_should_succeed},
      {.name = "string",
       .input = k_string_input,
       .expected = k_string_expected,
       .should_succeed = k_string_should_succeed},
      {.name = "function call",
       .input = k_func_call_input,
       .expected = k_func_call_expected,
       .should_succeed = k_func_call_should_succeed},
      {.name = "function call with path",
       .input = k_func_call_with_path_input,
       .expected = k_func_call_with_path_expected,
       .should_succeed = k_func_call_with_path_should_succeed},
      {.name = "function call with namespace",
       .input = k_func_call_with_namespace_input,
       .expected = k_func_call_with_namespace_expected,
       .should_succeed = k_func_call_with_namespace_should_succeed},
      {.name = "function call with args",
       .input = k_func_call_with_args_input,
       .expected = k_func_call_with_args_expected,
       .should_succeed = k_func_call_with_args_should_succeed},
      {.name = "function call with path args",
       .input = k_func_call_with_path_args_input,
       .expected = k_func_call_with_path_args_expected,
       .should_succeed = k_func_call_with_path_args_should_succeed},
      {.name = "nested function calls",
       .input = k_nested_func_calls_input,
       .expected = k_nested_func_calls_expected,
       .should_succeed = k_nested_func_calls_should_succeed},
      {.name = "with trailing text",
       .input = k_with_trailing_text_input,
       .expected = k_with_trailing_text_expected,
       .should_succeed = k_with_trailing_text_should_succeed},
      {.name = "invalid - empty",
       .input = k_invalid_empty_input,
       .expected = k_invalid_empty_expected,
       .should_succeed = k_invalid_empty_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}