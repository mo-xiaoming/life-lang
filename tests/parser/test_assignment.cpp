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
    test_json::assignment_expr(test_json::var_name("x"), test_json::integer(42));

// Assignment with expression
constexpr auto k_assignment_with_expr_should_succeed = true;
constexpr auto k_assignment_with_expr_input = "x = y + 10";
inline auto const k_assignment_with_expr_expected = test_json::assignment_expr(
    test_json::var_name("x"), test_json::binary_expr("+", test_json::var_name("y"), test_json::integer(10))
);

// Field access assignment
constexpr auto k_field_assignment_should_succeed = true;
constexpr auto k_field_assignment_input = "point.x = 5";
inline auto const k_field_assignment_expected = R"({
  "Assignment_Expr": {
    "target": {
      "Field_Access_Expr": {
        "object": {
          "Variable_Name": {
            "segments": [
              {
                "Variable_Name_Segment": {
                  "template_parameters": [],
                  "value": "point"
                }
              }
            ]
          }
        },
        "field_name": "x"
      }
    },
    "value": {
      "Integer": {
        "value": "5"
      }
    }
  }
})";

// Nested field assignment
constexpr auto k_nested_field_assignment_should_succeed = true;
constexpr auto k_nested_field_assignment_input = "obj.data.count = 100";
inline auto const k_nested_field_assignment_expected = R"({
  "Assignment_Expr": {
    "target": {
      "Field_Access_Expr": {
        "object": {
          "Field_Access_Expr": {
            "object": {
              "Variable_Name": {
                "segments": [
                  {
                    "Variable_Name_Segment": {
                      "template_parameters": [],
                      "value": "obj"
                    }
                  }
                ]
              }
            },
            "field_name": "data"
          }
        },
        "field_name": "count"
      }
    },
    "value": {
      "Integer": {
        "value": "100"
      }
    }
  }
})";

// Right-associative chained assignment (x = y = z parses as x = (y = z))
constexpr auto k_chained_assignment_should_succeed = true;
constexpr auto k_chained_assignment_input = "x = y = 42";
inline auto const k_chained_assignment_expected = R"({
  "Assignment_Expr": {
    "target": {
      "Variable_Name": {
        "segments": [
          {
            "Variable_Name_Segment": {
              "template_parameters": [],
              "value": "x"
            }
          }
        ]
      }
    },
    "value": {
      "Assignment_Expr": {
        "target": {
          "Variable_Name": {
            "segments": [
              {
                "Variable_Name_Segment": {
                  "template_parameters": [],
                  "value": "y"
                }
              }
            ]
          }
        },
        "value": {
          "Integer": {
            "value": "42"
          }
        }
      }
    }
  }
})";

// Assignment with function call
constexpr auto k_assignment_with_call_should_succeed = true;
constexpr auto k_assignment_with_call_input = "result = calculate()";
inline auto const k_assignment_with_call_expected = R"({
  "Assignment_Expr": {
    "target": {
      "Variable_Name": {
        "segments": [
          {
            "Variable_Name_Segment": {
              "template_parameters": [],
              "value": "result"
            }
          }
        ]
      }
    },
    "value": {
      "Function_Call_Expr": {
        "name": {
          "Variable_Name": {
            "segments": [
              {
                "Variable_Name_Segment": {
                  "template_parameters": [],
                  "value": "calculate"
                }
              }
            ]
          }
        },
        "parameters": []
      }
    }
  }
})";

// Assignment with string
constexpr auto k_assignment_with_string_should_succeed = true;
constexpr auto k_assignment_with_string_input = R"(name = "Alice")";
inline auto const k_assignment_with_string_expected = R"({
  "Assignment_Expr": {
    "target": {
      "Variable_Name": {
        "segments": [
          {
            "Variable_Name_Segment": {
              "template_parameters": [],
              "value": "name"
            }
          }
        ]
      }
    },
    "value": {
      "String": {
        "value": "\"Alice\""
      }
    }
  }
})";

