#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// === Arithmetic Operators ===

// Additive: +, -
constexpr auto k_addition_should_succeed = true;
constexpr auto k_addition_input = "1 + 2";
inline auto const k_addition_expected = test_sexp::binary_expr("+", test_sexp::integer(1), test_sexp::integer(2));

constexpr auto k_subtraction_should_succeed = true;
constexpr auto k_subtraction_input = "5 - 3";
inline auto const k_subtraction_expected = test_sexp::binary_expr("-", test_sexp::integer(5), test_sexp::integer(3));

constexpr auto k_addition_no_spaces_should_succeed = true;
constexpr auto k_addition_no_spaces_input = "1+2";
inline auto const k_addition_no_spaces_expected =
    test_sexp::binary_expr("+", test_sexp::integer(1), test_sexp::integer(2));

// Multiplicative: *, /, %
constexpr auto k_multiplication_should_succeed = true;
constexpr auto k_multiplication_input = "2 * 3";
inline auto const k_multiplication_expected = test_sexp::binary_expr("*", test_sexp::integer(2), test_sexp::integer(3));

constexpr auto k_division_should_succeed = true;
constexpr auto k_division_input = "8 / 4";
inline auto const k_division_expected = test_sexp::binary_expr("/", test_sexp::integer(8), test_sexp::integer(4));

constexpr auto k_modulo_should_succeed = true;
constexpr auto k_modulo_input = "10 % 3";
inline auto const k_modulo_expected = test_sexp::binary_expr("%", test_sexp::integer(10), test_sexp::integer(3));

// === Comparison Operators ===

// Note: Using integers on both sides to avoid template parameter ambiguity
constexpr auto k_less_than_should_succeed = true;
constexpr auto k_less_than_input = "5 < 10";
inline auto const k_less_than_expected = test_sexp::binary_expr("<", test_sexp::integer(5), test_sexp::integer(10));

constexpr auto k_greater_than_should_succeed = true;
constexpr auto k_greater_than_input = "10 > 5";
inline auto const k_greater_than_expected = test_sexp::binary_expr(">", test_sexp::integer(10), test_sexp::integer(5));

constexpr auto k_less_equal_should_succeed = true;
constexpr auto k_less_equal_input = "5 <= 10";
inline auto const k_less_equal_expected = test_sexp::binary_expr("<=", test_sexp::integer(5), test_sexp::integer(10));

constexpr auto k_greater_equal_should_succeed = true;
constexpr auto k_greater_equal_input = "10 >= 5";
inline auto const k_greater_equal_expected =
    test_sexp::binary_expr(">=", test_sexp::integer(10), test_sexp::integer(5));

// === Equality Operators ===

constexpr auto k_equal_should_succeed = true;
constexpr auto k_equal_input = "x == 42";
inline auto const k_equal_expected = test_sexp::binary_expr("==", test_sexp::var_name("x"), test_sexp::integer(42));

constexpr auto k_not_equal_should_succeed = true;
constexpr auto k_not_equal_input = "y != 0";
inline auto const k_not_equal_expected = test_sexp::binary_expr("!=", test_sexp::var_name("y"), test_sexp::integer(0));

// === Logical Operators ===

constexpr auto k_logical_and_should_succeed = true;
constexpr auto k_logical_and_input = "a && b";
inline auto const k_logical_and_expected =
    test_sexp::binary_expr("&&", test_sexp::var_name("a"), test_sexp::var_name("b"));

constexpr auto k_logical_or_should_succeed = true;
constexpr auto k_logical_or_input = "x || y";
inline auto const k_logical_or_expected =
    test_sexp::binary_expr("||", test_sexp::var_name("x"), test_sexp::var_name("y"));

// === Precedence Tests ===

