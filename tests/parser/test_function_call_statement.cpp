#include "utils.hpp"

using life_lang::ast::Function_Call_Statement;

PARSE_TEST(Function_Call_Statement, function_call_statement)

namespace {
// Helper to create Variable_Name JSON with single segment
inline auto var_name(char const* a_name) {
  return fmt::format(
      R"({{
    "Variable_Name": {{
      "segments": [
        {{
          "Variable_Name_Segment": {{
            "value": "{}",
            "templateParameters": []
          }}
        }}
      ]
    }}
  }})",
      a_name
  );
}

// No arguments
constexpr auto k_no_arguments_input = "hello();";
inline auto const k_no_arguments_expected = fmt::format(
    R"({{
  "Function_Call_Statement": {{
    "expr": {{
      "Function_Call_Expr": {{
        "name": {},
        "parameters": []
      }}
    }}
  }}
}})",
    var_name("hello")
);

// One argument
constexpr auto k_one_argument_input = "foo(x);";
inline auto const k_one_argument_expected = fmt::format(
    R"({{
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
}})",
    var_name("foo"), var_name("x")
);

// Two arguments
constexpr auto k_two_arguments_input = "foo(x, y);";
inline auto const k_two_arguments_expected = fmt::format(
    R"({{
  "Function_Call_Statement": {{
    "expr": {{
      "Function_Call_Expr": {{
        "name": {},
        "parameters": [
          {},
          {}
        ]
      }}
    }}
  }}
}})",
    var_name("foo"), var_name("x"), var_name("y")
);

// Dotted path argument (field access)
constexpr auto k_dotted_path_args_input = "foo(a, b.c);";
inline auto const k_dotted_path_args_expected = fmt::format(
    R"({{
  "Function_Call_Statement": {{
    "expr": {{
      "Function_Call_Expr": {{
        "name": {},
        "parameters": [
          {},
          {{
            "Field_Access_Expr": {{
              "fieldName": "c",
              "object": {}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    var_name("foo"), var_name("a"), var_name("b")
);

// Qualified namespace call
constexpr auto k_namespace_call_input = "Std.print(x);";
constexpr auto k_namespace_call_expected = R"({
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
            "Variable_Name": {
              "segments": [
                {
                  "Variable_Name_Segment": {
                    "value": "x",
                    "templateParameters": []
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

// Integer argument
constexpr auto k_integer_argument_input = "foo(42);";
constexpr auto k_integer_argument_expected = R"({
  "Function_Call_Statement": {
    "expr": {
      "Function_Call_Expr": {
        "name": {
          "Variable_Name": {
            "segments": [
              {
                "Variable_Name_Segment": {
                  "value": "foo",
                  "templateParameters": []
                }
              }
            ]
          }
        },
        "parameters": [
          {
            "Integer": {
              "value": "42"
            }
          }
        ]
      }
    }
  }
})";

// String argument
constexpr auto k_string_argument_input = R"(print("hello");)";
constexpr auto k_string_argument_expected = R"({
  "Function_Call_Statement": {
    "expr": {
      "Function_Call_Expr": {
        "name": {
          "Variable_Name": {
            "segments": [
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
              "value": "\"hello\""
            }
          }
        ]
      }
    }
  }
})";

// Trailing content
constexpr auto k_with_trailing_code_input = "foo(); bar";
constexpr auto k_with_trailing_code_expected = R"({
  "Function_Call_Statement": {
    "expr": {
      "Function_Call_Expr": {
        "name": {
          "Variable_Name": {
            "segments": [
              {
                "Variable_Name_Segment": {
                  "value": "foo",
                  "templateParameters": []
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

// Invalid cases - using empty JSON as placeholder (won't be compared since should_succeed=false)
constexpr auto k_invalid_no_semicolon_input = "foo()";
constexpr auto k_invalid_no_parentheses_input = "foo;";
constexpr auto k_invalid_empty_input = "";
constexpr auto k_invalid_expected = "{}";

}  // namespace

TEST_CASE("Parse Function_Call_Statement", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Function_Call_Statement_Params>({
          // No arguments
          {"no arguments", k_no_arguments_input, k_no_arguments_expected, true, ""},

          // With arguments
          {"one argument", k_one_argument_input, k_one_argument_expected, true, ""},
          {"two arguments", k_two_arguments_input, k_two_arguments_expected, true, ""},
          {"dotted path args", k_dotted_path_args_input, k_dotted_path_args_expected, true, ""},

          // Qualified paths
          {"namespace call", k_namespace_call_input, k_namespace_call_expected, true, ""},

          // Different argument types
          {"integer argument", k_integer_argument_input, k_integer_argument_expected, true, ""},
          {"string argument", k_string_argument_input, k_string_argument_expected, true, ""},

          // Trailing content
          {"with trailing code", k_with_trailing_code_input, k_with_trailing_code_expected, true, "bar"},

          // Invalid cases
          {"invalid - no semicolon", k_invalid_no_semicolon_input, k_invalid_expected, false, ""},
          {"invalid - no parentheses", k_invalid_no_parentheses_input, k_invalid_expected, false, "foo;"},
          {"invalid - empty", k_invalid_empty_input, k_invalid_expected, false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}