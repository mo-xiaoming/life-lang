#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {

// Single element with trailing comma (required for single-element tuples)
constexpr auto k_single_element_trailing_comma_should_succeed = true;
constexpr auto k_single_element_trailing_comma_input = "(42,)";
inline auto const k_single_element_trailing_comma_expected = R"((tuple_lit ((integer "42"))))";

// Two elements
constexpr auto k_two_elements_should_succeed = true;
constexpr auto k_two_elements_input = "(1, 2)";
inline auto const k_two_elements_expected = R"((tuple_lit ((integer "1") (integer "2"))))";

// Three elements
constexpr auto k_three_elements_should_succeed = true;
constexpr auto k_three_elements_input = "(1, 2, 3)";
inline auto const k_three_elements_expected = R"((tuple_lit ((integer "1") (integer "2") (integer "3"))))";

// Multiple elements with trailing comma
constexpr auto k_multiple_trailing_comma_should_succeed = true;
constexpr auto k_multiple_trailing_comma_input = "(1, 2, 3,)";
inline auto const k_multiple_trailing_comma_expected = R"((tuple_lit ((integer "1") (integer "2") (integer "3"))))";

// Mixed types (parser accepts, semantic analysis checks later)
constexpr auto k_mixed_types_should_succeed = true;
constexpr auto k_mixed_types_input = R"((42, "hello", true))";
inline std::string const k_mixed_types_expected =
    std::format(R"((tuple_lit ((integer "42") (string "\"hello\"") {})))", test_sexp::var_name("true"));

// Tuple with variables
constexpr auto k_with_variables_should_succeed = true;
constexpr auto k_with_variables_input = "(x, y, z)";
inline std::string const k_with_variables_expected = std::format(
    R"((tuple_lit ({} {} {})))",
    test_sexp::var_name("x"),
    test_sexp::var_name("y"),
    test_sexp::var_name("z")
);

// Tuple with expressions
constexpr auto k_with_expressions_should_succeed = true;
constexpr auto k_with_expressions_input = "(1 + 2, x * 3)";
inline std::string const k_with_expressions_expected = std::format(
    R"((tuple_lit ((binary + (integer "1") (integer "2")) (binary * {} (integer "3")))))",
    test_sexp::var_name("x")
);

// Nested tuples
constexpr auto k_nested_tuples_should_succeed = true;
constexpr auto k_nested_tuples_input = "((1, 2), (3, 4))";
inline auto const k_nested_tuples_expected =
    R"((tuple_lit ((tuple_lit ((integer "1") (integer "2"))) (tuple_lit ((integer "3") (integer "4"))))))";

// Tuple with function calls
constexpr auto k_with_function_calls_should_succeed = true;
constexpr auto k_with_function_calls_input = "(foo(), bar(x))";
inline std::string const k_with_function_calls_expected = std::format(
    R"((tuple_lit ((call {} ()) (call {} ({})))))",
    test_sexp::var_name("foo"),
    test_sexp::var_name("bar"),
    test_sexp::var_name("x")
);

// Tuple with struct literals
constexpr auto k_with_struct_literals_should_succeed = true;
constexpr auto k_with_struct_literals_input = "(Point { x: 1, y: 2 }, Point { x: 3, y: 4 })";
inline auto const k_with_struct_literals_expected =
    R"((tuple_lit ((struct_lit "Point" ((field_init "x" (integer "1")) (field_init "y" (integer "2")))) (struct_lit "Point" ((field_init "x" (integer "3")) (field_init "y" (integer "4")))))))";

// Tuple with array literals
constexpr auto k_with_array_literals_should_succeed = true;
constexpr auto k_with_array_literals_input = "([1, 2], [3, 4])";
inline auto const k_with_array_literals_expected =
    R"((tuple_lit ((array_lit ((integer "1") (integer "2"))) (array_lit ((integer "3") (integer "4"))))))";

// Large tuple (5 elements)
constexpr auto k_large_tuple_should_succeed = true;
constexpr auto k_large_tuple_input = "(1, 2, 3, 4, 5)";
inline auto const k_large_tuple_expected =
    R"((tuple_lit ((integer "1") (integer "2") (integer "3") (integer "4") (integer "5"))))";

// Tuple with string literals
constexpr auto k_with_strings_should_succeed = true;
constexpr auto k_with_strings_input = R"(("name", "age", "city"))";
inline auto const k_with_strings_expected =
    R"((tuple_lit ((string "\"name\"") (string "\"age\"") (string "\"city\""))))";

// === Parenthesized Expression Tests (NOT tuples) ===

// Single element without trailing comma - parenthesized expression
constexpr auto k_parenthesized_expr_should_succeed = true;
constexpr auto k_parenthesized_expr_input = "(42)";
inline auto const k_parenthesized_expr_expected = R"((integer "42"))";  // Just the expression, no tuple wrapper

