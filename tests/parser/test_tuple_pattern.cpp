#include "internal_rules.hpp"
#include "utils.hpp"
using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// === Valid Tuple Pattern Tests ===

// Two element tuple pattern
constexpr auto k_tuple_two_elements_should_succeed = true;
constexpr auto k_tuple_two_elements_input = "for (a, b) in pairs { process(a, b); }";
inline auto const k_tuple_two_elements_expected = test_json::for_expr(
    test_json::tuple_pattern({test_json::simple_pattern("a"), test_json::simple_pattern("b")}),
    test_json::var_name("pairs"),
    test_json::block({test_json::function_call_statement(
        test_json::function_call(test_json::var_name("process"), {test_json::var_name("a"), test_json::var_name("b")})
    )})
);

// Three element tuple pattern
constexpr auto k_tuple_three_elements_should_succeed = true;
constexpr auto k_tuple_three_elements_input = "for (x, y, z) in coords { move(x, y, z); }";
inline auto const k_tuple_three_elements_expected = test_json::for_expr(
    test_json::tuple_pattern(
        {test_json::simple_pattern("x"), test_json::simple_pattern("y"), test_json::simple_pattern("z")}
    ),
    test_json::var_name("coords"),
    test_json::block({test_json::function_call_statement(
        test_json::function_call(
            test_json::var_name("move"),
            {test_json::var_name("x"), test_json::var_name("y"), test_json::var_name("z")}
        )
    )})
);

// Single element tuple (edge case)
constexpr auto k_tuple_single_element_should_succeed = true;
constexpr auto k_tuple_single_element_input = "for (item) in items { process(item); }";
inline auto const k_tuple_single_element_expected = test_json::for_expr(
    test_json::tuple_pattern({test_json::simple_pattern("item")}),
    test_json::var_name("items"),
    test_json::block({test_json::function_call_statement(
        test_json::function_call(test_json::var_name("process"), {test_json::var_name("item")})
    )})
);

// Nested tuple in tuple: for ((a, b), c) in ...
constexpr auto k_nested_tuple_in_tuple_should_succeed = true;
constexpr auto k_nested_tuple_in_tuple_input = "for ((a, b), c) in nested { print(a, b, c); }";
inline auto const k_nested_tuple_in_tuple_expected = test_json::for_expr(
    test_json::tuple_pattern(
        {test_json::tuple_pattern({test_json::simple_pattern("a"), test_json::simple_pattern("b")}),
         test_json::simple_pattern("c")}
    ),
    test_json::var_name("nested"),
    test_json::block({test_json::function_call_statement(
        test_json::function_call(
            test_json::var_name("print"),
            {test_json::var_name("a"), test_json::var_name("b"), test_json::var_name("c")}
        )
    )})
);

// Nested struct in tuple: for (Point { x: px, y: py }, z) in ...
constexpr auto k_nested_struct_in_tuple_should_succeed = true;
constexpr auto k_nested_struct_in_tuple_input = "for (Point { x: px, y: py }, z) in data { use(px, py, z); }";
inline auto const k_nested_struct_in_tuple_expected = R"({
  "For_Expr": {
    "pattern": {
      "Tuple_Pattern": {
        "elements": [
          {
            "Struct_Pattern": {
              "type_name": {
                "Path_Type": {
                  "segments": [
                    {"Type_Name_Segment": {"type_params": [], "value": "Point"}}
                  ]
                }
              },
              "fields": [
                {"Field_Pattern": {"name": "x", "pattern": {"Simple_Pattern": {"name": "px"}}}},
                {"Field_Pattern": {"name": "y", "pattern": {"Simple_Pattern": {"name": "py"}}}}
              ]
            }
          },
          {"Simple_Pattern": {"name": "z"}}
        ]
      }
    },
    "iterator": {
      "Var_Name": {
        "segments": [
          {"Var_Name_Segment": {"type_params": [], "value": "data"}}
        ]
      }
    },
    "body": {
      "Block": {
        "statements": [
          {
            "Func_Call_Statement": {
              "expr": {
                "Func_Call_Expr": {
                  "name": {
                    "Var_Name": {
                      "segments": [
                        {"Var_Name_Segment": {"type_params": [], "value": "use"}}
                      ]
                    }
                  },
                  "params": [
                    {
                      "Var_Name": {
                        "segments": [
                          {"Var_Name_Segment": {"type_params": [], "value": "px"}}
                        ]
                      }
                    },
                    {
                      "Var_Name": {
                        "segments": [
                          {"Var_Name_Segment": {"type_params": [], "value": "py"}}
                        ]
                      }
                    },
                    {
                      "Var_Name": {
                        "segments": [
                          {"Var_Name_Segment": {"type_params": [], "value": "z"}}
                        ]
                      }
                    }
                  ]
                }
              }
            }
          }
        ]
      }
    }
  }
})";

// === Valid Tuple Pattern Tests (continued) ===

// Unit pattern (empty tuple) - () matches the unit value
constexpr auto k_unit_pattern_should_succeed = true;
constexpr auto k_unit_pattern_input = "for () in items { process(); }";
inline auto const k_unit_pattern_expected = test_json::for_expr(
    test_json::tuple_pattern({}),  // Empty tuple = unit pattern
    test_json::var_name("items"),
    test_json::block({test_json::function_call_statement(test_json::function_call(test_json::var_name("process"), {}))})
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

TEST_CASE("Parse Tuple_Pattern in For_Expr", "[parser][tuple_pattern]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          {"two element tuple",
           k_tuple_two_elements_input,
           k_tuple_two_elements_expected,
           k_tuple_two_elements_should_succeed},
          {"three element tuple",
           k_tuple_three_elements_input,
           k_tuple_three_elements_expected,
           k_tuple_three_elements_should_succeed},
          {"single element tuple",
           k_tuple_single_element_input,
           k_tuple_single_element_expected,
           k_tuple_single_element_should_succeed},
          {"nested tuple in tuple",
           k_nested_tuple_in_tuple_input,
           k_nested_tuple_in_tuple_expected,
           k_nested_tuple_in_tuple_should_succeed},
          {"nested struct in tuple",
           k_nested_struct_in_tuple_input,
           k_nested_struct_in_tuple_expected,
           k_nested_struct_in_tuple_should_succeed},
          {"unit pattern (empty tuple)", k_unit_pattern_input, k_unit_pattern_expected, k_unit_pattern_should_succeed},
          {"missing closing paren", k_missing_closing_paren_input, "", k_missing_closing_paren_should_succeed},
          {"missing opening paren", k_missing_opening_paren_input, "", k_missing_opening_paren_should_succeed},
          {"trailing comma", k_trailing_comma_input, "", k_trailing_comma_should_succeed},
          {"leading comma", k_leading_comma_input, "", k_leading_comma_should_succeed},
          {"double comma", k_double_comma_input, "", k_double_comma_should_succeed},
          {"invalid identifier", k_invalid_identifier_input, "", k_invalid_identifier_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
