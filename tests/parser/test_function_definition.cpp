#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Function_Definition;
using test_json::type_name;
using test_json::var_name;

PARSE_TEST(Function_Definition, function_definition)

namespace {
// Simple function definitions
constexpr auto k_empty_body_should_succeed = true;
constexpr auto k_empty_body_input = "fn hello(): Int {}";
inline auto const k_empty_body_expected = test_json::function_definition(
    test_json::function_declaration("hello", {}, type_name("Int")),
    test_json::block({})
);

// Functions with parameters
constexpr auto k_with_parameters_should_succeed = true;
constexpr auto k_with_parameters_input = "fn hello(a: Int, b: Double): Int {}";
inline auto const k_with_parameters_expected = test_json::function_definition(
    test_json::function_declaration(
        "hello",
        {test_json::function_parameter("a", type_name("Int")), test_json::function_parameter("b", type_name("Double"))},
        type_name("Int")
    ),
    test_json::block({})
);

// Functions with statements
constexpr auto k_with_return_should_succeed = true;
constexpr auto k_with_return_input = "fn hello(): Int {return world;}";
inline auto const k_with_return_expected = test_json::function_definition(
    test_json::function_declaration("hello", {}, type_name("Int")),
    test_json::block({test_json::return_statement(var_name("world"))})
);

constexpr auto k_with_statements_should_succeed = true;
constexpr auto k_with_statements_input = "fn hello(): Int {foo(); return 0;}";
inline auto const k_with_statements_expected = test_json::function_definition(
    test_json::function_declaration("hello", {}, type_name("Int")),
    test_json::block(
        {test_json::function_call_statement(test_json::function_call(var_name("foo"), {})),
         test_json::return_statement(test_json::integer("0"))}
    )
);

// Nested constructs
constexpr auto k_nested_block_should_succeed = true;
constexpr auto k_nested_block_input = R"(fn hello(a: Int): Int {
    hello();
    {
        return world;
    }
})";
inline auto const k_nested_block_expected = test_json::function_definition(
    test_json::function_declaration("hello", {test_json::function_parameter("a", type_name("Int"))}, type_name("Int")),
    test_json::block(
        {test_json::function_call_statement(test_json::function_call(var_name("hello"), {})),
         test_json::block({test_json::return_statement(var_name("world"))})}
    )
);

constexpr auto k_nested_function_should_succeed = true;
constexpr auto k_nested_function_input = R"(fn hello(): Int {
    fn world(): Int {
        return 0;
    }
    return world();
})";
inline auto const k_nested_function_expected = test_json::function_definition(
    test_json::function_declaration("hello", {}, type_name("Int")),
    test_json::block(
        {test_json::function_definition(
             test_json::function_declaration("world", {}, type_name("Int")),
             test_json::block({test_json::return_statement(test_json::integer("0"))})
         ),
         test_json::return_statement(test_json::function_call(var_name("world"), {}))}
    )
);

// Complex real-world examples
constexpr auto k_hello_world_should_succeed = true;
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
                        "template_parameters": []
                      }
                    },
                    {
                      "Type_Name_Segment": {
                        "value": "Array",
                        "template_parameters": [
                          {
                            "Type_Name": {
                              "segments": [
                                {
                                  "Type_Name_Segment": {
                                    "value": "Std",
                                    "template_parameters": []
                                  }
                                },
                                {
                                  "Type_Name_Segment": {
                                    "value": "String",
                                    "template_parameters": []
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
                  "template_parameters": []
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
                            "template_parameters": []
                          }
                        },
                        {
                          "Variable_Name_Segment": {
                            "value": "print",
                            "template_parameters": []
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
constexpr auto k_with_trailing_code_should_succeed = true;
constexpr auto k_with_trailing_code_input = "fn foo(): Int {} bar";
inline auto const k_with_trailing_code_expected =
    test_json::function_definition(test_json::function_declaration("foo", {}, type_name("Int")), test_json::block({}));

// Invalid cases
constexpr auto k_invalid_no_fn_keyword_should_succeed = false;
constexpr auto k_invalid_no_fn_keyword_input = "hello(): Int {}";
constexpr auto k_invalid_empty_should_succeed = false;
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
          {"empty body", k_empty_body_input, k_empty_body_expected, k_empty_body_should_succeed},

          // Functions with parameters
          {"with parameters", k_with_parameters_input, k_with_parameters_expected, k_with_parameters_should_succeed},

          // Functions with statements
          {"with return", k_with_return_input, k_with_return_expected, k_with_return_should_succeed},
          {"with statements", k_with_statements_input, k_with_statements_expected, k_with_statements_should_succeed},

          // Nested constructs
          {"nested block", k_nested_block_input, k_nested_block_expected, k_nested_block_should_succeed},
          {"nested function", k_nested_function_input, k_nested_function_expected, k_nested_function_should_succeed},

          // Complex real-world examples
          {"hello world", k_hello_world_input, k_hello_world_expected, k_hello_world_should_succeed},

          // Trailing content
          {"with trailing code",
           k_with_trailing_code_input,
           k_with_trailing_code_expected,
           k_with_trailing_code_should_succeed},

          // Invalid cases
          {"invalid - no fn keyword",
           k_invalid_no_fn_keyword_input,
           k_invalid_expected,
           k_invalid_no_fn_keyword_should_succeed},
          {"invalid - empty", k_invalid_empty_input, k_invalid_expected, k_invalid_empty_should_succeed},
      })
  );

  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