// Assignment precedence: x = y + z (should parse as x = (y + z))
constexpr auto k_assignment_precedence_should_succeed = true;
constexpr auto k_assignment_precedence_input = "x = y + z";
inline auto const k_assignment_precedence_expected = R"({
  "Assignment_Expr": {
    "target": {
      "Variable_Name": {
        "segments": [
          {
            "Variable_Name_Segment": {
              "template_parameters": [],
              "value": "x"
            }
          }
        ]
      }
    },
    "value": {
      "Binary_Expr": {
        "lhs": {
          "Variable_Name": {
            "segments": [
              {
                "Variable_Name_Segment": {
                  "template_parameters": [],
                  "value": "y"
                }
              }
            ]
          }
        },
        "op": "+",
        "rhs": {
          "Variable_Name": {
            "segments": [
              {
                "Variable_Name_Segment": {
                  "template_parameters": [],
                  "value": "z"
                }
              }
            ]
          }
        }
      }
    }
  }
})";

// Assignment with comparison (x = y < z should parse as x = (y < z))
constexpr auto k_assignment_with_comparison_should_succeed = true;
constexpr auto k_assignment_with_comparison_input = "flag = x < 10";
inline auto const k_assignment_with_comparison_expected = R"({
  "Assignment_Expr": {
    "target": {
      "Variable_Name": {
        "segments": [
          {
            "Variable_Name_Segment": {
              "template_parameters": [],
              "value": "flag"
            }
          }
        ]
      }
    },
    "value": {
      "Binary_Expr": {
        "lhs": {
          "Variable_Name": {
            "segments": [
              {
                "Variable_Name_Segment": {
                  "template_parameters": [],
                  "value": "x"
                }
              }
            ]
          }
        },
        "op": "<",
        "rhs": {
          "Integer": {
            "value": "10"
          }
        }
      }
    }
  }
})";

// Assignment self-reference (count = count + 1)
constexpr auto k_assignment_self_reference_should_succeed = true;
constexpr auto k_assignment_self_reference_input = "count = count + 1";
inline auto const k_assignment_self_reference_expected = R"({
  "Assignment_Expr": {
    "target": {
      "Variable_Name": {
        "segments": [
          {
            "Variable_Name_Segment": {
              "template_parameters": [],
              "value": "count"
            }
          }
        ]
      }
    },
    "value": {
      "Binary_Expr": {
        "lhs": {
          "Variable_Name": {
            "segments": [
              {
                "Variable_Name_Segment": {
                  "template_parameters": [],
                  "value": "count"
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
    }
  }
})";

}  // namespace

TEST_CASE("Parse Assignment_Expr", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          {"simple assignment", k_simple_assignment_input, k_simple_assignment_expected,
           k_simple_assignment_should_succeed},
          {"assignment with expr", k_assignment_with_expr_input, k_assignment_with_expr_expected,
           k_assignment_with_expr_should_succeed},
          {"field assignment", k_field_assignment_input, k_field_assignment_expected,
           k_field_assignment_should_succeed},
          {"nested field assignment", k_nested_field_assignment_input, k_nested_field_assignment_expected,
           k_nested_field_assignment_should_succeed},
          {"chained assignment", k_chained_assignment_input, k_chained_assignment_expected,
           k_chained_assignment_should_succeed},
          {"assignment with call", k_assignment_with_call_input, k_assignment_with_call_expected,
           k_assignment_with_call_should_succeed},
          {"assignment with string", k_assignment_with_string_input, k_assignment_with_string_expected,
           k_assignment_with_string_should_succeed},
          {"assignment precedence", k_assignment_precedence_input, k_assignment_precedence_expected,
           k_assignment_precedence_should_succeed},
          {"assignment with comparison", k_assignment_with_comparison_input, k_assignment_with_comparison_expected,
           k_assignment_with_comparison_should_succeed},
          {"assignment self-reference", k_assignment_self_reference_input, k_assignment_self_reference_expected,
           k_assignment_self_reference_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) { check_parse(params); }
}
