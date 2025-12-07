#include "internal_rules.hpp"
#include "utils.hpp"
using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// === Valid Tuple Pattern Tests ===

// Two element tuple pattern
constexpr auto k_tuple_two_elements_should_succeed = true;
constexpr auto k_tuple_two_elements_input = "for (a, b) in pairs { process(a, b); }";
inline auto const k_tuple_two_elements_expected = R"({
  "For_Expr": {
    "pattern": {
      "Tuple_Pattern": {
        "elements": [
          {"Simple_Pattern": {"name": "a"}},
          {"Simple_Pattern": {"name": "b"}}
        ]
      }
    },
    "iterator": {
      "Variable_Name": {
        "segments": [
          {"Variable_Name_Segment": {"template_parameters": [], "value": "pairs"}}
        ]
      }
    },
    "body": {
      "Block": {
        "statements": [
          {
            "Function_Call_Statement": {
              "expr": {
                "Function_Call_Expr": {
                  "name": {
                    "Variable_Name": {
                      "segments": [
                        {"Variable_Name_Segment": {"template_parameters": [], "value": "process"}}
                      ]
                    }
                  },
                  "parameters": [
                    {
                      "Variable_Name": {
                        "segments": [
                          {"Variable_Name_Segment": {"template_parameters": [], "value": "a"}}
                        ]
                      }
                    },
                    {
                      "Variable_Name": {
                        "segments": [
                          {"Variable_Name_Segment": {"template_parameters": [], "value": "b"}}
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

// Three element tuple pattern
constexpr auto k_tuple_three_elements_should_succeed = true;
constexpr auto k_tuple_three_elements_input = "for (x, y, z) in coords { move(x, y, z); }";
inline auto const k_tuple_three_elements_expected = R"({
  "For_Expr": {
    "pattern": {
      "Tuple_Pattern": {
        "elements": [
          {"Simple_Pattern": {"name": "x"}},
          {"Simple_Pattern": {"name": "y"}},
          {"Simple_Pattern": {"name": "z"}}
        ]
      }
    },
    "iterator": {
      "Variable_Name": {
        "segments": [
          {"Variable_Name_Segment": {"template_parameters": [], "value": "coords"}}
        ]
      }
    },
    "body": {
      "Block": {
        "statements": [
          {
            "Function_Call_Statement": {
              "expr": {
                "Function_Call_Expr": {
                  "name": {
                    "Variable_Name": {
                      "segments": [
                        {"Variable_Name_Segment": {"template_parameters": [], "value": "move"}}
                      ]
                    }
                  },
                  "parameters": [
                    {
                      "Variable_Name": {
                        "segments": [
                          {"Variable_Name_Segment": {"template_parameters": [], "value": "x"}}
                        ]
                      }
                    },
                    {
                      "Variable_Name": {
                        "segments": [
                          {"Variable_Name_Segment": {"template_parameters": [], "value": "y"}}
                        ]
                      }
                    },
                    {
                      "Variable_Name": {
                        "segments": [
                          {"Variable_Name_Segment": {"template_parameters": [], "value": "z"}}
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

// Single element tuple (edge case)
constexpr auto k_tuple_single_element_should_succeed = true;
constexpr auto k_tuple_single_element_input = "for (item) in items { process(item); }";
inline auto const k_tuple_single_element_expected = R"({
  "For_Expr": {
    "pattern": {
      "Tuple_Pattern": {
        "elements": [
          {"Simple_Pattern": {"name": "item"}}
        ]
      }
    },
    "iterator": {
      "Variable_Name": {
        "segments": [
          {"Variable_Name_Segment": {"template_parameters": [], "value": "items"}}
        ]
      }
    },
    "body": {
      "Block": {
        "statements": [
          {
            "Function_Call_Statement": {
              "expr": {
                "Function_Call_Expr": {
                  "name": {
                    "Variable_Name": {
                      "segments": [
                        {"Variable_Name_Segment": {"template_parameters": [], "value": "process"}}
                      ]
                    }
                  },
                  "parameters": [
                    {
                      "Variable_Name": {
                        "segments": [
                          {"Variable_Name_Segment": {"template_parameters": [], "value": "item"}}
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

// Nested tuple in tuple: for ((a, b), c) in ...
constexpr auto k_nested_tuple_in_tuple_should_succeed = true;
constexpr auto k_nested_tuple_in_tuple_input = "for ((a, b), c) in nested { print(a, b, c); }";
inline auto const k_nested_tuple_in_tuple_expected = R"({
  "For_Expr": {
    "pattern": {
      "Tuple_Pattern": {
        "elements": [
          {
            "Tuple_Pattern": {
              "elements": [
                {"Simple_Pattern": {"name": "a"}},
                {"Simple_Pattern": {"name": "b"}}
              ]
            }
          },
          {"Simple_Pattern": {"name": "c"}}
        ]
      }
    },
    "iterator": {
      "Variable_Name": {
        "segments": [
          {"Variable_Name_Segment": {"template_parameters": [], "value": "nested"}}
        ]
      }
    },
    "body": {
      "Block": {
        "statements": [
          {
            "Function_Call_Statement": {
              "expr": {
                "Function_Call_Expr": {
                  "name": {
                    "Variable_Name": {
                      "segments": [
                        {"Variable_Name_Segment": {"template_parameters": [], "value": "print"}}
                      ]
                    }
                  },
                  "parameters": [
                    {
                      "Variable_Name": {
                        "segments": [
                          {"Variable_Name_Segment": {"template_parameters": [], "value": "a"}}
                        ]
                      }
                    },
                    {
                      "Variable_Name": {
                        "segments": [
                          {"Variable_Name_Segment": {"template_parameters": [], "value": "b"}}
                        ]
                      }
                    },
                    {
                      "Variable_Name": {
                        "segments": [
                          {"Variable_Name_Segment": {"template_parameters": [], "value": "c"}}
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

// Nested struct in tuple: for (Point { x, y }, z) in ...
constexpr auto k_nested_struct_in_tuple_should_succeed = true;
constexpr auto k_nested_struct_in_tuple_input = "for (Point { x, y }, z) in data { use(x, y, z); }";
inline auto const k_nested_struct_in_tuple_expected = R"({
  "For_Expr": {
    "pattern": {
      "Tuple_Pattern": {
        "elements": [
          {
            "Struct_Pattern": {
              "type_name": {
                "Type_Name": {
                  "segments": [
                    {"Type_Name_Segment": {"template_parameters": [], "value": "Point"}}
                  ]
                }
              },
              "fields": [
                {"Simple_Pattern": {"name": "x"}},
                {"Simple_Pattern": {"name": "y"}}
              ]
            }
          },
          {"Simple_Pattern": {"name": "z"}}
        ]
      }
    },
    "iterator": {
      "Variable_Name": {
        "segments": [
          {"Variable_Name_Segment": {"template_parameters": [], "value": "data"}}
        ]
      }
    },
    "body": {
      "Block": {
        "statements": [
          {
            "Function_Call_Statement": {
              "expr": {
                "Function_Call_Expr": {
                  "name": {
                    "Variable_Name": {
                      "segments": [
                        {"Variable_Name_Segment": {"template_parameters": [], "value": "use"}}
                      ]
                    }
                  },
                  "parameters": [
                    {
                      "Variable_Name": {
                        "segments": [
                          {"Variable_Name_Segment": {"template_parameters": [], "value": "x"}}
                        ]
                      }
                    },
                    {
                      "Variable_Name": {
                        "segments": [
                          {"Variable_Name_Segment": {"template_parameters": [], "value": "y"}}
                        ]
                      }
                    },
                    {
                      "Variable_Name": {
                        "segments": [
                          {"Variable_Name_Segment": {"template_parameters": [], "value": "z"}}
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

// === Invalid Tuple Pattern Tests ===

// Empty tuple (invalid)
constexpr auto k_empty_tuple_should_succeed = false;
constexpr auto k_empty_tuple_input = "for () in items { }";

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
          {"two element tuple", k_tuple_two_elements_input, k_tuple_two_elements_expected,
           k_tuple_two_elements_should_succeed},
          {"three element tuple", k_tuple_three_elements_input, k_tuple_three_elements_expected,
           k_tuple_three_elements_should_succeed},
          {"single element tuple", k_tuple_single_element_input, k_tuple_single_element_expected,
           k_tuple_single_element_should_succeed},
          {"nested tuple in tuple", k_nested_tuple_in_tuple_input, k_nested_tuple_in_tuple_expected,
           k_nested_tuple_in_tuple_should_succeed},
          {"nested struct in tuple", k_nested_struct_in_tuple_input, k_nested_struct_in_tuple_expected,
           k_nested_struct_in_tuple_should_succeed},
          {"empty tuple (invalid)", k_empty_tuple_input, "", k_empty_tuple_should_succeed},
          {"missing closing paren", k_missing_closing_paren_input, "", k_missing_closing_paren_should_succeed},
          {"missing opening paren", k_missing_opening_paren_input, "", k_missing_opening_paren_should_succeed},
          {"trailing comma", k_trailing_comma_input, "", k_trailing_comma_should_succeed},
          {"leading comma", k_leading_comma_input, "", k_leading_comma_should_succeed},
          {"double comma", k_double_comma_input, "", k_double_comma_should_succeed},
          {"invalid identifier", k_invalid_identifier_input, "", k_invalid_identifier_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) { check_parse(params); }
}
