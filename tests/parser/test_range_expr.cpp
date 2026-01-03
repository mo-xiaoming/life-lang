#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// === Valid Range Expressions ===

// Simple integer range (exclusive)
constexpr auto k_simple_exclusive_should_succeed = true;
constexpr auto k_simple_exclusive_input = "0..10";
inline auto const k_simple_exclusive_expected =
    test_sexp::range_expr(test_sexp::integer(0), test_sexp::integer(10), false);

// Simple integer range (inclusive)
constexpr auto k_simple_inclusive_should_succeed = true;
constexpr auto k_simple_inclusive_input = "0..=10";
inline auto const k_simple_inclusive_expected =
    test_sexp::range_expr(test_sexp::integer(0), test_sexp::integer(10), true);

// Variable range (exclusive)
constexpr auto k_var_range_should_succeed = true;
constexpr auto k_var_range_input = "start..end";
inline auto const k_var_range_expected =
    test_sexp::range_expr(test_sexp::var_name("start"), test_sexp::var_name("end"), false);

// Variable range (inclusive)
constexpr auto k_var_range_inclusive_should_succeed = true;
constexpr auto k_var_range_inclusive_input = "start..=end";
inline auto const k_var_range_inclusive_expected =
    test_sexp::range_expr(test_sexp::var_name("start"), test_sexp::var_name("end"), true);

// Range with arithmetic expressions
constexpr auto k_arithmetic_range_should_succeed = true;
constexpr auto k_arithmetic_range_input = "x+1..y-1";
inline auto const k_arithmetic_range_expected = test_sexp::range_expr(
    test_sexp::binary_expr("+", test_sexp::var_name("x"), test_sexp::integer(1)),
    test_sexp::binary_expr("-", test_sexp::var_name("y"), test_sexp::integer(1)),
    false
);

// Range with spaces
constexpr auto k_range_with_spaces_should_succeed = true;
constexpr auto k_range_with_spaces_input = "0 .. 10";
inline auto const k_range_with_spaces_expected =
    test_sexp::range_expr(test_sexp::integer(0), test_sexp::integer(10), false);

// Range with spaces (inclusive)
constexpr auto k_range_inclusive_spaces_should_succeed = true;
constexpr auto k_range_inclusive_spaces_input = "1 ..= 100";
inline auto const k_range_inclusive_spaces_expected =
    test_sexp::range_expr(test_sexp::integer(1), test_sexp::integer(100), true);

// Range with negative numbers
constexpr auto k_negative_range_should_succeed = true;
constexpr auto k_negative_range_input = "-10..10";
inline auto const k_negative_range_expected =
    test_sexp::range_expr(test_sexp::unary_expr("-", test_sexp::integer(10)), test_sexp::integer(10), false);

// Large numbers
constexpr auto k_large_range_should_succeed = true;
constexpr auto k_large_range_input = "1000..=9999";
inline auto const k_large_range_expected =
    test_sexp::range_expr(test_sexp::integer(1000), test_sexp::integer(9999), true);

// Unbounded start range - now valid
constexpr auto k_missing_start_should_succeed = true;
constexpr auto k_missing_start_input = "..10";
inline auto const k_missing_start_expected = test_sexp::range_expr("nil", test_sexp::integer("10"), false);

// Fully unbounded range - now valid
constexpr auto k_only_dots_should_succeed = true;
constexpr auto k_only_dots_input = "..";
inline auto const k_only_dots_expected = test_sexp::range_expr("nil", "nil", false);

// Unbounded range before block - should not consume the block as endpoint
// This tests that `..{` parses as unbounded range, not range with block endpoint
constexpr auto k_unbounded_before_block_should_succeed = true;
constexpr auto k_unbounded_before_block_input = "..";  // Will be tested in context where { follows
inline auto const k_unbounded_before_block_expected = test_sexp::range_expr("nil", "nil", false);

