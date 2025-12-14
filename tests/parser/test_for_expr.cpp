#include "internal_rules.hpp"
#include "utils.hpp"
using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// === Valid For Loop Expressions ===

// Simple range iteration (exclusive)
constexpr auto k_simple_range_exclusive_should_succeed = true;
constexpr auto k_simple_range_exclusive_input = "for i in 0..10 { process(i); }";
inline auto const k_simple_range_exclusive_expected = test_sexp::for_expr(
    test_sexp::simple_pattern("i"),
    test_sexp::range_expr(test_sexp::integer(0), test_sexp::integer(10), false),
    test_sexp::block({test_sexp::function_call_statement(
        test_sexp::function_call(test_sexp::var_name("process"), {test_sexp::var_name("i")})
    )})
);

// Simple range iteration (inclusive)
constexpr auto k_simple_range_inclusive_should_succeed = true;
constexpr auto k_simple_range_inclusive_input = "for i in 0..=10 { process(i); }";
inline auto const k_simple_range_inclusive_expected = test_sexp::for_expr(
    test_sexp::simple_pattern("i"),
    test_sexp::range_expr(test_sexp::integer(0), test_sexp::integer(10), true),
    test_sexp::block({test_sexp::function_call_statement(
        test_sexp::function_call(test_sexp::var_name("process"), {test_sexp::var_name("i")})
    )})
);

// Variable range
constexpr auto k_var_range_should_succeed = true;
constexpr auto k_var_range_input = "for item in start..end { work(item); }";
inline auto const k_var_range_expected = test_sexp::for_expr(
    test_sexp::simple_pattern("item"),
    test_sexp::range_expr(test_sexp::var_name("start"), test_sexp::var_name("end"), false),
    test_sexp::block({test_sexp::function_call_statement(
        test_sexp::function_call(test_sexp::var_name("work"), {test_sexp::var_name("item")})
    )})
);

// Collection variable iteration
constexpr auto k_collection_iteration_should_succeed = true;
constexpr auto k_collection_iteration_input = "for user in users { handle(user); }";
inline auto const k_collection_iteration_expected = test_sexp::for_expr(
    test_sexp::simple_pattern("user"),
    test_sexp::var_name("users"),
    test_sexp::block({test_sexp::function_call_statement(
        test_sexp::function_call(test_sexp::var_name("handle"), {test_sexp::var_name("user")})
    )})
);

// Empty body
constexpr auto k_empty_body_should_succeed = true;
constexpr auto k_empty_body_input = "for x in 0..10 {}";
inline auto const k_empty_body_expected = test_sexp::for_expr(
    test_sexp::simple_pattern("x"),
    test_sexp::range_expr(test_sexp::integer(0), test_sexp::integer(10), false),
    test_sexp::block({})
);

// Nested for loops
constexpr auto k_nested_for_loops_should_succeed = true;
constexpr auto k_nested_for_loops_input = "for i in 0..3 { for j in 0..3 { process(i, j); } }";
inline auto const k_nested_for_loops_expected = test_sexp::for_expr(
    test_sexp::simple_pattern("i"),
    test_sexp::range_expr(test_sexp::integer(0), test_sexp::integer(3), false),
    test_sexp::block({test_sexp::for_statement(
        test_sexp::for_expr(
            test_sexp::simple_pattern("j"),
            test_sexp::range_expr(test_sexp::integer(0), test_sexp::integer(3), false),
            test_sexp::block({test_sexp::function_call_statement(
                test_sexp::function_call(
                    test_sexp::var_name("process"),
                    {test_sexp::var_name("i"), test_sexp::var_name("j")}
                )
            )})
        )
    )})
);

// For loop with multiple statements
constexpr auto k_multiple_statements_should_succeed = true;
constexpr auto k_multiple_statements_input = "for x in 0..5 { print(x); log(x); }";
inline auto const k_multiple_statements_expected = test_sexp::for_expr(
    test_sexp::simple_pattern("x"),
    test_sexp::range_expr(test_sexp::integer(0), test_sexp::integer(5), false),
    test_sexp::block(
        {test_sexp::function_call_statement(
             test_sexp::function_call(test_sexp::var_name("print"), {test_sexp::var_name("x")})
         ),
         test_sexp::function_call_statement(
             test_sexp::function_call(test_sexp::var_name("log"), {test_sexp::var_name("x")})
         )}
    )
);

// For with function call as iterator
constexpr auto k_func_call_iterator_should_succeed = true;
constexpr auto k_func_call_iterator_input = "for item in get_items() { process(item); }";
inline auto const k_func_call_iterator_expected = test_sexp::for_expr(
    test_sexp::simple_pattern("item"),
    test_sexp::function_call(test_sexp::var_name("get_items"), {}),
    test_sexp::block({test_sexp::function_call_statement(
        test_sexp::function_call(test_sexp::var_name("process"), {test_sexp::var_name("item")})
    )})
);

// For with spaces
constexpr auto k_with_spaces_should_succeed = true;
constexpr auto k_with_spaces_input = "for   x   in   0..10   {   work();   }";
inline auto const k_with_spaces_expected = test_sexp::for_expr(
    test_sexp::simple_pattern("x"),
    test_sexp::range_expr(test_sexp::integer(0), test_sexp::integer(10), false),
    test_sexp::block({test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name("work"), {}))})
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
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
