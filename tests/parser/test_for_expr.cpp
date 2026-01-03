#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;
using namespace test_sexp;

PARSE_TEST(Expr, expr)

namespace {
// === Valid For Loop Expressions ===

// Simple range iteration (exclusive)
constexpr auto k_simple_range_exclusive_should_succeed = true;
constexpr auto k_simple_range_exclusive_input = "for i in 0..10 { process(i); }";
inline auto const k_simple_range_exclusive_expected = for_expr(
    simple_pattern("i"),
    range_expr(integer("0"), integer("10"), false),
    block({function_call_statement(function_call(var_name("process"), {var_name("i")}))})
);

// Simple range iteration (inclusive)
constexpr auto k_simple_range_inclusive_should_succeed = true;
constexpr auto k_simple_range_inclusive_input = "for i in 0..=10 { process(i); }";
inline auto const k_simple_range_inclusive_expected = for_expr(
    simple_pattern("i"),
    range_expr(integer("0"), integer("10"), true),
    block({function_call_statement(function_call(var_name("process"), {var_name("i")}))})
);

// Variable range
constexpr auto k_var_range_should_succeed = true;
constexpr auto k_var_range_input = "for item in start..end { work(item); }";
inline auto const k_var_range_expected = for_expr(
    simple_pattern("item"),
    range_expr(var_name("start"), var_name("end"), false),
    block({function_call_statement(function_call(var_name("work"), {var_name("item")}))})
);

// Collection variable iteration
constexpr auto k_collection_iteration_should_succeed = true;
constexpr auto k_collection_iteration_input = "for user in users { handle(user); }";
inline auto const k_collection_iteration_expected = for_expr(
    simple_pattern("user"),
    var_name("users"),
    block({function_call_statement(function_call(var_name("handle"), {var_name("user")}))})
);

// Empty body
constexpr auto k_empty_body_should_succeed = true;
constexpr auto k_empty_body_input = "for x in 0..10 {}";
inline auto const k_empty_body_expected =
    for_expr(simple_pattern("x"), range_expr(integer("0"), integer("10"), false), block({}));

// Nested for loops
constexpr auto k_nested_for_loops_should_succeed = true;
constexpr auto k_nested_for_loops_input = "for i in 0..3 { for j in 0..3 { process(i, j); } }";
inline auto const k_nested_for_loops_expected = for_expr(
    simple_pattern("i"),
    range_expr(integer("0"), integer("3"), false),
    block({for_statement(for_expr(
        simple_pattern("j"),
        range_expr(integer("0"), integer("3"), false),
        block({function_call_statement(function_call(var_name("process"), {var_name("i"), var_name("j")}))})
    ))})
);

// For loop with multiple statements
constexpr auto k_multiple_statements_should_succeed = true;
constexpr auto k_multiple_statements_input = "for x in 0..5 { print(x); log(x); }";
inline auto const k_multiple_statements_expected = for_expr(
    simple_pattern("x"),
    range_expr(integer("0"), integer("5"), false),
    block(
        {function_call_statement(function_call(var_name("print"), {var_name("x")})),
         function_call_statement(function_call(var_name("log"), {var_name("x")}))}
    )
);

// For with function call as iterator
constexpr auto k_func_call_iterator_should_succeed = true;
constexpr auto k_func_call_iterator_input = "for item in get_items() { process(item); }";
inline auto const k_func_call_iterator_expected = for_expr(
    simple_pattern("item"),
    function_call(var_name("get_items"), {}),
    block({function_call_statement(function_call(var_name("process"), {var_name("item")}))})
);
// For with unbounded range - tests that block is not consumed as range endpoint
// This verifies that `for i in ..{ }` parses as unbounded range followed by loop body
constexpr auto k_unbounded_range_should_succeed = true;
constexpr auto k_unbounded_range_input = "for i in .. { process(i); }";
inline auto const k_unbounded_range_expected = for_expr(
    simple_pattern("i"),
    range_expr("nil", "nil", false),  // Unbounded range
    block({function_call_statement(function_call(var_name("process"), {var_name("i")}))})
);

// For with unbounded start range
constexpr auto k_unbounded_start_should_succeed = true;
constexpr auto k_unbounded_start_input = "for i in ..10 { process(i); }";
inline auto const k_unbounded_start_expected = for_expr(
    simple_pattern("i"),
    range_expr("nil", integer("10"), false),
    block({function_call_statement(function_call(var_name("process"), {var_name("i")}))})
);

// For with block expression as range endpoint (requires parentheses)
constexpr auto k_block_endpoint_should_succeed = true;
constexpr auto k_block_endpoint_input = "for i in 0..({ get_max() }) { process(i); }";
inline auto const k_block_endpoint_expected = for_expr(
    simple_pattern("i"),
    range_expr(integer("0"), block({}, function_call(var_name("get_max"), {})), false),
    block({function_call_statement(function_call(var_name("process"), {var_name("i")}))})
);

// For with spaces
constexpr auto k_with_spaces_should_succeed = true;
constexpr auto k_with_spaces_input = "for   x   in   0..10   {   work();   }";
inline auto const k_with_spaces_expected = for_expr(
    simple_pattern("x"),
    range_expr(integer("0"), integer("10"), false),
    block({function_call_statement(function_call(var_name("work"), {}))})
);

// === Invalid For Loop Expressions ===

// Missing 'in' keyword
constexpr auto k_missing_in_should_succeed = false;
constexpr auto k_missing_in_input = "for x 0..10 { work(); }";

// Missing binding variable
constexpr auto k_missing_binding_should_succeed = false;
constexpr auto k_missing_binding_input = "for in 0..10 { work(); }";

// Missing iterator expression
constexpr auto k_missing_iterator_should_succeed = false;
constexpr auto k_missing_iterator_input = "for x in { work(); }";

// Missing body
constexpr auto k_missing_body_should_succeed = false;
constexpr auto k_missing_body_input = "for x in 0..10";

// Missing opening brace
constexpr auto k_missing_open_brace_should_succeed = false;
constexpr auto k_missing_open_brace_input = "for x in 0..10 work(); }";

// Reserved keyword as binding
constexpr auto k_reserved_keyword_binding_should_succeed = false;
constexpr auto k_reserved_keyword_binding_input = "for if in 0..10 { work(); }";

}  // namespace

