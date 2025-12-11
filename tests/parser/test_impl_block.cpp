#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Impl_Block;

PARSE_TEST(Impl_Block, impl_block)

namespace {
// Empty impl block
constexpr auto k_empty_impl_should_succeed = true;
constexpr auto k_empty_impl_input = "impl Point { }";
inline auto const k_empty_impl_expected = R"(
{
  "Impl_Block": {
    "type_name": {
      "Type_Name": {
        "segments": [
          {"Type_Name_Segment": {"type_params": [], "value": "Point"}}
        ]
      }
    },
    "methods": []
  }
}
)";

// Basic impl block with single method
constexpr auto k_basic_single_method_should_succeed = true;
constexpr auto k_basic_single_method_input = "impl Point { fn distance(self: Point): F64 { return 0.0; } }";
inline auto const k_basic_single_method_expected = R"(
{
  "Impl_Block": {
    "type_name": {
      "Type_Name": {
        "segments": [
          {"Type_Name_Segment": {"type_params": [], "value": "Point"}}
        ]
      }
    },
    "methods": [
      {
        "Function_Definition": {
          "body": {
            "Block": {
              "statements": [
                {
                  "Return_Statement": {
                    "expr": {
                      "Float": {
                        "value": "0.0"
                      }
                    }
                  }
                }
              ]
            }
          },
          "declaration": {
            "Function_Declaration": {
              "name": "distance",
              "parameters": [
                {
                  "Function_Parameter": {
                    "is_mut": false,
                    "name": "self",
                    "type": {
                      "Type_Name": {
                        "segments": [
                          {"Type_Name_Segment": {"type_params": [], "value": "Point"}}
                        ]
                      }
                    }
                  }
                }
              ],
              "returnType": {
                "Type_Name": {
                  "segments": [
                    {"Type_Name_Segment": {"type_params": [], "value": "F64"}}
                  ]
                }
              },
              "type_params": []
            }
          }
        }
      }
    ]
  }
}
)";

// Impl block with optional self type (no explicit type annotation)
constexpr auto k_optional_self_type_should_succeed = true;
constexpr auto k_optional_self_type_input = "impl Point { fn distance(self): F64 { return 0.0; } }";
inline auto const k_optional_self_type_expected = R"(
{
  "Impl_Block": {
    "type_name": {
      "Type_Name": {
        "segments": [
          {"Type_Name_Segment": {"type_params": [], "value": "Point"}}
        ]
      }
    },
    "methods": [
      {
        "Function_Definition": {
          "body": {
            "Block": {
              "statements": [
                {
                  "Return_Statement": {
                    "expr": {
                      "Float": {
                        "value": "0.0"
                      }
                    }
                  }
                }
              ]
            }
          },
          "declaration": {
            "Function_Declaration": {
              "name": "distance",
              "parameters": [
                {
                  "Function_Parameter": {
                    "is_mut": false,
                    "name": "self"
                  }
                }
              ],
              "returnType": {
                "Type_Name": {
                  "segments": [
                    {"Type_Name_Segment": {"type_params": [], "value": "F64"}}
                  ]
                }
              },
              "type_params": []
            }
          }
        }
      }
    ]
  }
}
)";

// Generic impl block
constexpr auto k_generic_single_param_should_succeed = true;
constexpr auto k_generic_single_param_input = "impl<T> Array<T> { fn len(self: Array<T>): I32 { return 0; } }";
inline auto const k_generic_single_param_expected = R"(
{
  "Impl_Block": {
    "type_name": {
      "Type_Name": {
        "segments": [
          {
            "Type_Name_Segment": {
              "type_params": [
                {"Type_Name": {"segments": [{"Type_Name_Segment": {"type_params": [], "value": "T"}}]}}
              ],
              "value": "Array"
            }
          }
        ]
      }
    },
    "type_params": [
      {"Type_Name": {"segments": [{"Type_Name_Segment": {"type_params": [], "value": "T"}}]}}
    ],
    "methods": [
      {
        "Function_Definition": {
          "body": {
            "Block": {
              "statements": [
                {
                  "Return_Statement": {
                    "expr": {
                      "Integer": {
                        "value": "0"
                      }
                    }
                  }
                }
              ]
            }
          },
          "declaration": {
            "Function_Declaration": {
              "name": "len",
              "parameters": [
                {
                  "Function_Parameter": {
                    "is_mut": false,
                    "name": "self",
                    "type": {
                      "Type_Name": {
                        "segments": [
                          {
                            "Type_Name_Segment": {
                              "type_params": [
                                {"Type_Name": {"segments": [{"Type_Name_Segment": {"type_params": [], "value": "T"}}]}}
                              ],
                              "value": "Array"
                            }
                          }
                        ]
                      }
                    }
                  }
                }
              ],
              "returnType": {
                "Type_Name": {
                  "segments": [
                    {"Type_Name_Segment": {"type_params": [], "value": "I32"}}
                  ]
                }
              },
              "type_params": []
            }
          }
        }
      }
    ]
  }
}
)";

// Invalid cases
constexpr auto k_invalid_no_braces_should_succeed = false;
constexpr auto k_invalid_no_braces_input = "impl Point";
constexpr auto k_invalid_no_braces_expected =
    R"({"Impl_Block": {"type_name": {"Type_Name": {"segments": []}}, "methods": []}})";

constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";
constexpr auto k_invalid_empty_expected =
    R"({"Impl_Block": {"type_name": {"Type_Name": {"segments": []}}, "methods": []}})";
}  // namespace

TEST_CASE("Parse Impl_Block", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Impl_Block_Params>({
          {"empty impl", k_empty_impl_input, k_empty_impl_expected, k_empty_impl_should_succeed},
          {"basic single method",
           k_basic_single_method_input,
           k_basic_single_method_expected,
           k_basic_single_method_should_succeed},
          {"optional self type",
           k_optional_self_type_input,
           k_optional_self_type_expected,
           k_optional_self_type_should_succeed},
          {"generic single param",
           k_generic_single_param_input,
           k_generic_single_param_expected,
           k_generic_single_param_should_succeed},
          {"invalid - no braces",
           k_invalid_no_braces_input,
           k_invalid_no_braces_expected,
           k_invalid_no_braces_should_succeed},
          {"invalid - empty", k_invalid_empty_input, k_invalid_empty_expected, k_invalid_empty_should_succeed},
      })
  );

  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
