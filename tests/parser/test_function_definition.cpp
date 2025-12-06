#include "utils.hpp"

using life_lang::ast::Function_Definition;
using test_json::type_name;
using test_json::var_name;

PARSE_TEST(Function_Definition, function_definition)

namespace {
// Simple function definitions
constexpr auto k_empty_body_input = "fn hello(): Int {}";
inline auto const k_empty_body_expected = fmt::format(
    R"({{
  "Function_Definition": {{
    "declaration": {{
      "Function_Declaration": {{
        "name": "hello",
        "parameters": [],
        "returnType": {}
      }}
    }},
    "body": {{
      "Block": {{
        "statements": []
      }}
    }}
  }}
}})",
    type_name("Int")
);

// Functions with parameters
constexpr auto k_with_parameters_input = "fn hello(a: Int, b: Double): Int {}";
inline auto const k_with_parameters_expected = fmt::format(
    R"({{
  "Function_Definition": {{
    "declaration": {{
      "Function_Declaration": {{
        "name": "hello",
        "parameters": [
          {{
            "Function_Parameter": {{
              "is_mut": false,
              "name": "a",
              "type": {}
            }}
          }},
          {{
            "Function_Parameter": {{
              "is_mut": false,
              "name": "b",
              "type": {}
            }}
          }}
        ],
        "returnType": {}
      }}
    }},
    "body": {{
      "Block": {{
        "statements": []
      }}
    }}
  }}
}})",
    type_name("Int"), type_name("Double"), type_name("Int")
);

