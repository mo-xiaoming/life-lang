#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;
using test_json::var_name;

PARSE_TEST(Expr, expr)

namespace {
// Basic while loop
constexpr auto k_basic_while_should_succeed = true;
constexpr auto k_basic_while_input = "while x { return 1; }";
inline auto const k_basic_while_expected = fmt::format(
    R"({{
  "While_Expr": {{
    "condition": {},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Return_Statement": {{
              "expr": {{
                "Integer": {{
                  "value": "1"
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    var_name("x")
);

// While with comparison condition
constexpr auto k_while_comparison_should_succeed = true;
constexpr auto k_while_comparison_input = "while x < 10 { return x; }";
inline auto const k_while_comparison_expected = fmt::format(
    R"({{
  "While_Expr": {{
    "condition": {{
      "Binary_Expr": {{
        "lhs": {},
        "op": "<",
        "rhs": {{
          "Integer": {{
            "value": "10"
          }}
        }}
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Return_Statement": {{
              "expr": {}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    var_name("x"), var_name("x")
);

// While with complex condition
constexpr auto k_while_complex_condition_should_succeed = true;
constexpr auto k_while_complex_condition_input = "while x > 0 && y < 100 { foo(); }";
inline auto const k_while_complex_condition_expected = fmt::format(
    R"({{
  "While_Expr": {{
    "condition": {{
      "Binary_Expr": {{
        "lhs": {{
          "Binary_Expr": {{
            "lhs": {},
            "op": ">",
            "rhs": {{
              "Integer": {{
                "value": "0"
              }}
            }}
          }}
        }},
        "op": "&&",
        "rhs": {{
          "Binary_Expr": {{
            "lhs": {},
            "op": "<",
            "rhs": {{
              "Integer": {{
                "value": "100"
              }}
            }}
          }}
        }}
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Function_Call_Statement": {{
              "expr": {{
                "Function_Call_Expr": {{
                  "name": {},
                  "parameters": []
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    var_name("x"), var_name("y"), var_name("foo")
);

// While with empty body
constexpr auto k_while_empty_body_should_succeed = true;
constexpr auto k_while_empty_body_input = "while condition {}";
inline auto const k_while_empty_body_expected = fmt::format(
    R"({{
  "While_Expr": {{
    "condition": {},
    "body": {{
      "Block": {{
        "statements": []
      }}
    }}
  }}
}})",
    var_name("condition")
);

// While with multiple statements
constexpr auto k_while_multiple_statements_should_succeed = true;
constexpr auto k_while_multiple_statements_input = "while x { foo(); bar(); return x; }";
inline auto const k_while_multiple_statements_expected = fmt::format(
    R"({{
  "While_Expr": {{
    "condition": {},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Function_Call_Statement": {{
              "expr": {{
                "Function_Call_Expr": {{
                  "name": {},
                  "parameters": []
                }}
              }}
            }}
          }},
          {{
            "Function_Call_Statement": {{
              "expr": {{
                "Function_Call_Expr": {{
                  "name": {},
                  "parameters": []
                }}
              }}
            }}
          }},
          {{
            "Return_Statement": {{
              "expr": {}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    var_name("x"), var_name("foo"), var_name("bar"), var_name("x")
);

// While with function call condition
constexpr auto k_while_function_condition_should_succeed = true;
constexpr auto k_while_function_condition_input = "while has_more() { process(); }";
inline auto const k_while_function_condition_expected = fmt::format(
    R"({{
  "While_Expr": {{
    "condition": {{
      "Function_Call_Expr": {{
        "name": {},
        "parameters": []
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Function_Call_Statement": {{
              "expr": {{
                "Function_Call_Expr": {{
                  "name": {},
                  "parameters": []
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    var_name("has_more"), var_name("process")
);

// While with unary operator condition
constexpr auto k_while_unary_condition_should_succeed = true;
constexpr auto k_while_unary_condition_input = "while !done { work(); }";
inline auto const k_while_unary_condition_expected = fmt::format(
    R"({{
  "While_Expr": {{
    "condition": {{
      "Unary_Expr": {{
        "op": "!",
        "operand": {}
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Function_Call_Statement": {{
              "expr": {{
                "Function_Call_Expr": {{
                  "name": {},
                  "parameters": []
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    var_name("done"), var_name("work")
);

// Nested while loops
constexpr auto k_nested_while_should_succeed = true;
constexpr auto k_nested_while_input = "while x { while y { foo(); } }";
inline auto const k_nested_while_expected = fmt::format(
    R"({{
  "While_Expr": {{
    "condition": {},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "While_Statement": {{
              "expr": {{
                "While_Expr": {{
                  "condition": {},
                  "body": {{
                    "Block": {{
                      "statements": [
                        {{
                          "Function_Call_Statement": {{
                            "expr": {{
                              "Function_Call_Expr": {{
                                "name": {},
                                "parameters": []
                              }}
                            }}
                          }}
                        }}
                      ]
                    }}
                  }}
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    var_name("x"), var_name("y"), var_name("foo")
);

// While with both < and > in condition
constexpr auto k_while_less_and_greater_should_succeed = true;
constexpr auto k_while_less_and_greater_input = "while x < 10 && y > 5 { process(); }";
inline auto const k_while_less_and_greater_expected = fmt::format(
    R"({{
  "While_Expr": {{
    "condition": {{
      "Binary_Expr": {{
        "lhs": {{
          "Binary_Expr": {{
            "lhs": {},
            "op": "<",
            "rhs": {{
              "Integer": {{
                "value": "10"
              }}
            }}
          }}
        }},
        "op": "&&",
        "rhs": {{
          "Binary_Expr": {{
            "lhs": {},
            "op": ">",
            "rhs": {{
              "Integer": {{
                "value": "5"
              }}
            }}
          }}
        }}
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Function_Call_Statement": {{
              "expr": {{
                "Function_Call_Expr": {{
                  "name": {},
                  "parameters": []
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    var_name("x"), var_name("y"), var_name("process")
);

// Invalid cases
constexpr auto k_missing_condition_should_succeed = false;
constexpr auto k_missing_condition_input = "while { return 1; }";
inline auto const k_missing_condition_expected = "";

constexpr auto k_missing_body_should_succeed = false;
constexpr auto k_missing_body_input = "while x";
inline auto const k_missing_body_expected = "";

constexpr auto k_missing_braces_should_succeed = false;
constexpr auto k_missing_braces_input = "while x return 1;";
inline auto const k_missing_braces_expected = "";

constexpr auto k_parentheses_condition_should_succeed = false;
constexpr auto k_parentheses_condition_input = "while (x) { return 1; }";
inline auto const k_parentheses_condition_expected = "";

}  // namespace

TEST_CASE("Parse While_Expr", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          {"basic while", k_basic_while_input, k_basic_while_expected, k_basic_while_should_succeed},
          {"while with comparison", k_while_comparison_input, k_while_comparison_expected,
           k_while_comparison_should_succeed},
          {"while with complex condition", k_while_complex_condition_input, k_while_complex_condition_expected,
           k_while_complex_condition_should_succeed},
          {"while with empty body", k_while_empty_body_input, k_while_empty_body_expected,
           k_while_empty_body_should_succeed},
          {"while with multiple statements", k_while_multiple_statements_input, k_while_multiple_statements_expected,
           k_while_multiple_statements_should_succeed},
          {"while with function condition", k_while_function_condition_input, k_while_function_condition_expected,
           k_while_function_condition_should_succeed},
          {"while with unary condition", k_while_unary_condition_input, k_while_unary_condition_expected,
           k_while_unary_condition_should_succeed},
          {"nested while loops", k_nested_while_input, k_nested_while_expected, k_nested_while_should_succeed},
          {"while with < and >", k_while_less_and_greater_input, k_while_less_and_greater_expected,
           k_while_less_and_greater_should_succeed},
          {"invalid: missing condition", k_missing_condition_input, k_missing_condition_expected,
           k_missing_condition_should_succeed},
          {"invalid: missing body", k_missing_body_input, k_missing_body_expected, k_missing_body_should_succeed},
          {"invalid: missing braces", k_missing_braces_input, k_missing_braces_expected,
           k_missing_braces_should_succeed},
          {"invalid: parentheses around condition", k_parentheses_condition_input, k_parentheses_condition_expected,
           k_parentheses_condition_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) { check_parse(params); }
}
