#include "internal_rules.hpp"
#include "utils.hpp"
using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// Test cases for method chaining: foo().bar(), foo().bar().baz(), etc.

// Simple method call on function result: foo().bar()
constexpr auto k_method_on_call_result_should_succeed = true;
constexpr auto k_method_on_call_result_input = "foo().bar()";
inline auto const k_method_on_call_result_expected =
    test_sexp::function_call(test_sexp::var_name("bar"), {test_sexp::function_call(test_sexp::var_name("foo"), {})});

// Method call with arguments on function result: foo().bar(1, 2)
constexpr auto k_method_with_args_on_call_should_succeed = true;
constexpr auto k_method_with_args_on_call_input = "foo().bar(1, 2)";
inline auto const k_method_with_args_on_call_expected = test_sexp::function_call(
    test_sexp::var_name("bar"),
    {test_sexp::function_call(test_sexp::var_name("foo"), {}), test_sexp::integer(1), test_sexp::integer(2)}
);

// Chained method calls: foo().bar().baz()
constexpr auto k_chained_method_calls_should_succeed = true;
constexpr auto k_chained_method_calls_input = "foo().bar().baz()";
inline auto const k_chained_method_calls_expected = test_sexp::function_call(
    test_sexp::var_name("baz"),
    {test_sexp::function_call(test_sexp::var_name("bar"), {test_sexp::function_call(test_sexp::var_name("foo"), {})})}
);

// Field access on function result: foo().field
constexpr auto k_field_on_call_result_should_succeed = true;
constexpr auto k_field_on_call_result_input = "foo().field";
inline auto const k_field_on_call_result_expected =
    test_sexp::field_access(test_sexp::function_call(test_sexp::var_name("foo"), {}), "field");

// Path-based function call: obj.field.method() parses as qualified function name
// This is a function call where the name is a dotted path (qualified name)
constexpr auto k_path_func_call_should_succeed = true;
constexpr auto k_path_func_call_input = "obj.field.method()";
inline auto const k_path_func_call_expected =
    test_sexp::function_call(test_sexp::var_name_path({"obj", "field", "method"}), {});

// Mixed: field access on method result: foo().bar.baz
constexpr auto k_field_on_method_result_should_succeed = true;
constexpr auto k_field_on_method_result_input = "foo().bar.baz";
inline auto const k_field_on_method_result_expected = test_sexp::field_access(
    test_sexp::field_access(test_sexp::function_call(test_sexp::var_name("foo"), {}), "bar"),
    "baz"
);

// Complex chain: foo().bar(1).baz().qux
constexpr auto k_complex_chain_should_succeed = true;
constexpr auto k_complex_chain_input = "foo().bar(1).baz().qux";
inline auto const k_complex_chain_expected = test_sexp::field_access(
    test_sexp::function_call(
        test_sexp::var_name("baz"),
        {test_sexp::function_call(
            test_sexp::var_name("bar"),
            {test_sexp::function_call(test_sexp::var_name("foo"), {}), test_sexp::integer(1)}
        )}
    ),
    "qux"
);

}  // namespace

TEST_CASE("Parse Method Chaining") {
  std::vector<Expr_Params> const params_list = {
      {.name = "method on call result",
       .input = k_method_on_call_result_input,
       .expected = k_method_on_call_result_expected,
       .should_succeed = k_method_on_call_result_should_succeed},
      {.name = "method with args on call",
       .input = k_method_with_args_on_call_input,
       .expected = k_method_with_args_on_call_expected,
       .should_succeed = k_method_with_args_on_call_should_succeed},
      {.name = "chained method calls",
       .input = k_chained_method_calls_input,
       .expected = k_chained_method_calls_expected,
       .should_succeed = k_chained_method_calls_should_succeed},
      {.name = "field on call result",
       .input = k_field_on_call_result_input,
       .expected = k_field_on_call_result_expected,
       .should_succeed = k_field_on_call_result_should_succeed},
      {.name = "path-based function call",
       .input = k_path_func_call_input,
       .expected = k_path_func_call_expected,
       .should_succeed = k_path_func_call_should_succeed},
      {.name = "field on method result",
       .input = k_field_on_method_result_input,
       .expected = k_field_on_method_result_expected,
       .should_succeed = k_field_on_method_result_should_succeed},
      {.name = "complex chain",
       .input = k_complex_chain_input,
       .expected = k_complex_chain_expected,
       .should_succeed = k_complex_chain_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