// Multiplicative has higher precedence than additive
constexpr auto k_precedence_mul_add_should_succeed = true;
constexpr auto k_precedence_mul_add_input = "1 + 2 * 3";
inline auto const k_precedence_mul_add_expected = test_sexp::binary_expr(
    "+",
    test_sexp::integer(1),
    test_sexp::binary_expr("*", test_sexp::integer(2), test_sexp::integer(3))
);

constexpr auto k_precedence_div_sub_should_succeed = true;
constexpr auto k_precedence_div_sub_input = "10 - 8 / 2";
inline auto const k_precedence_div_sub_expected = test_sexp::binary_expr(
    "-",
    test_sexp::integer(10),
    test_sexp::binary_expr("/", test_sexp::integer(8), test_sexp::integer(2))
);

// Left associativity: same precedence evaluates left to right
constexpr auto k_left_assoc_add_should_succeed = true;
constexpr auto k_left_assoc_add_input = "1 + 2 + 3";
inline auto const k_left_assoc_add_expected = test_sexp::binary_expr(
    "+",
    test_sexp::binary_expr("+", test_sexp::integer(1), test_sexp::integer(2)),
    test_sexp::integer(3)
);

constexpr auto k_left_assoc_mul_should_succeed = true;
constexpr auto k_left_assoc_mul_input = "2 * 3 * 4";
inline auto const k_left_assoc_mul_expected = test_sexp::binary_expr(
    "*",
    test_sexp::binary_expr("*", test_sexp::integer(2), test_sexp::integer(3)),
    test_sexp::integer(4)
);

constexpr auto k_left_assoc_sub_should_succeed = true;
constexpr auto k_left_assoc_sub_input = "10 - 3 - 2";
inline auto const k_left_assoc_sub_expected = test_sexp::binary_expr(
    "-",
    test_sexp::binary_expr("-", test_sexp::integer(10), test_sexp::integer(3)),
    test_sexp::integer(2)
);

// Comparison has lower precedence than additive
constexpr auto k_precedence_cmp_add_should_succeed = true;
constexpr auto k_precedence_cmp_add_input = "1 + 2 > 3 + 4";
inline auto const k_precedence_cmp_add_expected = test_sexp::binary_expr(
    ">",
    test_sexp::binary_expr("+", test_sexp::integer(1), test_sexp::integer(2)),
    test_sexp::binary_expr("+", test_sexp::integer(3), test_sexp::integer(4))
);

// Equality has lower precedence than comparison
constexpr auto k_precedence_eq_cmp_should_succeed = true;
constexpr auto k_precedence_eq_cmp_input = "1 > 2 == 3 < 4";
inline auto const k_precedence_eq_cmp_expected = test_sexp::binary_expr(
    "==",
    test_sexp::binary_expr(">", test_sexp::integer(1), test_sexp::integer(2)),
    test_sexp::binary_expr("<", test_sexp::integer(3), test_sexp::integer(4))
);

// Logical AND has lower precedence than equality
constexpr auto k_precedence_and_eq_should_succeed = true;
constexpr auto k_precedence_and_eq_input = "a == 1 && b == 2";
inline auto const k_precedence_and_eq_expected = test_sexp::binary_expr(
    "&&",
    test_sexp::binary_expr("==", test_sexp::var_name("a"), test_sexp::integer(1)),
    test_sexp::binary_expr("==", test_sexp::var_name("b"), test_sexp::integer(2))
);

// Logical OR has lower precedence than AND
constexpr auto k_precedence_or_and_should_succeed = true;
constexpr auto k_precedence_or_and_input = "a && b || c && d";
inline auto const k_precedence_or_and_expected = test_sexp::binary_expr(
    "||",
    test_sexp::binary_expr("&&", test_sexp::var_name("a"), test_sexp::var_name("b")),
    test_sexp::binary_expr("&&", test_sexp::var_name("c"), test_sexp::var_name("d"))
);

