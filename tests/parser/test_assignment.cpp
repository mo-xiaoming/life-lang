#include "internal_rules.hpp"
#include "utils.hpp"
using life_lang::ast::Expr;

// PARSE_TEST generates check_parse() function and Expr_Params type
PARSE_TEST(Expr, expr)

namespace {
// === Valid Assignment Expressions ===

// Simple variable assignment
constexpr auto k_simple_assignment_should_succeed = true;
constexpr auto k_simple_assignment_input = "x = 42";
inline auto const k_simple_assignment_expected =
    test_sexp::assignment_expr(test_sexp::var_name("x"), test_sexp::integer(42));

// Assignment with expression
constexpr auto k_assignment_with_expr_should_succeed = true;
constexpr auto k_assignment_with_expr_input = "x = y + 10";
inline auto const k_assignment_with_expr_expected = test_sexp::assignment_expr(
    test_sexp::var_name("x"),
    test_sexp::binary_expr("+", test_sexp::var_name("y"), test_sexp::integer(10))
);

// Field access assignment
constexpr auto k_field_assignment_should_succeed = true;
constexpr auto k_field_assignment_input = "point.x = 5";
inline auto const k_field_assignment_expected =
    test_sexp::assignment_expr(test_sexp::field_access(test_sexp::var_name("point"), "x"), test_sexp::integer(5));

// Nested field assignment
constexpr auto k_nested_field_assignment_should_succeed = true;
constexpr auto k_nested_field_assignment_input = "obj.data.count = 100";
inline auto const k_nested_field_assignment_expected = test_sexp::assignment_expr(
    test_sexp::field_access(test_sexp::field_access(test_sexp::var_name("obj"), "data"), "count"),
    test_sexp::integer(100)
);

// Right-associative chained assignment (x = y = z parses as x = (y = z))
constexpr auto k_chained_assignment_should_succeed = true;
constexpr auto k_chained_assignment_input = "x = y = 42";
inline auto const k_chained_assignment_expected = test_sexp::assignment_expr(
    test_sexp::var_name("x"),
    test_sexp::assignment_expr(test_sexp::var_name("y"), test_sexp::integer(42))
);

// Assignment with function call
constexpr auto k_assignment_with_call_should_succeed = true;
constexpr auto k_assignment_with_call_input = "result = calculate()";
inline auto const k_assignment_with_call_expected = test_sexp::assignment_expr(
    test_sexp::var_name("result"),
    test_sexp::function_call(test_sexp::var_name("calculate"), {})
);

// Assignment with string
constexpr auto k_assignment_with_string_should_succeed = true;
constexpr auto k_assignment_with_string_input = R"(name = "Alice")";
inline auto const k_assignment_with_string_expected =
    test_sexp::assignment_expr(test_sexp::var_name("name"), test_sexp::string(R"("Alice")"));

// Assignment precedence: x = y + z (should parse as x = (y + z))
constexpr auto k_assignment_precedence_should_succeed = true;
constexpr auto k_assignment_precedence_input = "x = y + z";
inline auto const k_assignment_precedence_expected = test_sexp::assignment_expr(
    test_sexp::var_name("x"),
    test_sexp::binary_expr("+", test_sexp::var_name("y"), test_sexp::var_name("z"))
);

// Assignment with comparison (x = y < z should parse as x = (y < z))
constexpr auto k_assignment_with_comparison_should_succeed = true;
constexpr auto k_assignment_with_comparison_input = "flag = x < 10";
inline auto const k_assignment_with_comparison_expected = test_sexp::assignment_expr(
    test_sexp::var_name("flag"),
    test_sexp::binary_expr("<", test_sexp::var_name("x"), test_sexp::integer(10))
);

// Assignment self-reference (count = count + 1)
constexpr auto k_assignment_self_reference_should_succeed = true;
constexpr auto k_assignment_self_reference_input = "count = count + 1";
inline auto const k_assignment_self_reference_expected = test_sexp::assignment_expr(
    test_sexp::var_name("count"),
    test_sexp::binary_expr("+", test_sexp::var_name("count"), test_sexp::integer(1))
);

}  // namespace

TEST_CASE("Parse Assignment_Expr") {
  std::vector<Expr_Params> const params_list = {
      {.name = "simple assignment",
       .input = k_simple_assignment_input,
       .expected = k_simple_assignment_expected,
       .should_succeed = k_simple_assignment_should_succeed},
      {.name = "assignment with expr",
       .input = k_assignment_with_expr_input,
       .expected = k_assignment_with_expr_expected,
       .should_succeed = k_assignment_with_expr_should_succeed},
      {.name = "field assignment",
       .input = k_field_assignment_input,
       .expected = k_field_assignment_expected,
       .should_succeed = k_field_assignment_should_succeed},
      {.name = "nested field assignment",
       .input = k_nested_field_assignment_input,
       .expected = k_nested_field_assignment_expected,
       .should_succeed = k_nested_field_assignment_should_succeed},
      {.name = "chained assignment",
       .input = k_chained_assignment_input,
       .expected = k_chained_assignment_expected,
       .should_succeed = k_chained_assignment_should_succeed},
      {.name = "assignment with call",
       .input = k_assignment_with_call_input,
       .expected = k_assignment_with_call_expected,
       .should_succeed = k_assignment_with_call_should_succeed},
      {.name = "assignment with string",
       .input = k_assignment_with_string_input,
       .expected = k_assignment_with_string_expected,
       .should_succeed = k_assignment_with_string_should_succeed},
      {.name = "assignment precedence",
       .input = k_assignment_precedence_input,
       .expected = k_assignment_precedence_expected,
       .should_succeed = k_assignment_precedence_should_succeed},
      {.name = "assignment with comparison",
       .input = k_assignment_with_comparison_input,
       .expected = k_assignment_with_comparison_expected,
       .should_succeed = k_assignment_with_comparison_should_succeed},
      {.name = "assignment self-reference",
       .input = k_assignment_self_reference_input,
       .expected = k_assignment_self_reference_expected,
       .should_succeed = k_assignment_self_reference_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