// Functions with statements
constexpr auto k_with_return_input = "fn hello(): Int {return world;}";
inline auto const k_with_return_expected = fmt::format(
    R"({{
  "Function_Definition": {{
    "declaration": {{
      "Function_Declaration": {{
        "name": "hello",
        "parameters": [],
        "returnType": {}
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
    type_name("Int"), var_name("world")
);

constexpr auto k_with_statements_input = "fn hello(): Int {foo(); return 0;}";
inline auto const k_with_statements_expected = fmt::format(
    R"({{
  "Function_Definition": {{
    "declaration": {{
      "Function_Declaration": {{
        "name": "hello",
        "parameters": [],
        "returnType": {}
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
          }},
          {{
            "Return_Statement": {{
              "expr": {{
                "Integer": {{
                  "value": "0"
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    type_name("Int"), var_name("foo")
);

// Nested constructs
constexpr auto k_nested_block_input = R"(fn hello(a: Int): Int {
    hello();
    {
        return world;
    }
})";
inline auto const k_nested_block_expected = fmt::format(
    R"({{
  "Function_Definition": {{
    "declaration": {{
      "Function_Declaration": {{
        "name": "hello",
        "parameters": [
          {{
            "Function_Parameter": {{
              "is_mut": false,
              "name": "a",
              "type": {}
            }}
          }}
        ],
        "returnType": {}
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
          }},
          {{
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
        ]
      }}
    }}
  }}
}})",
    type_name("Int"), type_name("Int"), var_name("hello"), var_name("world")
);

constexpr auto k_nested_function_input = R"(fn hello(): Int {
    fn world(): Int {
        return 0;
    }
    return world();
})";
inline auto const k_nested_function_expected = fmt::format(
    R"({{
  "Function_Definition": {{
    "declaration": {{
      "Function_Declaration": {{
        "name": "hello",
        "parameters": [],
        "returnType": {}
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Function_Definition": {{
              "declaration": {{
                "Function_Declaration": {{
                  "name": "world",
                  "parameters": [],
                  "returnType": {}
                }}
              }},
              "body": {{
                "Block": {{
                  "statements": [
                    {{
                      "Return_Statement": {{
                        "expr": {{
                          "Integer": {{
                            "value": "0"
                          }}
                        }}
                      }}
                    }}
                  ]
                }}
              }}
            }}
          }},
          {{
            "Return_Statement": {{
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
    type_name("Int"), type_name("Int"), var_name("world")
);

// Complex real-world examples
constexpr auto k_hello_world_input = R"(fn main(args: Std.Array<Std.String>): I32 {
    Std.print("Hello, world!");
    return 0;
})";
inline auto const k_hello_world_expected = R"({
  "Function_Definition": {
    "declaration": {
      "Function_Declaration": {
        "name": "main",
        "parameters": [
          {
            "Function_Parameter": {
              "is_mut": false,
              "name": "args",
              "type": {
                "Type_Name": {
                  "segments": [
                    {
                      "Type_Name_Segment": {
                        "value": "Std",
                        "templateParameters": []
                      }
                    },
                    {
                      "Type_Name_Segment": {
                        "value": "Array",
                        "templateParameters": [
                          {
                            "Type_Name": {
                              "segments": [
                                {
                                  "Type_Name_Segment": {
                                    "value": "Std",
                                    "templateParameters": []
                                  }
                                },
                                {
                                  "Type_Name_Segment": {
                                    "value": "String",
                                    "templateParameters": []
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
        ],
        "returnType": {
          "Type_Name": {
            "segments": [
              {
                "Type_Name_Segment": {
                  "value": "I32",
                  "templateParameters": []
                }
              }
            ]
          }
        }
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
                        {
                          "Variable_Name_Segment": {
                            "value": "Std",
                            "templateParameters": []
                          }
                        },
                        {
                          "Variable_Name_Segment": {
                            "value": "print",
                            "templateParameters": []
                          }
                        }
                      ]
                    }
                  },
                  "parameters": [
                    {
                      "String": {
                        "value": "\"Hello, world!\""
                      }
                    }
                  ]
                }
              }
            }
          },
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
    }
  }
})";

// Trailing content
constexpr auto k_with_trailing_code_input = "fn foo(): Int {} bar";
inline auto const k_with_trailing_code_expected = fmt::format(
    R"({{
  "Function_Definition": {{
    "declaration": {{
      "Function_Declaration": {{
        "name": "foo",
        "parameters": [],
        "returnType": {}
      }}
    }},
    "body": {{
      "Block": {{
        "statements": []
      }}
    }}
  }}
}})",
    type_name("Int")
);

// Invalid cases
constexpr auto k_invalid_no_fn_keyword_input = "hello(): Int {}";
constexpr auto k_invalid_empty_input = "";
inline auto const k_invalid_expected = R"({
  "Function_Definition": {
    "declaration": {
      "Function_Declaration": {
        "name": "",
        "parameters": [],
        "returnType": {
          "Type_Name": {
            "segments": []
          }
        }
      }
    },
    "body": {
      "Block": {
        "statements": []
      }
    }
  }
})";

}  // namespace

TEST_CASE("Parse Function_Definition", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Function_Definition_Params>({
          // Simple function definitions
          {"empty body", k_empty_body_input, k_empty_body_expected, true, ""},

          // Functions with parameters
          {"with parameters", k_with_parameters_input, k_with_parameters_expected, true, ""},

          // Functions with statements
          {"with return", k_with_return_input, k_with_return_expected, true, ""},
          {"with statements", k_with_statements_input, k_with_statements_expected, true, ""},

          // Nested constructs
          {"nested block", k_nested_block_input, k_nested_block_expected, true, ""},
          {"nested function", k_nested_function_input, k_nested_function_expected, true, ""},

          // Complex real-world examples
          {"hello world", k_hello_world_input, k_hello_world_expected, true, ""},

          // Trailing content
          {"with trailing code", k_with_trailing_code_input, k_with_trailing_code_expected, true, "bar"},

          // Invalid cases
          {"invalid - no fn keyword", k_invalid_no_fn_keyword_input, k_invalid_expected, false, "hello(): Int {}"},
          {"invalid - empty", k_invalid_empty_input, k_invalid_expected, false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}
