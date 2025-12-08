#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// === Arithmetic Operators ===

// Additive: +, -
constexpr auto k_addition_should_succeed = true;
constexpr auto k_addition_input = "1 + 2";
inline auto const k_addition_expected = test_json::binary_expr("+", test_json::integer(1), test_json::integer(2));

constexpr auto k_subtraction_should_succeed = true;
constexpr auto k_subtraction_input = "5 - 3";
inline auto const k_subtraction_expected = test_json::binary_expr("-", test_json::integer(5), test_json::integer(3));

constexpr auto k_addition_no_spaces_should_succeed = true;
constexpr auto k_addition_no_spaces_input = "1+2";
inline auto const k_addition_no_spaces_expected =
    test_json::binary_expr("+", test_json::integer(1), test_json::integer(2));

// Multiplicative: *, /, %
constexpr auto k_multiplication_should_succeed = true;
constexpr auto k_multiplication_input = "2 * 3";
inline auto const k_multiplication_expected = test_json::binary_expr("*", test_json::integer(2), test_json::integer(3));

constexpr auto k_division_should_succeed = true;
constexpr auto k_division_input = "8 / 4";
inline auto const k_division_expected = test_json::binary_expr("/", test_json::integer(8), test_json::integer(4));

constexpr auto k_modulo_should_succeed = true;
constexpr auto k_modulo_input = "10 % 3";
inline auto const k_modulo_expected = test_json::binary_expr("%", test_json::integer(10), test_json::integer(3));

// === Comparison Operators ===

// Note: Using integers on both sides to avoid template parameter ambiguity
constexpr auto k_less_than_should_succeed = true;
constexpr auto k_less_than_input = "5 < 10";
inline auto const k_less_than_expected = test_json::binary_expr("<", test_json::integer(5), test_json::integer(10));

constexpr auto k_greater_than_should_succeed = true;
constexpr auto k_greater_than_input = "10 > 5";
inline auto const k_greater_than_expected = test_json::binary_expr(">", test_json::integer(10), test_json::integer(5));

constexpr auto k_less_equal_should_succeed = true;
constexpr auto k_less_equal_input = "5 <= 10";
inline auto const k_less_equal_expected = test_json::binary_expr("<=", test_json::integer(5), test_json::integer(10));

constexpr auto k_greater_equal_should_succeed = true;
constexpr auto k_greater_equal_input = "10 >= 5";
inline auto const k_greater_equal_expected =
    test_json::binary_expr(">=", test_json::integer(10), test_json::integer(5));

// === Equality Operators ===

constexpr auto k_equal_should_succeed = true;
constexpr auto k_equal_input = "x == 42";
inline auto const k_equal_expected = test_json::binary_expr("==", test_json::var_name("x"), test_json::integer(42));

constexpr auto k_not_equal_should_succeed = true;
constexpr auto k_not_equal_input = "y != 0";
inline auto const k_not_equal_expected = test_json::binary_expr("!=", test_json::var_name("y"), test_json::integer(0));

// === Logical Operators ===

constexpr auto k_logical_and_should_succeed = true;
constexpr auto k_logical_and_input = "a && b";
inline auto const k_logical_and_expected =
    test_json::binary_expr("&&", test_json::var_name("a"), test_json::var_name("b"));

constexpr auto k_logical_or_should_succeed = true;
constexpr auto k_logical_or_input = "x || y";
inline auto const k_logical_or_expected =
    test_json::binary_expr("||", test_json::var_name("x"), test_json::var_name("y"));

// === Precedence Tests ===

// Multiplicative has higher precedence than additive
constexpr auto k_precedence_mul_add_should_succeed = true;
constexpr auto k_precedence_mul_add_input = "1 + 2 * 3";
inline auto const k_precedence_mul_add_expected = test_json::binary_expr(
    "+", test_json::integer(1), test_json::binary_expr("*", test_json::integer(2), test_json::integer(3))
);

constexpr auto k_precedence_div_sub_should_succeed = true;
constexpr auto k_precedence_div_sub_input = "10 - 8 / 2";
inline auto const k_precedence_div_sub_expected = test_json::binary_expr(
    "-", test_json::integer(10), test_json::binary_expr("/", test_json::integer(8), test_json::integer(2))
);

