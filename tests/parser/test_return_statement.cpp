#include "utils.hpp"

using life_lang::ast::Return_Statement;
using test_json::var_name;

PARSE_TEST(Return_Statement, return_statement)

namespace {
// Simple variable return
constexpr auto k_simple_variable_name_input = "return hello;";
inline auto const k_simple_variable_name_expected = fmt::format(
    R"({{
  "Return_Statement": {{
    "expr": {}
  }}
}})",
    var_name("hello")
);

// Field access
constexpr auto k_field_access_input = "return obj.field;";
inline auto const k_field_access_expected = fmt::format(
    R"({{
  "Return_Statement": {{
    "expr": {{
      "Field_Access_Expr": {{
        "fieldName": "field",
        "object": {}
      }}
    }}
  }}
}})",
    var_name("obj")
);

// Nested field access
constexpr auto k_nested_field_access_input = "return obj.inner.value;";
inline auto const k_nested_field_access_expected = fmt::format(
    R"({{
  "Return_Statement": {{
    "expr": {{
      "Field_Access_Expr": {{
        "fieldName": "value",
        "object": {{
          "Field_Access_Expr": {{
            "fieldName": "inner",
            "object": {}
          }}
        }}
      }}
    }}
  }}
}})",
    var_name("obj")
);

// Function call without arguments
constexpr auto k_function_call_input = "return foo();";
inline auto const k_function_call_expected = fmt::format(
    R"({{
  "Return_Statement": {{
    "expr": {{
      "Function_Call_Expr": {{
        "name": {},
        "parameters": []
      }}
    }}
  }}
}})",
    var_name("foo")
);

// Function call with argument
constexpr auto k_function_call_with_arg_input = "return foo(x);";
inline auto const k_function_call_with_arg_expected = fmt::format(
    R"({{
  "Return_Statement": {{
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

// Return integer literal
constexpr auto k_return_integer_input = "return 42;";
constexpr auto k_return_integer_expected = R"({
  "Return_Statement": {
    "expr": {
      "Integer": {
        "value": "42"
      }
    }
  }
})";

// Return string literal
constexpr auto k_return_string_input = R"(return "hello";)";
constexpr auto k_return_string_expected = R"({
  "Return_Statement": {
    "expr": {
      "String": {
        "value": "\"hello\""
      }
    }
  }
})";

// Trailing content
constexpr auto k_with_trailing_code_input = "return x; y";
inline auto const k_with_trailing_code_expected = fmt::format(
    R"({{
  "Return_Statement": {{
    "expr": {}
  }}
}})",
    var_name("x")
);

// Invalid cases
constexpr auto k_invalid_no_semicolon_input = "return x";
constexpr auto k_invalid_no_expression_input = "return;";
constexpr auto k_invalid_empty_input = "";
constexpr auto k_invalid_expected = "{}";
}  // namespace

TEST_CASE("Parse Return_Statement", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Return_Statement_Params>({
          {"simple variable_name", k_simple_variable_name_input, k_simple_variable_name_expected, true, ""},
          {"field access", k_field_access_input, k_field_access_expected, true, ""},
          {"nested field access", k_nested_field_access_input, k_nested_field_access_expected, true, ""},
          {"function call", k_function_call_input, k_function_call_expected, true, ""},
          {"function call with arg", k_function_call_with_arg_input, k_function_call_with_arg_expected, true, ""},
          {"return integer", k_return_integer_input, k_return_integer_expected, true, ""},
          {"return string", k_return_string_input, k_return_string_expected, true, ""},
          {"with trailing code", k_with_trailing_code_input, k_with_trailing_code_expected, true, "y"},
          {"invalid - no semicolon", k_invalid_no_semicolon_input, k_invalid_expected, false, ""},
          {"invalid - no expression", k_invalid_no_expression_input, k_invalid_expected, false, ";"},
          {"invalid - empty", k_invalid_empty_input, k_invalid_expected, false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}