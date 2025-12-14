#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// Arithmetic negation tests
constexpr auto k_neg_integer_should_succeed = true;
constexpr auto k_neg_integer_input = "-42";
inline auto const k_neg_integer_expected = test_sexp::unary_expr("-", test_sexp::integer("42"));

constexpr auto k_neg_var_should_succeed = true;
constexpr auto k_neg_var_input = "-x";
inline auto const k_neg_var_expected = test_sexp::unary_expr("-", test_sexp::var_name("x"));

constexpr auto k_double_neg_should_succeed = true;
constexpr auto k_double_neg_input = "--x";
inline auto const k_double_neg_expected =
    test_sexp::unary_expr("-", test_sexp::unary_expr("-", test_sexp::var_name("x")));

// Arithmetic positive tests
constexpr auto k_pos_integer_should_succeed = true;
constexpr auto k_pos_integer_input = "+42";
inline auto const k_pos_integer_expected = test_sexp::unary_expr("+", test_sexp::integer("42"));

constexpr auto k_pos_var_should_succeed = true;
constexpr auto k_pos_var_input = "+x";
inline auto const k_pos_var_expected = test_sexp::unary_expr("+", test_sexp::var_name("x"));

// Logical NOT tests
constexpr auto k_not_var_should_succeed = true;
constexpr auto k_not_var_input = "!flag";
inline auto const k_not_var_expected = test_sexp::unary_expr("!", test_sexp::var_name("flag"));

constexpr auto k_double_not_should_succeed = true;
constexpr auto k_double_not_input = "!!x";
inline auto const k_double_not_expected =
    test_sexp::unary_expr("!", test_sexp::unary_expr("!", test_sexp::var_name("x")));

// Bitwise NOT tests
constexpr auto k_bitnot_var_should_succeed = true;
constexpr auto k_bitnot_var_input = "~bits";
inline auto const k_bitnot_var_expected = test_sexp::unary_expr("~", test_sexp::var_name("bits"));

constexpr auto k_bitnot_integer_should_succeed = true;
constexpr auto k_bitnot_integer_input = "~255";
inline auto const k_bitnot_integer_expected = test_sexp::unary_expr("~", test_sexp::integer("255"));

// Mixed unary operators
constexpr auto k_neg_not_should_succeed = true;
constexpr auto k_neg_not_input = "-!x";
inline auto const k_neg_not_expected = test_sexp::unary_expr("-", test_sexp::unary_expr("!", test_sexp::var_name("x")));

constexpr auto k_not_neg_should_succeed = true;
constexpr auto k_not_neg_input = "!-x";
inline auto const k_not_neg_expected = test_sexp::unary_expr("!", test_sexp::unary_expr("-", test_sexp::var_name("x")));

// Unary with field access
constexpr auto k_neg_field_should_succeed = true;
constexpr auto k_neg_field_input = "-obj.field";
inline auto const k_neg_field_expected =
    test_sexp::unary_expr("-", test_sexp::field_access(test_sexp::var_name("obj"), "field"));

// Unary with function call
constexpr auto k_neg_call_should_succeed = true;
constexpr auto k_neg_call_input = "-calculate()";
inline auto const k_neg_call_expected =
    test_sexp::unary_expr("-", test_sexp::function_call(test_sexp::var_name("calculate"), {}));

}  // namespace

TEST_CASE("Parse Unary_Expr") {
  std::vector<Expr_Params> const params_list = {
      {.name = "neg integer",
       .input = k_neg_integer_input,
       .expected = k_neg_integer_expected,
       .should_succeed = k_neg_integer_should_succeed},
      {.name = "neg variable",
       .input = k_neg_var_input,
       .expected = k_neg_var_expected,
       .should_succeed = k_neg_var_should_succeed},
      {.name = "double negation",
       .input = k_double_neg_input,
       .expected = k_double_neg_expected,
       .should_succeed = k_double_neg_should_succeed},
      {.name = "pos integer",
       .input = k_pos_integer_input,
       .expected = k_pos_integer_expected,
       .should_succeed = k_pos_integer_should_succeed},
      {.name = "pos variable",
       .input = k_pos_var_input,
       .expected = k_pos_var_expected,
       .should_succeed = k_pos_var_should_succeed},
      {.name = "logical NOT",
       .input = k_not_var_input,
       .expected = k_not_var_expected,
       .should_succeed = k_not_var_should_succeed},
      {.name = "double NOT",
       .input = k_double_not_input,
       .expected = k_double_not_expected,
       .should_succeed = k_double_not_should_succeed},
      {.name = "bitwise NOT",
       .input = k_bitnot_var_input,
       .expected = k_bitnot_var_expected,
       .should_succeed = k_bitnot_var_should_succeed},
      {.name = "bitwise NOT integer",
       .input = k_bitnot_integer_input,
       .expected = k_bitnot_integer_expected,
       .should_succeed = k_bitnot_integer_should_succeed},
      {.name = "mixed: neg NOT",
       .input = k_neg_not_input,
       .expected = k_neg_not_expected,
       .should_succeed = k_neg_not_should_succeed},
      {.name = "mixed: NOT neg",
       .input = k_not_neg_input,
       .expected = k_not_neg_expected,
       .should_succeed = k_not_neg_should_succeed},
      {.name = "neg field access",
       .input = k_neg_field_input,
       .expected = k_neg_field_expected,
       .should_succeed = k_neg_field_should_succeed},
      {.name = "neg function call",
       .input = k_neg_call_input,
       .expected = k_neg_call_expected,
       .should_succeed = k_neg_call_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
