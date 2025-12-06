#include "internal_rules.hpp"
#include "utils.hpp"
using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// Test cases for method chaining: foo().bar(), foo().bar().baz(), etc.

// Simple method call on function result: foo().bar()
constexpr auto k_method_on_call_result_should_succeed = true;
constexpr auto k_method_on_call_result_input = "foo().bar()";
inline auto const k_method_on_call_result_expected = R"({
  "Function_Call_Expr": {
    "name": {
      "Variable_Name": {
        "segments": [
          {"Variable_Name_Segment": {"template_parameters": [], "value": "bar"}}
        ]
      }
    },
    "parameters": [
      {
        "Function_Call_Expr": {
          "name": {
            "Variable_Name": {
              "segments": [
                {"Variable_Name_Segment": {"template_parameters": [], "value": "foo"}}
              ]
            }
          },
          "parameters": []
        }
      }
    ]
  }
})";

// Method call with arguments on function result: foo().bar(1, 2)
constexpr auto k_method_with_args_on_call_should_succeed = true;
constexpr auto k_method_with_args_on_call_input = "foo().bar(1, 2)";
inline auto const k_method_with_args_on_call_expected = R"({
  "Function_Call_Expr": {
    "name": {
      "Variable_Name": {
        "segments": [
          {"Variable_Name_Segment": {"template_parameters": [], "value": "bar"}}
        ]
      }
    },
    "parameters": [
      {
        "Function_Call_Expr": {
          "name": {
            "Variable_Name": {
              "segments": [
                {"Variable_Name_Segment": {"template_parameters": [], "value": "foo"}}
              ]
            }
          },
          "parameters": []
        }
      },
      {"Integer": {"value": "1"}},
      {"Integer": {"value": "2"}}
    ]
  }
})";

// Chained method calls: foo().bar().baz()
constexpr auto k_chained_method_calls_should_succeed = true;
constexpr auto k_chained_method_calls_input = "foo().bar().baz()";
inline auto const k_chained_method_calls_expected = R"({
  "Function_Call_Expr": {
    "name": {
      "Variable_Name": {
        "segments": [
          {"Variable_Name_Segment": {"template_parameters": [], "value": "baz"}}
        ]
      }
    },
    "parameters": [
      {
        "Function_Call_Expr": {
          "name": {
            "Variable_Name": {
              "segments": [
                {"Variable_Name_Segment": {"template_parameters": [], "value": "bar"}}
              ]
            }
          },
          "parameters": [
            {
              "Function_Call_Expr": {
                "name": {
                  "Variable_Name": {
                    "segments": [
                      {"Variable_Name_Segment": {"template_parameters": [], "value": "foo"}}
                    ]
                  }
                },
                "parameters": []
              }
            }
          ]
        }
      }
    ]
  }
})";

// Field access on function result: foo().field
constexpr auto k_field_on_call_result_should_succeed = true;
constexpr auto k_field_on_call_result_input = "foo().field";
inline auto const k_field_on_call_result_expected = R"({
  "Field_Access_Expr": {
    "field_name": "field",
    "object": {
      "Function_Call_Expr": {
        "name": {
          "Variable_Name": {
            "segments": [
              {"Variable_Name_Segment": {"template_parameters": [], "value": "foo"}}
            ]
          }
        },
        "parameters": []
      }
    }
  }
})";

// Path-based function call: obj.field.method() parses as qualified function name
// This is NOT a method call in UFCS style - it's a function call where the name is a dotted path
constexpr auto k_path_function_call_should_succeed = true;
constexpr auto k_path_function_call_input = "obj.field.method()";
inline auto const k_path_function_call_expected = R"({
  "Function_Call_Expr": {
    "name": {
      "Variable_Name": {
        "segments": [
          {"Variable_Name_Segment": {"template_parameters": [], "value": "obj"}},
          {"Variable_Name_Segment": {"template_parameters": [], "value": "field"}},
          {"Variable_Name_Segment": {"template_parameters": [], "value": "method"}}
        ]
      }
    },
    "parameters": []
  }
})";

// Mixed: field access on method result: foo().bar.baz
constexpr auto k_field_on_method_result_should_succeed = true;
constexpr auto k_field_on_method_result_input = "foo().bar.baz";
inline auto const k_field_on_method_result_expected = R"({
  "Field_Access_Expr": {
    "field_name": "baz",
    "object": {
      "Field_Access_Expr": {
        "field_name": "bar",
        "object": {
          "Function_Call_Expr": {
            "name": {
              "Variable_Name": {
                "segments": [
                  {"Variable_Name_Segment": {"template_parameters": [], "value": "foo"}}
                ]
              }
            },
            "parameters": []
          }
        }
      }
    }
  }
})";

// Complex chain: foo().bar(1).baz().qux
constexpr auto k_complex_chain_should_succeed = true;
constexpr auto k_complex_chain_input = "foo().bar(1).baz().qux";
inline auto const k_complex_chain_expected = R"({
  "Field_Access_Expr": {
    "field_name": "qux",
    "object": {
      "Function_Call_Expr": {
        "name": {
          "Variable_Name": {
            "segments": [
              {"Variable_Name_Segment": {"template_parameters": [], "value": "baz"}}
            ]
          }
        },
        "parameters": [
          {
            "Function_Call_Expr": {
              "name": {
                "Variable_Name": {
                  "segments": [
                    {"Variable_Name_Segment": {"template_parameters": [], "value": "bar"}}
                  ]
                }
              },
              "parameters": [
                {
                  "Function_Call_Expr": {
                    "name": {
                      "Variable_Name": {
                        "segments": [
                          {"Variable_Name_Segment": {"template_parameters": [], "value": "foo"}}
                        ]
                      }
                    },
                    "parameters": []
                  }
                },
                {"Integer": {"value": "1"}}
              ]
            }
          }
        ]
      }
    }
  }
})";

}  // namespace

TEST_CASE("Parse Method Chaining", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          {"method on call result", k_method_on_call_result_input, k_method_on_call_result_expected,
           k_method_on_call_result_should_succeed},
          {"method with args on call", k_method_with_args_on_call_input, k_method_with_args_on_call_expected,
           k_method_with_args_on_call_should_succeed},
          {"chained method calls", k_chained_method_calls_input, k_chained_method_calls_expected,
           k_chained_method_calls_should_succeed},
          {"field on call result", k_field_on_call_result_input, k_field_on_call_result_expected,
           k_field_on_call_result_should_succeed},
          {"path-based function call", k_path_function_call_input, k_path_function_call_expected,
           k_path_function_call_should_succeed},
          {"field on method result", k_field_on_method_result_input, k_field_on_method_result_expected,
           k_field_on_method_result_should_succeed},
          {"complex chain", k_complex_chain_input, k_complex_chain_expected, k_complex_chain_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) { check_parse(params); }
}
