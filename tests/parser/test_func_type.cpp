// Function Type Parser Tests

#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Function_Type;

PARSE_TEST(Function_Type, function_type)

namespace {

// Simple function type with no params
constexpr auto k_no_params_should_succeed = true;
constexpr auto k_no_params_input = "fn(): ()";
inline auto const k_no_params_expected = R"JSON({
  "Function_Type": {
    "param_types": [],
    "return_type": {
      "Tuple_Type": {
        "element_types": []
      }
    }
  }
})JSON";

// Function type with single param
constexpr auto k_single_param_should_succeed = true;
constexpr auto k_single_param_input = "fn(I32): Bool";
inline auto const k_single_param_expected = R"({
  "Function_Type": {
    "param_types": [
      {
        "Path_Type": {
          "segments": [
            {
              "Type_Name_Segment": {
                "value": "I32",
                "type_params": []
              }
            }
          ]
        }
      }
    ],
    "return_type": {
      "Path_Type": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "Bool",
              "type_params": []
            }
          }
        ]
      }
    }
  }
})";

// Function type with multiple params
constexpr auto k_multiple_params_should_succeed = true;
constexpr auto k_multiple_params_input = "fn(I32, I32): I32";
inline auto const k_multiple_params_expected = R"({
  "Function_Type": {
    "param_types": [
      {
        "Path_Type": {
          "segments": [
            {
              "Type_Name_Segment": {
                "value": "I32",
                "type_params": []
              }
            }
          ]
        }
      },
      {
        "Path_Type": {
          "segments": [
            {
              "Type_Name_Segment": {
                "value": "I32",
                "type_params": []
              }
            }
          ]
        }
      }
    ],
    "return_type": {
      "Path_Type": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "I32",
              "type_params": []
            }
          }
        ]
      }
    }
  }
})";

// Function type with qualified types
constexpr auto k_qualified_types_should_succeed = true;
constexpr auto k_qualified_types_input = "fn(Std.String): Std.Result";
inline auto const k_qualified_types_expected = R"({
  "Function_Type": {
    "param_types": [
      {
        "Path_Type": {
          "segments": [
            {
              "Type_Name_Segment": {
                "value": "Std",
                "type_params": []
              }
            },
            {
              "Type_Name_Segment": {
                "value": "String",
                "type_params": []
              }
            }
          ]
        }
      }
    ],
    "return_type": {
      "Path_Type": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "Std",
              "type_params": []
            }
          },
          {
            "Type_Name_Segment": {
              "value": "Result",
              "type_params": []
            }
          }
        ]
      }
    }
  }
})";

// Higher-order function type (function takes function)
constexpr auto k_higher_order_should_succeed = true;
constexpr auto k_higher_order_input = "fn(fn(I32): Bool): Bool";
inline auto const k_higher_order_expected = R"({
  "Function_Type": {
    "param_types": [
      {
        "Function_Type": {
          "param_types": [
            {
              "Path_Type": {
                "segments": [
                  {
                    "Type_Name_Segment": {
                      "value": "I32",
                      "type_params": []
                    }
                  }
                ]
              }
            }
          ],
          "return_type": {
            "Path_Type": {
              "segments": [
                {
                  "Type_Name_Segment": {
                    "value": "Bool",
                    "type_params": []
                  }
                }
              ]
            }
          }
        }
      }
    ],
    "return_type": {
      "Path_Type": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "Bool",
              "type_params": []
            }
          }
        ]
      }
    }
  }
})";

// Function type with generic types
constexpr auto k_generic_types_should_succeed = true;
constexpr auto k_generic_types_input = "fn(Array<I32>): Option<I32>";
inline auto const k_generic_types_expected = R"({
  "Function_Type": {
    "param_types": [
      {
        "Path_Type": {
          "segments": [
            {
              "Type_Name_Segment": {
                "value": "Array",
                "type_params": [
                  {
                    "Path_Type": {
                      "segments": [
                        {
                          "Type_Name_Segment": {
                            "value": "I32",
                            "type_params": []
                          }
                        }
                      ]
                    }
                  }
                ]
              }
            }
          ]
        }
      }
    ],
    "return_type": {
      "Path_Type": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "Option",
              "type_params": [
                {
                  "Path_Type": {
                    "segments": [
                      {
                        "Type_Name_Segment": {
                          "value": "I32",
                          "type_params": []
                        }
                      }
                    ]
                  }
                }
              ]
            }
          }
        ]
      }
    }
  }
})";

// Function type with spaces
constexpr auto k_with_spaces_should_succeed = true;
constexpr auto k_with_spaces_input = "fn( I32 , Bool ): String";
inline auto const k_with_spaces_expected = R"({
  "Function_Type": {
    "param_types": [
      {
        "Path_Type": {
          "segments": [
            {
              "Type_Name_Segment": {
                "value": "I32",
                "type_params": []
              }
            }
          ]
        }
      },
      {
        "Path_Type": {
          "segments": [
            {
              "Type_Name_Segment": {
                "value": "Bool",
                "type_params": []
              }
            }
          ]
        }
      }
    ],
    "return_type": {
      "Path_Type": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "String",
              "type_params": []
            }
          }
        ]
      }
    }
  }
})";

// Invalid: missing return type
constexpr auto k_missing_return_type_should_succeed = false;
constexpr auto k_missing_return_type_input = "fn(I32)";

// Invalid: missing colon
constexpr auto k_missing_colon_should_succeed = false;
constexpr auto k_missing_colon_input = "fn(I32) Bool";

// Invalid: missing parentheses
constexpr auto k_missing_parens_should_succeed = false;
constexpr auto k_missing_parens_input = "fn I32: Bool";

}  // namespace

TEST_CASE("Parse Function_Type", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Function_Type_Params>({
          {"no params", k_no_params_input, k_no_params_expected, k_no_params_should_succeed},
          {"single param", k_single_param_input, k_single_param_expected, k_single_param_should_succeed},
          {"multiple params", k_multiple_params_input, k_multiple_params_expected, k_multiple_params_should_succeed},
          {"qualified types", k_qualified_types_input, k_qualified_types_expected, k_qualified_types_should_succeed},
          {"higher-order", k_higher_order_input, k_higher_order_expected, k_higher_order_should_succeed},
          {"generic types", k_generic_types_input, k_generic_types_expected, k_generic_types_should_succeed},
          {"with spaces", k_with_spaces_input, k_with_spaces_expected, k_with_spaces_should_succeed},
          {"invalid - missing return type", k_missing_return_type_input, "", k_missing_return_type_should_succeed},
          {"invalid - missing colon", k_missing_colon_input, "", k_missing_colon_should_succeed},
          {"invalid - missing parens", k_missing_parens_input, "", k_missing_parens_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
