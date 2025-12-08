#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// Arithmetic negation tests
constexpr auto k_neg_integer_should_succeed = true;
constexpr auto k_neg_integer_input = "-42";
inline auto const k_neg_integer_expected = test_json::unary_expr("-", test_json::integer("42"));

constexpr auto k_neg_variable_should_succeed = true;
constexpr auto k_neg_variable_input = "-x";
inline auto const k_neg_variable_expected = test_json::unary_expr("-", test_json::var_name("x"));

constexpr auto k_double_neg_should_succeed = true;
constexpr auto k_double_neg_input = "--x";
inline auto const k_double_neg_expected =
    test_json::unary_expr("-", test_json::unary_expr("-", test_json::var_name("x")));

// Arithmetic positive tests
constexpr auto k_pos_integer_should_succeed = true;
constexpr auto k_pos_integer_input = "+42";
inline auto const k_pos_integer_expected = test_json::unary_expr("+", test_json::integer("42"));

constexpr auto k_pos_variable_should_succeed = true;
constexpr auto k_pos_variable_input = "+x";
inline auto const k_pos_variable_expected = test_json::unary_expr("+", test_json::var_name("x"));

// Logical NOT tests
constexpr auto k_not_variable_should_succeed = true;
constexpr auto k_not_variable_input = "!flag";
inline auto const k_not_variable_expected = test_json::unary_expr("!", test_json::var_name("flag"));

constexpr auto k_double_not_should_succeed = true;
constexpr auto k_double_not_input = "!!x";
inline auto const k_double_not_expected =
    test_json::unary_expr("!", test_json::unary_expr("!", test_json::var_name("x")));

// Bitwise NOT tests
constexpr auto k_bitnot_variable_should_succeed = true;
constexpr auto k_bitnot_variable_input = "~bits";
inline auto const k_bitnot_variable_expected = test_json::unary_expr("~", test_json::var_name("bits"));

constexpr auto k_bitnot_integer_should_succeed = true;
constexpr auto k_bitnot_integer_input = "~255";
inline auto const k_bitnot_integer_expected = test_json::unary_expr("~", test_json::integer("255"));

// Mixed unary operators
constexpr auto k_neg_not_should_succeed = true;
constexpr auto k_neg_not_input = "-!x";
inline auto const k_neg_not_expected = test_json::unary_expr("-", test_json::unary_expr("!", test_json::var_name("x")));

constexpr auto k_not_neg_should_succeed = true;
constexpr auto k_not_neg_input = "!-x";
inline auto const k_not_neg_expected = test_json::unary_expr("!", test_json::unary_expr("-", test_json::var_name("x")));

// Unary with field access
constexpr auto k_neg_field_should_succeed = true;
constexpr auto k_neg_field_input = "-obj.field";
inline auto const k_neg_field_expected =
    test_json::unary_expr("-", test_json::field_access(test_json::var_name("obj"), "field"));

// Unary with function call
constexpr auto k_neg_call_should_succeed = true;
constexpr auto k_neg_call_input = "-calculate()";
inline auto const k_neg_call_expected =
    test_json::unary_expr("-", test_json::function_call(test_json::var_name("calculate"), {}));

}  // namespace

TEST_CASE("Parse Unary_Expr", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          {"neg integer", k_neg_integer_input, k_neg_integer_expected, k_neg_integer_should_succeed},
          {"neg variable", k_neg_variable_input, k_neg_variable_expected, k_neg_variable_should_succeed},
          {"double negation", k_double_neg_input, k_double_neg_expected, k_double_neg_should_succeed},
          {"pos integer", k_pos_integer_input, k_pos_integer_expected, k_pos_integer_should_succeed},
          {"pos variable", k_pos_variable_input, k_pos_variable_expected, k_pos_variable_should_succeed},
          {"logical NOT", k_not_variable_input, k_not_variable_expected, k_not_variable_should_succeed},
          {"double NOT", k_double_not_input, k_double_not_expected, k_double_not_should_succeed},
          {"bitwise NOT", k_bitnot_variable_input, k_bitnot_variable_expected, k_bitnot_variable_should_succeed},
          {"bitwise NOT integer", k_bitnot_integer_input, k_bitnot_integer_expected, k_bitnot_integer_should_succeed},
          {"mixed: neg NOT", k_neg_not_input, k_neg_not_expected, k_neg_not_should_succeed},
          {"mixed: NOT neg", k_not_neg_input, k_not_neg_expected, k_not_neg_should_succeed},
          {"neg field access", k_neg_field_input, k_neg_field_expected, k_neg_field_should_succeed},
          {"neg function call", k_neg_call_input, k_neg_call_expected, k_neg_call_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
