#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// === Valid Tuple Pattern Tests ===

// Two element tuple pattern
constexpr auto k_tuple_two_elements_should_succeed = true;
constexpr auto k_tuple_two_elements_input = "for (a, b) in pairs { process(a, b); }";
inline auto const k_tuple_two_elements_expected = test_sexp::for_expr(
    test_sexp::tuple_pattern({test_sexp::simple_pattern("a"), test_sexp::simple_pattern("b")}),
    test_sexp::var_name("pairs"),
    test_sexp::block({test_sexp::function_call_statement(
        test_sexp::function_call(test_sexp::var_name("process"), {test_sexp::var_name("a"), test_sexp::var_name("b")})
    )})
);

// Three element tuple pattern
constexpr auto k_tuple_three_elements_should_succeed = true;
constexpr auto k_tuple_three_elements_input = "for (x, y, z) in coords { move(x, y, z); }";
inline auto const k_tuple_three_elements_expected = test_sexp::for_expr(
    test_sexp::tuple_pattern(
        {test_sexp::simple_pattern("x"), test_sexp::simple_pattern("y"), test_sexp::simple_pattern("z")}
    ),
    test_sexp::var_name("coords"),
    test_sexp::block({test_sexp::function_call_statement(
        test_sexp::function_call(
            test_sexp::var_name("move"),
            {test_sexp::var_name("x"), test_sexp::var_name("y"), test_sexp::var_name("z")}
        )
    )})
);

// Single element tuple (edge case)
constexpr auto k_tuple_single_element_should_succeed = true;
constexpr auto k_tuple_single_element_input = "for (item) in items { process(item); }";
inline auto const k_tuple_single_element_expected = test_sexp::for_expr(
    test_sexp::tuple_pattern({test_sexp::simple_pattern("item")}),
    test_sexp::var_name("items"),
    test_sexp::block({test_sexp::function_call_statement(
        test_sexp::function_call(test_sexp::var_name("process"), {test_sexp::var_name("item")})
    )})
);

// Nested tuple in tuple: for ((a, b), c) in ...
constexpr auto k_nested_tuple_in_tuple_should_succeed = true;
constexpr auto k_nested_tuple_in_tuple_input = "for ((a, b), c) in nested { print(a, b, c); }";
inline auto const k_nested_tuple_in_tuple_expected = test_sexp::for_expr(
    test_sexp::tuple_pattern(
        {test_sexp::tuple_pattern({test_sexp::simple_pattern("a"), test_sexp::simple_pattern("b")}),
         test_sexp::simple_pattern("c")}
    ),
    test_sexp::var_name("nested"),
    test_sexp::block({test_sexp::function_call_statement(
        test_sexp::function_call(
            test_sexp::var_name("print"),
            {test_sexp::var_name("a"), test_sexp::var_name("b"), test_sexp::var_name("c")}
        )
    )})
);

// Nested struct in tuple: for (Point { x: px, y: py }, z) in ...
constexpr auto k_nested_struct_in_tuple_should_succeed = true;
constexpr auto k_nested_struct_in_tuple_input = "for (Point { x: px, y: py }, z) in data { use(px, py, z); }";
inline auto const k_nested_struct_in_tuple_expected = test_sexp::for_expr(
    test_sexp::tuple_pattern(
        {test_sexp::struct_pattern(
             test_sexp::type_name("Point"),
             {test_sexp::field_pattern("x", test_sexp::simple_pattern("px")),
              test_sexp::field_pattern("y", test_sexp::simple_pattern("py"))}
         ),
         test_sexp::simple_pattern("z")}
    ),
    test_sexp::var_name("data"),
    test_sexp::block({test_sexp::function_call_statement(
        test_sexp::function_call(
            test_sexp::var_name("use"),
            {test_sexp::var_name("px"), test_sexp::var_name("py"), test_sexp::var_name("z")}
        )
    )})
);

// === Valid Tuple Pattern Tests (continued) ===

// Unit pattern (empty tuple) - () matches the unit value
constexpr auto k_unit_pattern_should_succeed = true;
constexpr auto k_unit_pattern_input = "for () in items { process(); }";
inline auto const k_unit_pattern_expected = test_sexp::for_expr(
    test_sexp::tuple_pattern({}),  // Empty tuple = unit pattern
    test_sexp::var_name("items"),
    test_sexp::block({test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name("process"), {}))})
);

// === Invalid Tuple Pattern Tests ===

// === Invalid Tuple Pattern Tests ===

// Missing closing paren
constexpr auto k_missing_closing_paren_should_succeed = false;
constexpr auto k_missing_closing_paren_input = "for (a, b in items { }";

// Missing opening paren
constexpr auto k_missing_opening_paren_should_succeed = false;
constexpr auto k_missing_opening_paren_input = "for a, b) in items { }";

// Trailing comma
constexpr auto k_trailing_comma_should_succeed = false;
constexpr auto k_trailing_comma_input = "for (a, b,) in items { }";

// Leading comma
constexpr auto k_leading_comma_should_succeed = false;
constexpr auto k_leading_comma_input = "for (,a, b) in items { }";

// Double comma
constexpr auto k_double_comma_should_succeed = false;
constexpr auto k_double_comma_input = "for (a,, b) in items { }";

// Invalid identifier (starts with number)
constexpr auto k_invalid_identifier_should_succeed = false;
constexpr auto k_invalid_identifier_input = "for (1a, b) in items { }";

}  // namespace

TEST_CASE("Parse Tuple_Pattern in For_Expr") {
  std::vector<Expr_Params> const params_list = {
      {.name = "two element tuple",
       .input = k_tuple_two_elements_input,
       .expected = k_tuple_two_elements_expected,
       .should_succeed = k_tuple_two_elements_should_succeed},
      {.name = "three element tuple",
       .input = k_tuple_three_elements_input,
       .expected = k_tuple_three_elements_expected,
       .should_succeed = k_tuple_three_elements_should_succeed},
      {.name = "single element tuple",
       .input = k_tuple_single_element_input,
       .expected = k_tuple_single_element_expected,
       .should_succeed = k_tuple_single_element_should_succeed},
      {.name = "nested tuple in tuple",
       .input = k_nested_tuple_in_tuple_input,
       .expected = k_nested_tuple_in_tuple_expected,
       .should_succeed = k_nested_tuple_in_tuple_should_succeed},
      {.name = "nested struct in tuple",
       .input = k_nested_struct_in_tuple_input,
       .expected = k_nested_struct_in_tuple_expected,
       .should_succeed = k_nested_struct_in_tuple_should_succeed},
      {.name = "unit pattern (empty tuple)",
       .input = k_unit_pattern_input,
       .expected = k_unit_pattern_expected,
       .should_succeed = k_unit_pattern_should_succeed},
      {.name = "missing closing paren",
       .input = k_missing_closing_paren_input,
       .expected = "",
       .should_succeed = k_missing_closing_paren_should_succeed},
      {.name = "missing opening paren",
       .input = k_missing_opening_paren_input,
       .expected = "",
       .should_succeed = k_missing_opening_paren_should_succeed},
      {.name = "trailing comma",
       .input = k_trailing_comma_input,
       .expected = "",
       .should_succeed = k_trailing_comma_should_succeed},
      {.name = "leading comma",
       .input = k_leading_comma_input,
       .expected = "",
       .should_succeed = k_leading_comma_should_succeed},
      {.name = "double comma",
       .input = k_double_comma_input,
       .expected = "",
       .should_succeed = k_double_comma_should_succeed},
      {.name = "invalid identifier",
       .input = k_invalid_identifier_input,
       .expected = "",
       .should_succeed = k_invalid_identifier_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
