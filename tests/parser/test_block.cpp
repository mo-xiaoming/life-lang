#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Block;
using test_json::var_name;

PARSE_TEST(Block, block)

namespace {
// Empty block
constexpr auto k_empty_block_should_succeed = true;
constexpr auto k_empty_block_input = "{}";
inline auto const k_empty_block_expected = R"({
  "Block": {
    "statements": []
  }
})";

// Single statement blocks
constexpr auto k_single_return_should_succeed = true;
constexpr auto k_single_return_input = "{return hello;}";
inline auto const k_single_return_expected = fmt::format(
    R"({{
  "Block": {{
    "statements": [
      {{
        "Return_Statement": {{
          "expr": {}
        }}
      }}
    ]
  }}
}})",
    var_name("hello")
);

constexpr auto k_single_function_call_should_succeed = true;
constexpr auto k_single_function_call_input = "{foo();}";
inline auto const k_single_function_call_expected = fmt::format(
    R"({{
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
}})",
    var_name("foo")
);

// Multiple statements
constexpr auto k_two_statements_should_succeed = true;
constexpr auto k_two_statements_input = "{hello.a(); return world;}";
inline auto const k_two_statements_expected = fmt::format(
    R"({{
  "Block": {{
    "statements": [
      {{
        "Function_Call_Statement": {{
          "expr": {{
            "Function_Call_Expr": {{
              "name": {{
                "Variable_Name": {{
                  "segments": [
                    {{
                      "Variable_Name_Segment": {{
                        "value": "hello",
                        "templateParameters": []
                      }}
                    }},
                    {{
                      "Variable_Name_Segment": {{
                        "value": "a",
                        "templateParameters": []
                      }}
                    }}
                  ]
                }}
              }},
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
}})",
    var_name("world")
);

constexpr auto k_multiple_statements_should_succeed = true;
constexpr auto k_multiple_statements_input = "{foo(); bar(); return 0;}";
inline auto const k_multiple_statements_expected = fmt::format(
    R"({{
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
          "expr": {{
            "Integer": {{
              "value": "0"
            }}
          }}
        }}
      }}
    ]
  }}
}})",
    var_name("foo"), var_name("bar")
);

// Nested blocks
constexpr auto k_nested_block_should_succeed = true;
constexpr auto k_nested_block_input = "{hello(b); {return world;}}";
inline auto const k_nested_block_expected = fmt::format(
    R"({{
  "Block": {{
    "statements": [
      {{
        "Function_Call_Statement": {{
          "expr": {{
            "Function_Call_Expr": {{
              "name": {},
              "parameters": [
                {}
              ]
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
}})",
    var_name("hello"), var_name("b"), var_name("world")
);

// Whitespace handling
constexpr auto k_with_spaces_should_succeed = true;
constexpr auto k_with_spaces_input = "{  foo(  )  ;  }";
inline auto const k_with_spaces_expected = fmt::format(
    R"({{
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
}})",
    var_name("foo")
);

// Trailing content
constexpr auto k_with_trailing_code_should_succeed = true;
constexpr auto k_with_trailing_code_input = "{return x;} y";
inline auto const k_with_trailing_code_expected = fmt::format(
    R"({{
  "Block": {{
    "statements": [
      {{
        "Return_Statement": {{
          "expr": {}
        }}
      }}
    ]
  }}
}})",
    var_name("x")
);

// Invalid cases
constexpr auto k_invalid_no_closing_brace_should_succeed = false;
constexpr auto k_invalid_no_closing_brace_input = "{return x;";
constexpr auto k_invalid_no_opening_brace_should_succeed = false;
constexpr auto k_invalid_no_opening_brace_input = "return x;}";
constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";
inline auto const k_invalid_expected = R"({
  "Block": {
    "statements": []
  }
})";
}  // namespace

TEST_CASE("Parse Block", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Block_Params>({
          // Empty block
          {"empty block", k_empty_block_input, k_empty_block_expected, k_empty_block_should_succeed},

          // Single statement blocks
          {"single return", k_single_return_input, k_single_return_expected, k_single_return_should_succeed},
          {"single function call", k_single_function_call_input, k_single_function_call_expected,
           k_single_function_call_should_succeed},

          // Multiple statements
          {"two statements", k_two_statements_input, k_two_statements_expected, k_two_statements_should_succeed},
          {"multiple statements", k_multiple_statements_input, k_multiple_statements_expected,
           k_multiple_statements_should_succeed},

          // Nested blocks
          {"nested block", k_nested_block_input, k_nested_block_expected, k_nested_block_should_succeed},

          // Whitespace handling
          {"with spaces", k_with_spaces_input, k_with_spaces_expected, k_with_spaces_should_succeed},

          // Trailing content
          {"with trailing code", k_with_trailing_code_input, k_with_trailing_code_expected,
           k_with_trailing_code_should_succeed},

          // Invalid cases
          {"invalid - no closing brace", k_invalid_no_closing_brace_input, k_invalid_expected,
           k_invalid_no_closing_brace_should_succeed},
          {"invalid - no opening brace", k_invalid_no_opening_brace_input, k_invalid_expected,
           k_invalid_no_opening_brace_should_succeed},
          {"invalid - empty", k_invalid_empty_input, k_invalid_expected, k_invalid_empty_should_succeed},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}