// Left associativity: same precedence evaluates left to right
constexpr auto k_left_assoc_add_should_succeed = true;
constexpr auto k_left_assoc_add_input = "1 + 2 + 3";
inline auto const k_left_assoc_add_expected = test_json::binary_expr(
    "+", test_json::binary_expr("+", test_json::integer(1), test_json::integer(2)), test_json::integer(3)
);

constexpr auto k_left_assoc_mul_should_succeed = true;
constexpr auto k_left_assoc_mul_input = "2 * 3 * 4";
inline auto const k_left_assoc_mul_expected = test_json::binary_expr(
    "*", test_json::binary_expr("*", test_json::integer(2), test_json::integer(3)), test_json::integer(4)
);

constexpr auto k_left_assoc_sub_should_succeed = true;
constexpr auto k_left_assoc_sub_input = "10 - 3 - 2";
inline auto const k_left_assoc_sub_expected = test_json::binary_expr(
    "-", test_json::binary_expr("-", test_json::integer(10), test_json::integer(3)), test_json::integer(2)
);

// Comparison has lower precedence than additive
constexpr auto k_precedence_cmp_add_should_succeed = true;
constexpr auto k_precedence_cmp_add_input = "1 + 2 > 3 + 4";
inline auto const k_precedence_cmp_add_expected = test_json::binary_expr(
    ">", test_json::binary_expr("+", test_json::integer(1), test_json::integer(2)),
    test_json::binary_expr("+", test_json::integer(3), test_json::integer(4))
);

// Equality has lower precedence than comparison
constexpr auto k_precedence_eq_cmp_should_succeed = true;
constexpr auto k_precedence_eq_cmp_input = "1 > 2 == 3 < 4";
inline auto const k_precedence_eq_cmp_expected = test_json::binary_expr(
    "==", test_json::binary_expr(">", test_json::integer(1), test_json::integer(2)),
    test_json::binary_expr("<", test_json::integer(3), test_json::integer(4))
);

// Logical AND has lower precedence than equality
constexpr auto k_precedence_and_eq_should_succeed = true;
constexpr auto k_precedence_and_eq_input = "a == 1 && b == 2";
inline auto const k_precedence_and_eq_expected = test_json::binary_expr(
    "&&", test_json::binary_expr("==", test_json::var_name("a"), test_json::integer(1)),
    test_json::binary_expr("==", test_json::var_name("b"), test_json::integer(2))
);

// Logical OR has lower precedence than AND
constexpr auto k_precedence_or_and_should_succeed = true;
constexpr auto k_precedence_or_and_input = "a && b || c && d";
inline auto const k_precedence_or_and_expected = test_json::binary_expr(
    "||", test_json::binary_expr("&&", test_json::var_name("a"), test_json::var_name("b")),
    test_json::binary_expr("&&", test_json::var_name("c"), test_json::var_name("d"))
);

// Complex nested expression
constexpr auto k_complex_expr_should_succeed = true;
constexpr auto k_complex_expr_input = "1 + 2 * 3 == 7 && x > 0";
inline auto const k_complex_expr_expected = test_json::binary_expr(
    "&&",
    test_json::binary_expr(
        "==",
        test_json::binary_expr(
            "+", test_json::integer(1), test_json::binary_expr("*", test_json::integer(2), test_json::integer(3))
        ),
        test_json::integer(7)
    ),
    test_json::binary_expr(">", test_json::var_name("x"), test_json::integer(0))
);

// === Whitespace Variations ===

constexpr auto k_extra_spaces_should_succeed = true;
constexpr auto k_extra_spaces_input = "1   +   2";
inline auto const k_extra_spaces_expected = test_json::binary_expr("+", test_json::integer(1), test_json::integer(2));

constexpr auto k_tabs_should_succeed = true;
constexpr auto k_tabs_input = "3\t*\t4";
inline auto const k_tabs_expected = test_json::binary_expr("*", test_json::integer(3), test_json::integer(4));

// === With Variables ===

constexpr auto k_var_addition_should_succeed = true;
constexpr auto k_var_addition_input = "x + y";
inline auto const k_var_addition_expected =
    test_json::binary_expr("+", test_json::var_name("x"), test_json::var_name("y"));

constexpr auto k_var_complex_should_succeed = true;
constexpr auto k_var_complex_input = "a * b + c";
inline auto const k_var_complex_expected = test_json::binary_expr(
    "+", test_json::binary_expr("*", test_json::var_name("a"), test_json::var_name("b")), test_json::var_name("c")
);

