#include "internal_rules.hpp"
#include "utils.hpp"
using life_lang::ast::Statement;

// PARSE_TEST generates check_parse() function and Statement_Params type
PARSE_TEST(Statement, statement)

namespace {
// === Valid Break Statements ===

// Simple break without value
constexpr auto k_simple_break_should_succeed = true;
constexpr auto k_simple_break_input = "break;";
inline auto const k_simple_break_expected = test_json::break_statement();

// Break with integer value
constexpr auto k_break_with_integer_should_succeed = true;
constexpr auto k_break_with_integer_input = "break 42;";
inline auto const k_break_with_integer_expected = test_json::break_statement(test_json::integer(42));

// Break with variable
constexpr auto k_break_with_var_should_succeed = true;
constexpr auto k_break_with_var_input = "break result;";
inline auto const k_break_with_var_expected = test_json::break_statement(test_json::var_name("result"));

// Break with expression
constexpr auto k_break_with_expression_should_succeed = true;
constexpr auto k_break_with_expression_input = "break x + 1;";
inline auto const k_break_with_expression_expected = R"({
  "Break_Statement": {
    "value": {
      "Binary_Expr": {
        "lhs": {
          "Var_Name": {
            "segments": [
              {
                "Var_Name_Segment": {
                  "type_params": [],
                  "value": "x"
                }
              }
            ]
          }
        },
        "op": "+",
        "rhs": {
          "Integer": {
            "value": "1"
          }
        }
      }
    }
  }
})";

// Break with function call
constexpr auto k_break_with_func_call_should_succeed = true;
constexpr auto k_break_with_func_call_input = "break calculate();";
inline auto const k_break_with_func_call_expected = fmt::format(
    R"({{
  "Break_Statement": {{
    "value": {{
      "Func_Call_Expr": {{
        "name": {},
        "params": []
      }}
    }}
  }}
}})",
    test_json::var_name("calculate")
);

// Break with string
constexpr auto k_break_with_string_should_succeed = true;
constexpr auto k_break_with_string_input = R"(break "done";)";
inline auto const k_break_with_string_expected = R"({
  "Break_Statement": {
    "value": {
      "String": {
        "value": "\"done\""
      }
    }
  }
})";

// Break with spaces
constexpr auto k_break_with_spaces_should_succeed = true;
constexpr auto k_break_with_spaces_input = "break   value   ;";
inline auto const k_break_with_spaces_expected = fmt::format(
    R"({{
  "Break_Statement": {{
    "value": {}
  }}
}})",
    test_json::var_name("value")
);

// === Invalid Break Statements ===

// Missing semicolon
constexpr auto k_missing_semicolon_should_succeed = false;
constexpr auto k_missing_semicolon_input = "break";

// Note: "break;;" actually succeeds - parses "break;" and leaves second ";" unparsed
// This is expected behavior for statement parsing

// Break is a keyword, can't use as variable name
constexpr auto k_break_as_var_should_succeed = false;
constexpr auto k_break_as_var_input = "break = 5;";

}  // namespace

TEST_CASE("Parse Break_Statement", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Statement_Params>({
          // Valid cases
          {"simple break", k_simple_break_input, k_simple_break_expected, k_simple_break_should_succeed},
          {"break with integer",
           k_break_with_integer_input,
           k_break_with_integer_expected,
           k_break_with_integer_should_succeed},
          {"break with variable", k_break_with_var_input, k_break_with_var_expected, k_break_with_var_should_succeed},
          {"break with expression",
           k_break_with_expression_input,
           k_break_with_expression_expected,
           k_break_with_expression_should_succeed},
          {"break with function call",
           k_break_with_func_call_input,
           k_break_with_func_call_expected,
           k_break_with_func_call_should_succeed},
          {"break with string",
           k_break_with_string_input,
           k_break_with_string_expected,
           k_break_with_string_should_succeed},
          {"break with spaces",
           k_break_with_spaces_input,
           k_break_with_spaces_expected,
           k_break_with_spaces_should_succeed},

          // Invalid cases
          {"invalid - missing semicolon", k_missing_semicolon_input, "", k_missing_semicolon_should_succeed},
          {"invalid - break as variable", k_break_as_var_input, "", k_break_as_var_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
