#include "internal_rules.hpp"
#include "utils.hpp"
using life_lang::ast::Statement;

// PARSE_TEST generates check_parse() function and Statement_Params type
PARSE_TEST(Statement, statement)

namespace {
// === Valid Let Statements ===

// Simple let binding
constexpr auto k_simple_let_should_succeed = true;
constexpr auto k_simple_let_input = "let x = 42;";
inline auto const k_simple_let_expected = R"({
  "Let_Statement": {
    "is_mut": false,
    "pattern": {
      "Simple_Pattern": {
        "name": "x"
      }
    },
    "type": null,
    "value": {
      "Integer": {
        "value": "42"
      }
    }
  }
})";

// Let with type annotation
constexpr auto k_let_with_type_should_succeed = true;
constexpr auto k_let_with_type_input = "let x: I32 = 42;";
inline auto const k_let_with_type_expected = R"({
  "Let_Statement": {
    "is_mut": false,
    "pattern": {
      "Simple_Pattern": {
        "name": "x"
      }
    },
    "type": {
      "Type_Name": {
        "segments": [
          {
            "Type_Name_Segment": {
              "template_parameters": [],
              "value": "I32"
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
})";

// Let mut binding
constexpr auto k_let_mut_should_succeed = true;
constexpr auto k_let_mut_input = "let mut count = 0;";
inline auto const k_let_mut_expected = R"({
  "Let_Statement": {
    "is_mut": true,
    "pattern": {
      "Simple_Pattern": {
        "name": "count"
      }
    },
    "type": null,
    "value": {
      "Integer": {
        "value": "0"
      }
    }
  }
})";

// Let mut with type annotation
constexpr auto k_let_mut_with_type_should_succeed = true;
constexpr auto k_let_mut_with_type_input = "let mut counter: I32 = 100;";
inline auto const k_let_mut_with_type_expected = R"({
  "Let_Statement": {
    "is_mut": true,
    "pattern": {
      "Simple_Pattern": {
        "name": "counter"
      }
    },
    "type": {
      "Type_Name": {
        "segments": [
          {
            "Type_Name_Segment": {
              "template_parameters": [],
              "value": "I32"
            }
          }
        ]
      }
    },
    "value": {
      "Integer": {
        "value": "100"
      }
    }
  }
})";

// Let with variable initializer
constexpr auto k_let_with_variable_should_succeed = true;
constexpr auto k_let_with_variable_input = "let y = x;";
inline auto const k_let_with_variable_expected = fmt::format(
    R"({{
  "Let_Statement": {{
    "is_mut": false,
    "pattern": {{
      "Simple_Pattern": {{
        "name": "y"
      }}
    }},
    "type": null,
    "value": {}
  }}
}})",
    test_json::var_name("x")
);

// Let with expression initializer
constexpr auto k_let_with_expression_should_succeed = true;
constexpr auto k_let_with_expression_input = "let result = x + 10;";
inline auto const k_let_with_expression_expected = R"({
  "Let_Statement": {
    "is_mut": false,
    "pattern": {
      "Simple_Pattern": {
        "name": "result"
      }
    },
    "type": null,
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
        "op": "+",
        "rhs": {
          "Integer": {
            "value": "10"
          }
        }
      }
    }
  }
})";

// Let with function call initializer
constexpr auto k_let_with_function_call_should_succeed = true;
constexpr auto k_let_with_function_call_input = "let value = calculate();";
inline auto const k_let_with_function_call_expected = R"({
  "Let_Statement": {
    "is_mut": false,
    "pattern": {
      "Simple_Pattern": {
        "name": "value"
      }
    },
    "type": null,
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

// Let with tuple pattern
constexpr auto k_let_with_tuple_pattern_should_succeed = true;
constexpr auto k_let_with_tuple_pattern_input = "let (x, y) = point;";
inline auto const k_let_with_tuple_pattern_expected = fmt::format(
    R"({{
  "Let_Statement": {{
    "is_mut": false,
    "pattern": {{
      "Tuple_Pattern": {{
        "elements": [
          {{
            "Simple_Pattern": {{
              "name": "x"
            }}
          }},
          {{
            "Simple_Pattern": {{
              "name": "y"
            }}
          }}
        ]
      }}
    }},
    "type": null,
    "value": {}
  }}
}})",
    test_json::var_name("point")
);