// Complex nested expression
constexpr auto k_complex_expr_should_succeed = true;
constexpr auto k_complex_expr_input = "1 + 2 * 3 == 7 && x > 0";
inline auto const k_complex_expr_expected = test_sexp::binary_expr(
    "&&",
    test_sexp::binary_expr(
        "==",
        test_sexp::binary_expr(
            "+",
            test_sexp::integer(1),
            test_sexp::binary_expr("*", test_sexp::integer(2), test_sexp::integer(3))
        ),
        test_sexp::integer(7)
    ),
    test_sexp::binary_expr(">", test_sexp::var_name("x"), test_sexp::integer(0))
);

// === Whitespace Variations ===

constexpr auto k_extra_spaces_should_succeed = true;
constexpr auto k_extra_spaces_input = "1   +   2";
inline auto const k_extra_spaces_expected = test_sexp::binary_expr("+", test_sexp::integer(1), test_sexp::integer(2));

constexpr auto k_tabs_should_succeed = true;
constexpr auto k_tabs_input = "3\t*\t4";
inline auto const k_tabs_expected = test_sexp::binary_expr("*", test_sexp::integer(3), test_sexp::integer(4));

// === With Variables ===

constexpr auto k_var_addition_should_succeed = true;
constexpr auto k_var_addition_input = "x + y";
inline auto const k_var_addition_expected =
    test_sexp::binary_expr("+", test_sexp::var_name("x"), test_sexp::var_name("y"));

constexpr auto k_var_complex_should_succeed = true;
constexpr auto k_var_complex_input = "a * b + c";
inline auto const k_var_complex_expected = test_sexp::binary_expr(
    "+",
    test_sexp::binary_expr("*", test_sexp::var_name("a"), test_sexp::var_name("b")),
    test_sexp::var_name("c")
);

// === Trailing Content ===

constexpr auto k_with_trailing_should_succeed = false;  // New parser requires full consumption
constexpr auto k_with_trailing_input = "1 + 2 other";
inline auto const k_with_trailing_expected = "{}";

// === Invalid Cases ===
// Note: Parser is lenient and will parse partial expressions, so these
// succeed but stop at the operator. Testing truly invalid syntax is better
// done at statement level where semicolons are required.

constexpr auto k_invalid_only_operator_should_succeed = false;
constexpr auto k_invalid_only_operator_input = "+";
constexpr auto k_invalid_only_operator_expected = "{}";

constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";
constexpr auto k_invalid_empty_expected = "{}";

}  // namespace