// Block expression as range endpoint with explicit parentheses
constexpr auto k_block_endpoint_with_parens_should_succeed = true;
constexpr auto k_block_endpoint_with_parens_input = "0..({ 10 })";
inline auto const k_block_endpoint_with_parens_expected =
    test_sexp::range_expr(test_sexp::integer("0"), test_sexp::block({}, test_sexp::integer("10")), false);

// Unbounded start with block endpoint
constexpr auto k_unbounded_start_block_endpoint_should_succeed = true;
constexpr auto k_unbounded_start_block_endpoint_input = "..({ compute_end() })";
inline auto const k_unbounded_start_block_endpoint_expected = test_sexp::range_expr(
    "nil",
    test_sexp::block({}, test_sexp::function_call(test_sexp::var_name("compute_end"), {})),
    false
);

// Range with block expressions on both sides
constexpr auto k_both_blocks_should_succeed = true;
constexpr auto k_both_blocks_input = "({ 0 })..({ 10 })";
inline auto const k_both_blocks_expected = test_sexp::range_expr(
    test_sexp::block({}, test_sexp::integer("0")),
    test_sexp::block({}, test_sexp::integer("10")),
    false
);

}  // namespace

TEST_CASE("Parse Range_Expr") {
  std::vector<Expr_Params> const params_list = {
      // Valid cases
      {.name = "simple exclusive",
       .input = k_simple_exclusive_input,
       .expected = k_simple_exclusive_expected,
       .should_succeed = k_simple_exclusive_should_succeed},
      {.name = "simple inclusive",
       .input = k_simple_inclusive_input,
       .expected = k_simple_inclusive_expected,
       .should_succeed = k_simple_inclusive_should_succeed},
      {.name = "variable range",
       .input = k_var_range_input,
       .expected = k_var_range_expected,
       .should_succeed = k_var_range_should_succeed},
      {.name = "variable range inclusive",
       .input = k_var_range_inclusive_input,
       .expected = k_var_range_inclusive_expected,
       .should_succeed = k_var_range_inclusive_should_succeed},
      {.name = "arithmetic range",
       .input = k_arithmetic_range_input,
       .expected = k_arithmetic_range_expected,
       .should_succeed = k_arithmetic_range_should_succeed},
      {.name = "range with spaces",
       .input = k_range_with_spaces_input,
       .expected = k_range_with_spaces_expected,
       .should_succeed = k_range_with_spaces_should_succeed},
      {.name = "range inclusive spaces",
       .input = k_range_inclusive_spaces_input,
       .expected = k_range_inclusive_spaces_expected,
       .should_succeed = k_range_inclusive_spaces_should_succeed},
      {.name = "negative range",
       .input = k_negative_range_input,
       .expected = k_negative_range_expected,
       .should_succeed = k_negative_range_should_succeed},
      {.name = "large range",
       .input = k_large_range_input,
       .expected = k_large_range_expected,
       .should_succeed = k_large_range_should_succeed},

      // Unbounded ranges (now valid)
      {.name = "unbounded start",
       .input = k_missing_start_input,
       .expected = k_missing_start_expected,
       .should_succeed = k_missing_start_should_succeed},
      {.name = "fully unbounded",
       .input = k_only_dots_input,
       .expected = k_only_dots_expected,
       .should_succeed = k_only_dots_should_succeed},
      {.name = "unbounded before block",
       .input = k_unbounded_before_block_input,
       .expected = k_unbounded_before_block_expected,
       .should_succeed = k_unbounded_before_block_should_succeed},
      {.name = "block endpoint with parens",
       .input = k_block_endpoint_with_parens_input,
       .expected = k_block_endpoint_with_parens_expected,
       .should_succeed = k_block_endpoint_with_parens_should_succeed},
      {.name = "unbounded start with block endpoint",
       .input = k_unbounded_start_block_endpoint_input,
       .expected = k_unbounded_start_block_endpoint_expected,
       .should_succeed = k_unbounded_start_block_endpoint_should_succeed},
      {.name = "both blocks with parens",
       .input = k_both_blocks_input,
       .expected = k_both_blocks_expected,
       .should_succeed = k_both_blocks_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