// === Trailing Content ===

constexpr auto k_with_trailing_should_succeed = true;
constexpr auto k_with_trailing_input = "1 + 2 other";
inline auto const k_with_trailing_expected = test_json::binary_expr("+", test_json::integer(1), test_json::integer(2));

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

TEST_CASE("Parse Binary_Expr", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          // Arithmetic operators
          {"addition", k_addition_input, k_addition_expected, k_addition_should_succeed},
          {"subtraction", k_subtraction_input, k_subtraction_expected, k_subtraction_should_succeed},
          {"addition no spaces", k_addition_no_spaces_input, k_addition_no_spaces_expected,
           k_addition_no_spaces_should_succeed},
          {"multiplication", k_multiplication_input, k_multiplication_expected, k_multiplication_should_succeed},
          {"division", k_division_input, k_division_expected, k_division_should_succeed},
          {"modulo", k_modulo_input, k_modulo_expected, k_modulo_should_succeed},

          // Comparison operators
          {"less than", k_less_than_input, k_less_than_expected, k_less_than_should_succeed},
          {"greater than", k_greater_than_input, k_greater_than_expected, k_greater_than_should_succeed},
          {"less equal", k_less_equal_input, k_less_equal_expected, k_less_equal_should_succeed},
          {"greater equal", k_greater_equal_input, k_greater_equal_expected, k_greater_equal_should_succeed},

          // Equality operators
          {"equal", k_equal_input, k_equal_expected, k_equal_should_succeed},
          {"not equal", k_not_equal_input, k_not_equal_expected, k_not_equal_should_succeed},

          // Logical operators
          {"logical AND", k_logical_and_input, k_logical_and_expected, k_logical_and_should_succeed},
          {"logical OR", k_logical_or_input, k_logical_or_expected, k_logical_or_should_succeed},

          // Precedence tests
          {"precedence: mul before add", k_precedence_mul_add_input, k_precedence_mul_add_expected,
           k_precedence_mul_add_should_succeed},
          {"precedence: div before sub", k_precedence_div_sub_input, k_precedence_div_sub_expected,
           k_precedence_div_sub_should_succeed},
          {"left associativity: add", k_left_assoc_add_input, k_left_assoc_add_expected,
           k_left_assoc_add_should_succeed},
          {"left associativity: mul", k_left_assoc_mul_input, k_left_assoc_mul_expected,
           k_left_assoc_mul_should_succeed},
          {"left associativity: sub", k_left_assoc_sub_input, k_left_assoc_sub_expected,
           k_left_assoc_sub_should_succeed},
          {"precedence: cmp after add", k_precedence_cmp_add_input, k_precedence_cmp_add_expected,
           k_precedence_cmp_add_should_succeed},
          {"precedence: eq after cmp", k_precedence_eq_cmp_input, k_precedence_eq_cmp_expected,
           k_precedence_eq_cmp_should_succeed},
          {"precedence: and after eq", k_precedence_and_eq_input, k_precedence_and_eq_expected,
           k_precedence_and_eq_should_succeed},
          {"precedence: or after and", k_precedence_or_and_input, k_precedence_or_and_expected,
           k_precedence_or_and_should_succeed},
          {"complex expression", k_complex_expr_input, k_complex_expr_expected, k_complex_expr_should_succeed},

          // Whitespace variations
          {"extra spaces", k_extra_spaces_input, k_extra_spaces_expected, k_extra_spaces_should_succeed},
          {"tabs", k_tabs_input, k_tabs_expected, k_tabs_should_succeed},

          // With variables
          {"variable addition", k_var_addition_input, k_var_addition_expected, k_var_addition_should_succeed},
          {"variable complex", k_var_complex_input, k_var_complex_expected, k_var_complex_should_succeed},

          // Trailing content
          {"with trailing", k_with_trailing_input, k_with_trailing_expected, k_with_trailing_should_succeed},

          // Invalid cases
          {"invalid - only operator", k_invalid_only_operator_input, k_invalid_only_operator_expected,
           k_invalid_only_operator_should_succeed},
          {"invalid - empty", k_invalid_empty_input, k_invalid_empty_expected, k_invalid_empty_should_succeed},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}