TEST_CASE("Parse Binary_Expr") {
  std::vector<Expr_Params> const params_list = {
      // Arithmetic operators
      {.name = "addition",
       .input = k_addition_input,
       .expected = k_addition_expected,
       .should_succeed = k_addition_should_succeed},
      {.name = "subtraction",
       .input = k_subtraction_input,
       .expected = k_subtraction_expected,
       .should_succeed = k_subtraction_should_succeed},
      {.name = "addition no spaces",
       .input = k_addition_no_spaces_input,
       .expected = k_addition_no_spaces_expected,
       .should_succeed = k_addition_no_spaces_should_succeed},
      {.name = "multiplication",
       .input = k_multiplication_input,
       .expected = k_multiplication_expected,
       .should_succeed = k_multiplication_should_succeed},
      {.name = "division",
       .input = k_division_input,
       .expected = k_division_expected,
       .should_succeed = k_division_should_succeed},
      {.name = "modulo",
       .input = k_modulo_input,
       .expected = k_modulo_expected,
       .should_succeed = k_modulo_should_succeed},

      // Comparison operators
      {.name = "less than",
       .input = k_less_than_input,
       .expected = k_less_than_expected,
       .should_succeed = k_less_than_should_succeed},
      {.name = "greater than",
       .input = k_greater_than_input,
       .expected = k_greater_than_expected,
       .should_succeed = k_greater_than_should_succeed},
      {.name = "less equal",
       .input = k_less_equal_input,
       .expected = k_less_equal_expected,
       .should_succeed = k_less_equal_should_succeed},
      {.name = "greater equal",
       .input = k_greater_equal_input,
       .expected = k_greater_equal_expected,
       .should_succeed = k_greater_equal_should_succeed},

      // Equality operators
      {.name = "equal", .input = k_equal_input, .expected = k_equal_expected, .should_succeed = k_equal_should_succeed},
      {.name = "not equal",
       .input = k_not_equal_input,
       .expected = k_not_equal_expected,
       .should_succeed = k_not_equal_should_succeed},

      // Logical operators
      {.name = "logical AND",
       .input = k_logical_and_input,
       .expected = k_logical_and_expected,
       .should_succeed = k_logical_and_should_succeed},
      {.name = "logical OR",
       .input = k_logical_or_input,
       .expected = k_logical_or_expected,
       .should_succeed = k_logical_or_should_succeed},

      // Precedence tests
      {.name = "precedence: mul before add",
       .input = k_precedence_mul_add_input,
       .expected = k_precedence_mul_add_expected,
       .should_succeed = k_precedence_mul_add_should_succeed},
      {.name = "precedence: div before sub",
       .input = k_precedence_div_sub_input,
       .expected = k_precedence_div_sub_expected,
       .should_succeed = k_precedence_div_sub_should_succeed},
      {.name = "left associativity: add",
       .input = k_left_assoc_add_input,
       .expected = k_left_assoc_add_expected,
       .should_succeed = k_left_assoc_add_should_succeed},
      {.name = "left associativity: mul",
       .input = k_left_assoc_mul_input,
       .expected = k_left_assoc_mul_expected,
       .should_succeed = k_left_assoc_mul_should_succeed},
      {.name = "left associativity: sub",
       .input = k_left_assoc_sub_input,
       .expected = k_left_assoc_sub_expected,
       .should_succeed = k_left_assoc_sub_should_succeed},
      {.name = "precedence: cmp after add",
       .input = k_precedence_cmp_add_input,
       .expected = k_precedence_cmp_add_expected,
       .should_succeed = k_precedence_cmp_add_should_succeed},
      {.name = "precedence: eq after cmp",
       .input = k_precedence_eq_cmp_input,
       .expected = k_precedence_eq_cmp_expected,
       .should_succeed = k_precedence_eq_cmp_should_succeed},
      {.name = "precedence: and after eq",
       .input = k_precedence_and_eq_input,
       .expected = k_precedence_and_eq_expected,
       .should_succeed = k_precedence_and_eq_should_succeed},
      {.name = "precedence: or after and",
       .input = k_precedence_or_and_input,
       .expected = k_precedence_or_and_expected,
       .should_succeed = k_precedence_or_and_should_succeed},
      {.name = "complex expression",
       .input = k_complex_expr_input,
       .expected = k_complex_expr_expected,
       .should_succeed = k_complex_expr_should_succeed},

      // Whitespace variations
      {.name = "extra spaces",
       .input = k_extra_spaces_input,
       .expected = k_extra_spaces_expected,
       .should_succeed = k_extra_spaces_should_succeed},
      {.name = "tabs", .input = k_tabs_input, .expected = k_tabs_expected, .should_succeed = k_tabs_should_succeed},

      // With variables
      {.name = "variable addition",
       .input = k_var_addition_input,
       .expected = k_var_addition_expected,
       .should_succeed = k_var_addition_should_succeed},
      {.name = "variable complex",
       .input = k_var_complex_input,
       .expected = k_var_complex_expected,
       .should_succeed = k_var_complex_should_succeed},

      // Trailing content
      {.name = "with trailing",
       .input = k_with_trailing_input,
       .expected = k_with_trailing_expected,
       .should_succeed = k_with_trailing_should_succeed},

      // Invalid cases
      {.name = "invalid - only operator",
       .input = k_invalid_only_operator_input,
       .expected = k_invalid_only_operator_expected,
       .should_succeed = k_invalid_only_operator_should_succeed},
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