// Let with qualified type
constexpr auto k_let_with_qualified_type_should_succeed = true;
constexpr auto k_let_with_qualified_type_input = "let name: Std.String = create_name();";
inline auto const k_let_with_qualified_type_expected = R"({
  "Let_Statement": {
    "is_mut": false,
    "pattern": {
      "Simple_Pattern": {
        "name": "name"
      }
    },
    "type": {
      "Type_Name": {
        "segments": [
          {
            "Type_Name_Segment": {
              "template_parameters": [],
              "value": "Std"
            }
          },
          {
            "Type_Name_Segment": {
              "template_parameters": [],
              "value": "String"
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
                  "value": "create_name"
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

// Let with string initializer
constexpr auto k_let_with_string_should_succeed = true;
constexpr auto k_let_with_string_input = R"(let msg = "hello";)";
inline auto const k_let_with_string_expected = R"({
  "Let_Statement": {
    "is_mut": false,
    "pattern": {
      "Simple_Pattern": {
        "name": "msg"
      }
    },
    "type": null,
    "value": {
      "String": {
        "value": "\"hello\""
      }
    }
  }
})";

// === Invalid Let Statements ===

// Missing initializer
constexpr auto k_let_missing_init_should_succeed = false;
constexpr auto k_let_missing_init_input = "let x;";

// Missing pattern
constexpr auto k_let_missing_pattern_should_succeed = false;
constexpr auto k_let_missing_pattern_input = "let = 42;";

// Missing semicolon
constexpr auto k_let_missing_semicolon_should_succeed = false;
constexpr auto k_let_missing_semicolon_input = "let x = 42";

// Invalid pattern syntax
constexpr auto k_let_invalid_pattern_should_succeed = false;
constexpr auto k_let_invalid_pattern_input = "let 123 = x;";

// Missing equals sign
constexpr auto k_let_missing_equals_should_succeed = false;
constexpr auto k_let_missing_equals_input = "let x 42;";

}  // namespace

TEST_CASE("Parse Let_Statement", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Statement_Params>({
          {"simple let", k_simple_let_input, k_simple_let_expected, k_simple_let_should_succeed},
          {"let with type", k_let_with_type_input, k_let_with_type_expected, k_let_with_type_should_succeed},
          {"let mut", k_let_mut_input, k_let_mut_expected, k_let_mut_should_succeed},
          {"let mut with type", k_let_mut_with_type_input, k_let_mut_with_type_expected,
           k_let_mut_with_type_should_succeed},
          {"let with variable", k_let_with_variable_input, k_let_with_variable_expected,
           k_let_with_variable_should_succeed},
          {"let with expression", k_let_with_expression_input, k_let_with_expression_expected,
           k_let_with_expression_should_succeed},
          {"let with function call", k_let_with_function_call_input, k_let_with_function_call_expected,
           k_let_with_function_call_should_succeed},
          {"let with tuple pattern", k_let_with_tuple_pattern_input, k_let_with_tuple_pattern_expected,
           k_let_with_tuple_pattern_should_succeed},
          {"let with qualified type", k_let_with_qualified_type_input, k_let_with_qualified_type_expected,
           k_let_with_qualified_type_should_succeed},
          {"let with string", k_let_with_string_input, k_let_with_string_expected, k_let_with_string_should_succeed},
          {"invalid: missing init", k_let_missing_init_input, "", k_let_missing_init_should_succeed},
          {"invalid: missing pattern", k_let_missing_pattern_input, "", k_let_missing_pattern_should_succeed},
          {"invalid: missing semicolon", k_let_missing_semicolon_input, "", k_let_missing_semicolon_should_succeed},
          {"invalid: invalid pattern", k_let_invalid_pattern_input, "", k_let_invalid_pattern_should_succeed},
          {"invalid: missing equals", k_let_missing_equals_input, "", k_let_missing_equals_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) { check_parse(params); }
}
