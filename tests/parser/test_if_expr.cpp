#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;
using namespace test_sexp;

PARSE_TEST(Expr, expr)

namespace {
// Basic if without else
constexpr auto k_if_only_should_succeed = true;
constexpr auto k_if_only_input = "if x { return 1; }";
inline auto const k_if_only_expected = if_expr(var_name("x"), block({return_statement(integer("1"))}));

// If with else
constexpr auto k_if_else_should_succeed = true;
constexpr auto k_if_else_input = "if condition { return 1; } else { return 2; }";
inline auto const k_if_else_expected = if_else_expr(
    var_name("condition"),
    block({return_statement(integer("1"))}),
    block({return_statement(integer("2"))})
);

// If with single else-if
constexpr auto k_if_elseif_should_succeed = true;
constexpr auto k_if_elseif_input = "if a { return 1; } else if b { return 2; }";
inline auto const k_if_elseif_expected = if_with_elseif(
    var_name("a"),
    block({return_statement(integer("1"))}),
    {else_if_clause(var_name("b"), block({return_statement(integer("2"))}))}
);

// If with else-if and final else
constexpr auto k_if_elseif_else_should_succeed = true;
constexpr auto k_if_elseif_else_input = "if a { return 1; } else if b { return 2; } else { return 3; }";
inline auto const k_if_elseif_else_expected = if_with_elseif(
    var_name("a"),
    block({return_statement(integer("1"))}),
    {else_if_clause(var_name("b"), block({return_statement(integer("2"))}))},
    block({return_statement(integer("3"))})
);

// If with multiple else-if clauses
constexpr auto k_multiple_elseif_should_succeed = true;
constexpr auto k_multiple_elseif_input =
    "if a { return 1; } else if b { return 2; } else if c { return 3; } else { return 4; }";
inline auto const k_multiple_elseif_expected = if_with_elseif(
    var_name("a"),
    block({return_statement(integer("1"))}),
    {else_if_clause(var_name("b"), block({return_statement(integer("2"))})),
     else_if_clause(var_name("c"), block({return_statement(integer("3"))}))},
    block({return_statement(integer("4"))})
);

// If expression with binary operators
constexpr auto k_if_with_comparison_should_succeed = true;
constexpr auto k_if_with_comparison_input = "if x > y { return x; } else { return y; }";
inline auto const k_if_with_comparison_expected = if_else_expr(
    binary_expr(">", var_name("x"), var_name("y")),
    block({return_statement(var_name("x"))}),
    block({return_statement(var_name("y"))})
);

// Empty blocks
constexpr auto k_if_empty_blocks_should_succeed = true;
constexpr auto k_if_empty_blocks_input = "if x {} else if y {} else {}";
inline auto const k_if_empty_blocks_expected =
    if_with_elseif(var_name("x"), block({}), {else_if_clause(var_name("y"), block({}))}, block({}));

// Invalid: missing condition
constexpr auto k_missing_condition_should_succeed = false;
constexpr auto k_missing_condition_input = "if { return 1; }";
inline auto const k_missing_condition_expected = "";

// Invalid: missing block
constexpr auto k_missing_block_should_succeed = false;
constexpr auto k_missing_block_input = "if x";
inline auto const k_missing_block_expected = "";

// Invalid: missing else-if condition
constexpr auto k_missing_elseif_condition_should_succeed = false;
constexpr auto k_missing_elseif_condition_input = "if x { return 1; } else if { return 2; }";
inline auto const k_missing_elseif_condition_expected = "";

// Invalid: missing else-if block
constexpr auto k_missing_elseif_block_should_succeed = false;
constexpr auto k_missing_elseif_block_input = "if x { return 1; } else if y";
inline auto const k_missing_elseif_block_expected = "";

}  // namespace

TEST_CASE("Parse If_Expr") {
  std::vector<Expr_Params> const params_list = {
      {.name = "if only",
       .input = k_if_only_input,
       .expected = k_if_only_expected,
       .should_succeed = k_if_only_should_succeed},
      {.name = "if else",
       .input = k_if_else_input,
       .expected = k_if_else_expected,
       .should_succeed = k_if_else_should_succeed},
      {.name = "if else-if",
       .input = k_if_elseif_input,
       .expected = k_if_elseif_expected,
       .should_succeed = k_if_elseif_should_succeed},
      {.name = "if else-if else",
       .input = k_if_elseif_else_input,
       .expected = k_if_elseif_else_expected,
       .should_succeed = k_if_elseif_else_should_succeed},
      {.name = "multiple else-if",
       .input = k_multiple_elseif_input,
       .expected = k_multiple_elseif_expected,
       .should_succeed = k_multiple_elseif_should_succeed},
      {.name = "if with comparison",
       .input = k_if_with_comparison_input,
       .expected = k_if_with_comparison_expected,
       .should_succeed = k_if_with_comparison_should_succeed},
      {.name = "if empty blocks",
       .input = k_if_empty_blocks_input,
       .expected = k_if_empty_blocks_expected,
       .should_succeed = k_if_empty_blocks_should_succeed},
      {.name = "missing condition",
       .input = k_missing_condition_input,
       .expected = k_missing_condition_expected,
       .should_succeed = k_missing_condition_should_succeed},
      {.name = "missing block",
       .input = k_missing_block_input,
       .expected = k_missing_block_expected,
       .should_succeed = k_missing_block_should_succeed},
      {.name = "missing else-if condition",
       .input = k_missing_elseif_condition_input,
       .expected = k_missing_elseif_condition_expected,
       .should_succeed = k_missing_elseif_condition_should_succeed},
      {.name = "missing else-if block",
       .input = k_missing_elseif_block_input,
       .expected = k_missing_elseif_block_expected,
       .should_succeed = k_missing_elseif_block_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