// Complex parenthesized expression
constexpr auto k_complex_parenthesized_should_succeed = true;
constexpr auto k_complex_parenthesized_input = "((1 + 2) * 3)";
inline auto const k_complex_parenthesized_expected =
    R"((binary * (binary + (integer "1") (integer "2")) (integer "3")))";

// === Invalid Tuple Literals ===

// Missing closing paren
constexpr auto k_missing_closing_paren_should_succeed = false;
constexpr auto k_missing_closing_paren_input = "(1, 2";

// Missing opening paren
constexpr auto k_missing_opening_paren_should_succeed = false;
constexpr auto k_missing_opening_paren_input = "1, 2)";

// Double comma
constexpr auto k_double_comma_should_succeed = false;
constexpr auto k_double_comma_input = "(1,, 2)";

// Leading comma
constexpr auto k_leading_comma_should_succeed = false;
constexpr auto k_leading_comma_input = "(, 1, 2)";

// Empty tuple (should parse as unit literal, not tuple)
constexpr auto k_empty_tuple_should_succeed = true;
constexpr auto k_empty_tuple_input = "()";
inline auto const k_empty_tuple_expected = "unit";  // Unit literal, not tuple

}  // namespace

TEST_CASE("Parse Tuple_Literal") {
  std::vector<Expr_Params> const params_list = {
      {.name = "single element with trailing comma",
       .input = k_single_element_trailing_comma_input,
       .expected = k_single_element_trailing_comma_expected,
       .should_succeed = k_single_element_trailing_comma_should_succeed},
      {.name = "two elements",
       .input = k_two_elements_input,
       .expected = k_two_elements_expected,
       .should_succeed = k_two_elements_should_succeed},
      {.name = "three elements",
       .input = k_three_elements_input,
       .expected = k_three_elements_expected,
       .should_succeed = k_three_elements_should_succeed},
      {.name = "multiple with trailing comma",
       .input = k_multiple_trailing_comma_input,
       .expected = k_multiple_trailing_comma_expected,
       .should_succeed = k_multiple_trailing_comma_should_succeed},
      {.name = "mixed types",
       .input = k_mixed_types_input,
       .expected = k_mixed_types_expected,
       .should_succeed = k_mixed_types_should_succeed},
      {.name = "with variables",
       .input = k_with_variables_input,
       .expected = k_with_variables_expected,
       .should_succeed = k_with_variables_should_succeed},
      {.name = "with expressions",
       .input = k_with_expressions_input,
       .expected = k_with_expressions_expected,
       .should_succeed = k_with_expressions_should_succeed},
      {.name = "nested tuples",
       .input = k_nested_tuples_input,
       .expected = k_nested_tuples_expected,
       .should_succeed = k_nested_tuples_should_succeed},
      {.name = "with function calls",
       .input = k_with_function_calls_input,
       .expected = k_with_function_calls_expected,
       .should_succeed = k_with_function_calls_should_succeed},
      {.name = "with struct literals",
       .input = k_with_struct_literals_input,
       .expected = k_with_struct_literals_expected,
       .should_succeed = k_with_struct_literals_should_succeed},
      {.name = "with array literals",
       .input = k_with_array_literals_input,
       .expected = k_with_array_literals_expected,
       .should_succeed = k_with_array_literals_should_succeed},
      {.name = "large tuple",
       .input = k_large_tuple_input,
       .expected = k_large_tuple_expected,
       .should_succeed = k_large_tuple_should_succeed},
      {.name = "with strings",
       .input = k_with_strings_input,
       .expected = k_with_strings_expected,
       .should_succeed = k_with_strings_should_succeed},
      // Parenthesized expressions (NOT tuples)
      {.name = "parenthesized expression",
       .input = k_parenthesized_expr_input,
       .expected = k_parenthesized_expr_expected,
       .should_succeed = k_parenthesized_expr_should_succeed},
      {.name = "complex parenthesized",
       .input = k_complex_parenthesized_input,
       .expected = k_complex_parenthesized_expected,
       .should_succeed = k_complex_parenthesized_should_succeed},
      // Invalid cases
      {.name = "missing closing paren",
       .input = k_missing_closing_paren_input,
       .expected = "",
       .should_succeed = k_missing_closing_paren_should_succeed},
      {.name = "missing opening paren",
       .input = k_missing_opening_paren_input,
       .expected = "",
       .should_succeed = k_missing_opening_paren_should_succeed},
      {.name = "double comma",
       .input = k_double_comma_input,
       .expected = "",
       .should_succeed = k_double_comma_should_succeed},
      {.name = "leading comma",
       .input = k_leading_comma_input,
       .expected = "",
       .should_succeed = k_leading_comma_should_succeed},
      {.name = "empty tuple (unit literal)",
       .input = k_empty_tuple_input,
       .expected = k_empty_tuple_expected,
       .should_succeed = k_empty_tuple_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
