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
    test_json::range_expr(test_json::integer(0), test_json::integer(10), false);

// Simple integer range (inclusive)
constexpr auto k_simple_inclusive_should_succeed = true;
constexpr auto k_simple_inclusive_input = "0..=10";
inline auto const k_simple_inclusive_expected =
    test_json::range_expr(test_json::integer(0), test_json::integer(10), true);

// Variable range (exclusive)
constexpr auto k_variable_range_should_succeed = true;
constexpr auto k_variable_range_input = "start..end";
inline auto const k_variable_range_expected =
    test_json::range_expr(test_json::var_name("start"), test_json::var_name("end"), false);

// Variable range (inclusive)
constexpr auto k_variable_range_inclusive_should_succeed = true;
constexpr auto k_variable_range_inclusive_input = "start..=end";
inline auto const k_variable_range_inclusive_expected = fmt::format(
    R"({{
  "Range_Expr": {{
    "start": {},
    "end": {},
    "inclusive": true
  }}
}})",
    test_json::var_name("start"),
    test_json::var_name("end")
);

// Range with arithmetic expressions
constexpr auto k_arithmetic_range_should_succeed = true;
constexpr auto k_arithmetic_range_input = "x+1..y-1";
inline auto const k_arithmetic_range_expected = R"({
  "Range_Expr": {
    "start": {
      "Binary_Expr": {
        "lhs": {
          "Variable_Name": {
            "segments": [
              {
                "Variable_Name_Segment": {
                  "type_params": [],
                  "value": "x"
                }
              }
            ]
          }
        },
        "op": "+",
        "rhs": {
          "Integer": {
            "value": "1"
          }
        }
      }
    },
    "end": {
      "Binary_Expr": {
        "lhs": {
          "Variable_Name": {
            "segments": [
              {
                "Variable_Name_Segment": {
                  "type_params": [],
                  "value": "y"
                }
              }
            ]
          }
        },
        "op": "-",
        "rhs": {
          "Integer": {
            "value": "1"
          }
        }
      }
    },
    "inclusive": false
  }
})";

// Range with spaces
constexpr auto k_range_with_spaces_should_succeed = true;
constexpr auto k_range_with_spaces_input = "0 .. 10";
inline auto const k_range_with_spaces_expected = R"({
  "Range_Expr": {
    "start": {
      "Integer": {
        "value": "0"
      }
    },
    "end": {
      "Integer": {
        "value": "10"
      }
    },
    "inclusive": false
  }
})";

// Range with spaces (inclusive)
constexpr auto k_range_inclusive_spaces_should_succeed = true;
constexpr auto k_range_inclusive_spaces_input = "1 ..= 100";
inline auto const k_range_inclusive_spaces_expected = R"({
  "Range_Expr": {
    "start": {
      "Integer": {
        "value": "1"
      }
    },
    "end": {
      "Integer": {
        "value": "100"
      }
    },
    "inclusive": true
  }
})";

// Range with negative numbers
constexpr auto k_negative_range_should_succeed = true;
constexpr auto k_negative_range_input = "-10..10";
inline auto const k_negative_range_expected = R"({
  "Range_Expr": {
    "start": {
      "Unary_Expr": {
        "op": "-",
        "operand": {
          "Integer": {
            "value": "10"
          }
        }
      }
    },
    "end": {
      "Integer": {
        "value": "10"
      }
    },
    "inclusive": false
  }
})";

// Large numbers
constexpr auto k_large_range_should_succeed = true;
constexpr auto k_large_range_input = "1000..=9999";
inline auto const k_large_range_expected = R"({
  "Range_Expr": {
    "start": {
      "Integer": {
        "value": "1000"
      }
    },
    "end": {
      "Integer": {
        "value": "9999"
      }
    },
    "inclusive": true
  }
})";

// === Invalid Range Expressions ===

// Missing start - `..` is not a valid expression start
constexpr auto k_missing_start_should_succeed = false;
constexpr auto k_missing_start_input = "..10";

// Only dots - not a valid expression
constexpr auto k_only_dots_should_succeed = false;
constexpr auto k_only_dots_input = "..";

}  // namespace

TEST_CASE("Parse Range_Expr", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          // Valid cases
          {"simple exclusive",
           k_simple_exclusive_input,
           k_simple_exclusive_expected,
           k_simple_exclusive_should_succeed},
          {"simple inclusive",
           k_simple_inclusive_input,
           k_simple_inclusive_expected,
           k_simple_inclusive_should_succeed},
          {"variable range", k_variable_range_input, k_variable_range_expected, k_variable_range_should_succeed},
          {"variable range inclusive",
           k_variable_range_inclusive_input,
           k_variable_range_inclusive_expected,
           k_variable_range_inclusive_should_succeed},
          {"arithmetic range",
           k_arithmetic_range_input,
           k_arithmetic_range_expected,
           k_arithmetic_range_should_succeed},
          {"range with spaces",
           k_range_with_spaces_input,
           k_range_with_spaces_expected,
           k_range_with_spaces_should_succeed},
          {"range inclusive spaces",
           k_range_inclusive_spaces_input,
           k_range_inclusive_spaces_expected,
           k_range_inclusive_spaces_should_succeed},
          {"negative range", k_negative_range_input, k_negative_range_expected, k_negative_range_should_succeed},
          {"large range", k_large_range_input, k_large_range_expected, k_large_range_should_succeed},

          // Invalid cases
          {"invalid - missing start", k_missing_start_input, "", k_missing_start_should_succeed},
          {"invalid - only dots", k_only_dots_input, "", k_only_dots_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