TEST_CASE("Parse For_Expr") {
  std::vector<Expr_Params> const params_list = {
      // Valid cases
      {.name = "simple range exclusive",
       .input = k_simple_range_exclusive_input,
       .expected = k_simple_range_exclusive_expected,
       .should_succeed = k_simple_range_exclusive_should_succeed},
      {.name = "simple range inclusive",
       .input = k_simple_range_inclusive_input,
       .expected = k_simple_range_inclusive_expected,
       .should_succeed = k_simple_range_inclusive_should_succeed},
      {.name = "variable range",
       .input = k_var_range_input,
       .expected = k_var_range_expected,
       .should_succeed = k_var_range_should_succeed},
      {.name = "collection iteration",
       .input = k_collection_iteration_input,
       .expected = k_collection_iteration_expected,
       .should_succeed = k_collection_iteration_should_succeed},
      {.name = "empty body",
       .input = k_empty_body_input,
       .expected = k_empty_body_expected,
       .should_succeed = k_empty_body_should_succeed},
      {.name = "nested for loops",
       .input = k_nested_for_loops_input,
       .expected = k_nested_for_loops_expected,
       .should_succeed = k_nested_for_loops_should_succeed},
      {.name = "multiple statements",
       .input = k_multiple_statements_input,
       .expected = k_multiple_statements_expected,
       .should_succeed = k_multiple_statements_should_succeed},
      {.name = "function call iterator",
       .input = k_func_call_iterator_input,
       .expected = k_func_call_iterator_expected,
       .should_succeed = k_func_call_iterator_should_succeed},
      {.name = "unbounded range",
       .input = k_unbounded_range_input,
       .expected = k_unbounded_range_expected,
       .should_succeed = k_unbounded_range_should_succeed},
      {.name = "unbounded start range",
       .input = k_unbounded_start_input,
       .expected = k_unbounded_start_expected,
       .should_succeed = k_unbounded_start_should_succeed},
      {.name = "block endpoint with parens",
       .input = k_block_endpoint_input,
       .expected = k_block_endpoint_expected,
       .should_succeed = k_block_endpoint_should_succeed},
      {.name = "with spaces",
       .input = k_with_spaces_input,
       .expected = k_with_spaces_expected,
       .should_succeed = k_with_spaces_should_succeed},

      // Invalid cases
      {.name = "invalid - missing in",
       .input = k_missing_in_input,
       .expected = "",
       .should_succeed = k_missing_in_should_succeed},
      {.name = "invalid - missing binding",
       .input = k_missing_binding_input,
       .expected = "",
       .should_succeed = k_missing_binding_should_succeed},
      {.name = "invalid - missing iterator",
       .input = k_missing_iterator_input,
       .expected = "",
       .should_succeed = k_missing_iterator_should_succeed},
      {.name = "invalid - missing body",
       .input = k_missing_body_input,
       .expected = "",
       .should_succeed = k_missing_body_should_succeed},
      {.name = "invalid - missing open brace",
       .input = k_missing_open_brace_input,
       .expected = "",
       .should_succeed = k_missing_open_brace_should_succeed},
      {.name = "invalid - reserved keyword binding",
       .input = k_reserved_keyword_binding_input,
       .expected = "",
       .should_succeed = k_reserved_keyword_binding_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
