#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// Basic while loop
constexpr auto k_basic_while_should_succeed = true;
constexpr auto k_basic_while_input = "while x { return 1; }";
inline auto const k_basic_while_expected = test_sexp::while_expr(
    test_sexp::var_name("x"),
    test_sexp::block({test_sexp::return_statement(test_sexp::integer(1))})
);

// While with comparison condition
constexpr auto k_while_comparison_should_succeed = true;
constexpr auto k_while_comparison_input = "while x < 10 { return x; }";
inline auto const k_while_comparison_expected = test_sexp::while_expr(
    test_sexp::binary_expr("<", test_sexp::var_name("x"), test_sexp::integer(10)),
    test_sexp::block({test_sexp::return_statement(test_sexp::var_name("x"))})
);

// While with complex condition
constexpr auto k_while_complex_condition_should_succeed = true;
constexpr auto k_while_complex_condition_input = "while x > 0 && y < 100 { foo(); }";
inline auto const k_while_complex_condition_expected = test_sexp::while_expr(
    test_sexp::binary_expr(
        "&&",
        test_sexp::binary_expr(">", test_sexp::var_name("x"), test_sexp::integer(0)),
        test_sexp::binary_expr("<", test_sexp::var_name("y"), test_sexp::integer(100))
    ),
    test_sexp::block({test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name("foo"), {}))})
);

// While with empty body
constexpr auto k_while_empty_body_should_succeed = true;
constexpr auto k_while_empty_body_input = "while condition {}";
inline auto const k_while_empty_body_expected =
    test_sexp::while_expr(test_sexp::var_name("condition"), test_sexp::block({}));

// While with multiple statements
constexpr auto k_while_multiple_statements_should_succeed = true;
constexpr auto k_while_multiple_statements_input = "while x { foo(); bar(); return x; }";
inline auto const k_while_multiple_statements_expected = test_sexp::while_expr(
    test_sexp::var_name("x"),
    test_sexp::block(
        {test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name("foo"), {})),
         test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name("bar"), {})),
         test_sexp::return_statement(test_sexp::var_name("x"))}
    )
);

// While with function call condition
constexpr auto k_while_func_condition_should_succeed = true;
constexpr auto k_while_func_condition_input = "while has_more() { process(); }";
inline auto const k_while_func_condition_expected = test_sexp::while_expr(
    test_sexp::function_call(test_sexp::var_name("has_more"), {}),
    test_sexp::block({test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name("process"), {}))})
);

// While with unary operator condition
constexpr auto k_while_unary_condition_should_succeed = true;
constexpr auto k_while_unary_condition_input = "while !done { work(); }";
inline auto const k_while_unary_condition_expected = test_sexp::while_expr(
    test_sexp::unary_expr("!", test_sexp::var_name("done")),
    test_sexp::block({test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name("work"), {}))})
);

// Nested while loops
constexpr auto k_nested_while_should_succeed = true;
constexpr auto k_nested_while_input = "while x { while y { foo(); } }";
inline auto const k_nested_while_expected = test_sexp::while_expr(
    test_sexp::var_name("x"),
    test_sexp::block({test_sexp::while_statement(
        test_sexp::while_expr(
            test_sexp::var_name("y"),
            test_sexp::block(
                {test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name("foo"), {}))}
            )
        )
    )})
);

// While with both < and > in condition
constexpr auto k_while_less_and_greater_should_succeed = true;
constexpr auto k_while_less_and_greater_input = "while x < 10 && y > 5 { process(); }";
inline auto const k_while_less_and_greater_expected = test_sexp::while_expr(
    test_sexp::binary_expr(
        "&&",
        test_sexp::binary_expr("<", test_sexp::var_name("x"), test_sexp::integer(10)),
        test_sexp::binary_expr(">", test_sexp::var_name("y"), test_sexp::integer(5))
    ),
    test_sexp::block({test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name("process"), {}))})
);

// Invalid cases
constexpr auto k_missing_condition_should_succeed = false;
constexpr auto k_missing_condition_input = "while { return 1; }";
inline auto const k_missing_condition_expected = "";

constexpr auto k_missing_body_should_succeed = false;
constexpr auto k_missing_body_input = "while x";
inline auto const k_missing_body_expected = "";

constexpr auto k_missing_braces_should_succeed = false;
constexpr auto k_missing_braces_input = "while x return 1;";
inline auto const k_missing_braces_expected = "";

// NOTE: With parenthesized expressions support, `while (x)` is valid because `(x)` is a valid expr,
// it is consistent with allowing (a + b).method()
constexpr auto k_parentheses_condition_should_succeed = true;
constexpr auto k_parentheses_condition_input = "while (x) { return 1; }";
inline auto const k_parentheses_condition_expected = test_sexp::while_expr(
    test_sexp::var_name("x"),
    test_sexp::block({test_sexp::return_statement(test_sexp::integer(1))})
);

}  // namespace

TEST_CASE("Parse While_Expr") {
  std::vector<Expr_Params> const params_list = {
      {.name = "basic while",
       .input = k_basic_while_input,
       .expected = k_basic_while_expected,
       .should_succeed = k_basic_while_should_succeed},
      {.name = "while with comparison",
       .input = k_while_comparison_input,
       .expected = k_while_comparison_expected,
       .should_succeed = k_while_comparison_should_succeed},
      {.name = "while with complex condition",
       .input = k_while_complex_condition_input,
       .expected = k_while_complex_condition_expected,
       .should_succeed = k_while_complex_condition_should_succeed},
      {.name = "while with empty body",
       .input = k_while_empty_body_input,
       .expected = k_while_empty_body_expected,
       .should_succeed = k_while_empty_body_should_succeed},
      {.name = "while with multiple statements",
       .input = k_while_multiple_statements_input,
       .expected = k_while_multiple_statements_expected,
       .should_succeed = k_while_multiple_statements_should_succeed},
      {.name = "while with function condition",
       .input = k_while_func_condition_input,
       .expected = k_while_func_condition_expected,
       .should_succeed = k_while_func_condition_should_succeed},
      {.name = "while with unary condition",
       .input = k_while_unary_condition_input,
       .expected = k_while_unary_condition_expected,
       .should_succeed = k_while_unary_condition_should_succeed},
      {.name = "nested while loops",
       .input = k_nested_while_input,
       .expected = k_nested_while_expected,
       .should_succeed = k_nested_while_should_succeed},
      {.name = "while with < and >",
       .input = k_while_less_and_greater_input,
       .expected = k_while_less_and_greater_expected,
       .should_succeed = k_while_less_and_greater_should_succeed},
      {.name = "invalid: missing condition",
       .input = k_missing_condition_input,
       .expected = k_missing_condition_expected,
       .should_succeed = k_missing_condition_should_succeed},
      {.name = "invalid: missing body",
       .input = k_missing_body_input,
       .expected = k_missing_body_expected,
       .should_succeed = k_missing_body_should_succeed},
      {.name = "invalid: missing braces",
       .input = k_missing_braces_input,
       .expected = k_missing_braces_expected,
       .should_succeed = k_missing_braces_should_succeed},
      {.name = "invalid: parentheses around condition",
       .input = k_parentheses_condition_input,
       .expected = k_parentheses_condition_expected,
       .should_succeed = k_parentheses_condition_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
