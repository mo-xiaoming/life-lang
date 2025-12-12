#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Type_Alias;

PARSE_TEST(Type_Alias, type_alias)

namespace {
// Simple type alias
constexpr auto k_simple_alias_should_succeed = true;
constexpr auto k_simple_alias_input = "type My_Type = I32;";
inline auto const k_simple_alias_expected = R"(
{
  "Type_Alias": {
    "name": "My_Type",
    "aliased_type": {
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
}
)";

// Generic type alias with single parameter
constexpr auto k_generic_single_param_should_succeed = true;
constexpr auto k_generic_single_param_input = "type String_Map<T> = Map<String, T>;";
inline auto const k_generic_single_param_expected = R"(
{
  "Type_Alias": {
    "name": "String_Map",
    "type_params": [
      {
        "Type_Param": {
          "name": {
            "Path_Type": {
              "segments": [
                {
                  "Type_Name_Segment": {
                    "value": "T",
                    "type_params": []
                  }
                }
              ]
            }
          }
        }
      }
    ],
    "aliased_type": {
      "Path_Type": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "Map",
              "type_params": [
                {
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
                },
                {
                  "Path_Type": {
                    "segments": [
                      {
                        "Type_Name_Segment": {
                          "value": "T",
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
}
)";

// Generic type alias with multiple parameters
constexpr auto k_generic_multi_param_should_succeed = true;
constexpr auto k_generic_multi_param_input = "type Pair<A, B> = Tuple<A, B>;";
inline auto const k_generic_multi_param_expected = R"(
{
  "Type_Alias": {
    "name": "Pair",
    "type_params": [
      {
        "Type_Param": {
          "name": {
            "Path_Type": {
              "segments": [
                {
                  "Type_Name_Segment": {
                    "value": "A",
                    "type_params": []
                  }
                }
              ]
            }
          }
        }
      },
      {
        "Type_Param": {
          "name": {
            "Path_Type": {
              "segments": [
                {
                  "Type_Name_Segment": {
                    "value": "B",
                    "type_params": []
                  }
                }
              ]
            }
          }
        }
      }
    ],
    "aliased_type": {
      "Path_Type": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "Tuple",
              "type_params": [
                {
                  "Path_Type": {
                    "segments": [
                      {
                        "Type_Name_Segment": {
                          "value": "A",
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
                          "value": "B",
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
}
)";

// Qualified type path
constexpr auto k_qualified_path_should_succeed = true;
constexpr auto k_qualified_path_input = "type My_String = Std.String;";
inline auto const k_qualified_path_expected = R"(
{
  "Type_Alias": {
    "name": "My_String",
    "aliased_type": {
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
  }
}
)";

// Nested generic types
constexpr auto k_nested_generics_should_succeed = true;
constexpr auto k_nested_generics_input = "type Result_List<T, E> = Vec<Result<T, E>>;";
inline auto const k_nested_generics_expected = R"(
{
  "Type_Alias": {
    "name": "Result_List",
    "type_params": [
      {
        "Type_Param": {
          "name": {
            "Path_Type": {
              "segments": [
                {
                  "Type_Name_Segment": {
                    "value": "T",
                    "type_params": []
                  }
                }
              ]
            }
          }
        }
      },
      {
        "Type_Param": {
          "name": {
            "Path_Type": {
              "segments": [
                {
                  "Type_Name_Segment": {
                    "value": "E",
                    "type_params": []
                  }
                }
              ]
            }
          }
        }
      }
    ],
    "aliased_type": {
      "Path_Type": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "Vec",
              "type_params": [
                {
                  "Path_Type": {
                    "segments": [
                      {
                        "Type_Name_Segment": {
                          "value": "Result",
                          "type_params": [
                            {
                              "Path_Type": {
                                "segments": [
                                  {
                                    "Type_Name_Segment": {
                                      "value": "T",
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
                                      "value": "E",
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
              ]
            }
          }
        ]
      }
    }
  }
}
)";

// Type parameter with trait bounds
constexpr auto k_type_param_with_bounds_should_succeed = true;
constexpr auto k_type_param_with_bounds_input = "type Sorted_Vec<T: Ord> = Vec<T>;";
inline auto const k_type_param_with_bounds_expected = R"(
{
  "Type_Alias": {
    "name": "Sorted_Vec",
    "type_params": [
      {
        "Type_Param": {
          "name": {
            "Path_Type": {
              "segments": [
                {
                  "Type_Name_Segment": {
                    "value": "T",
                    "type_params": []
                  }
                }
              ]
            }
          },
          "bounds": [
            {
              "Trait_Bound": {
                "trait_name": {
                  "Path_Type": {
                    "segments": [
                      {
                        "Type_Name_Segment": {
                          "value": "Ord",
                          "type_params": []
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
    ],
    "aliased_type": {
      "Path_Type": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "Vec",
              "type_params": [
                {
                  "Path_Type": {
                    "segments": [
                      {
                        "Type_Name_Segment": {
                          "value": "T",
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
}
)";

// Multiple trait bounds
constexpr auto k_multiple_bounds_should_succeed = true;
constexpr auto k_multiple_bounds_input = "type Display_Vec<T: Display + Clone> = Vec<T>;";
inline auto const k_multiple_bounds_expected = R"(
{
  "Type_Alias": {
    "name": "Display_Vec",
    "type_params": [
      {
        "Type_Param": {
          "name": {
            "Path_Type": {
              "segments": [
                {
                  "Type_Name_Segment": {
                    "value": "T",
                    "type_params": []
                  }
                }
              ]
            }
          },
          "bounds": [
            {
              "Trait_Bound": {
                "trait_name": {
                  "Path_Type": {
                    "segments": [
                      {
                        "Type_Name_Segment": {
                          "value": "Display",
                          "type_params": []
                        }
                      }
                    ]
                  }
                }
              }
            },
            {
              "Trait_Bound": {
                "trait_name": {
                  "Path_Type": {
                    "segments": [
                      {
                        "Type_Name_Segment": {
                          "value": "Clone",
                          "type_params": []
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
    ],
    "aliased_type": {
      "Path_Type": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "Vec",
              "type_params": [
                {
                  "Path_Type": {
                    "segments": [
                      {
                        "Type_Name_Segment": {
                          "value": "T",
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
}
)";

// Trailing content (parser stops at semicolon)
constexpr auto k_with_trailing_content_should_succeed = true;
constexpr auto k_with_trailing_content_input = "type My_Int = I32; fn";
inline auto const k_with_trailing_content_expected = R"(
{
  "Type_Alias": {
    "name": "My_Int",
    "aliased_type": {
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
}
)";

// Error: Missing semicolon
constexpr auto k_missing_semicolon_should_succeed = false;
constexpr auto k_missing_semicolon_input = "type My_Type = I32";
inline auto const k_missing_semicolon_expected = "";

// Error: Missing equals sign
constexpr auto k_missing_equals_should_succeed = false;
constexpr auto k_missing_equals_input = "type My_Type I32;";
inline auto const k_missing_equals_expected = "";

// Error: Missing type name
constexpr auto k_missing_type_should_succeed = false;
constexpr auto k_missing_type_input = "type My_Type = ;";
inline auto const k_missing_type_expected = "";

// Error: Missing alias name
constexpr auto k_missing_name_should_succeed = false;
constexpr auto k_missing_name_input = "type = I32;";
inline auto const k_missing_name_expected = "";

}  // namespace

TEST_CASE("Parse Type_Alias", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Type_Alias_Params>({
          {"simple alias", k_simple_alias_input, k_simple_alias_expected, k_simple_alias_should_succeed},
          {"generic single parameter",
           k_generic_single_param_input,
           k_generic_single_param_expected,
           k_generic_single_param_should_succeed},
          {"generic multiple parameters",
           k_generic_multi_param_input,
           k_generic_multi_param_expected,
           k_generic_multi_param_should_succeed},
          {"qualified path", k_qualified_path_input, k_qualified_path_expected, k_qualified_path_should_succeed},
          {"nested generics", k_nested_generics_input, k_nested_generics_expected, k_nested_generics_should_succeed},
          {"type parameter with bounds",
           k_type_param_with_bounds_input,
           k_type_param_with_bounds_expected,
           k_type_param_with_bounds_should_succeed},
          {"multiple trait bounds",
           k_multiple_bounds_input,
           k_multiple_bounds_expected,
           k_multiple_bounds_should_succeed},
          {"with trailing content",
           k_with_trailing_content_input,
           k_with_trailing_content_expected,
           k_with_trailing_content_should_succeed},
          {"missing semicolon",
           k_missing_semicolon_input,
           k_missing_semicolon_expected,
           k_missing_semicolon_should_succeed},
          {"missing equals", k_missing_equals_input, k_missing_equals_expected, k_missing_equals_should_succeed},
          {"missing type", k_missing_type_input, k_missing_type_expected, k_missing_type_should_succeed},
          {"missing name", k_missing_name_input, k_missing_name_expected, k_missing_name_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
