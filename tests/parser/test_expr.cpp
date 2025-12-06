#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {

// Variable_Name expressions
constexpr auto k_simple_variable_name_should_succeed = true;
constexpr auto k_simple_variable_name_input = "hello";
constexpr auto k_simple_variable_name_expected =
    R"({"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}})";

constexpr auto k_dotted_path_should_succeed = true;
constexpr auto k_dotted_path_input = "a.b.c";
constexpr auto k_dotted_path_expected =
    R"({"Field_Access_Expr": {"fieldName": "c", "object": {"Field_Access_Expr": {"fieldName": "b", "object": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "a"}}]}}}}}})";

// Integer literals
constexpr auto k_integer_should_succeed = true;
constexpr auto k_integer_input = "42";
constexpr auto k_integer_expected = R"({"Integer": {"value": "42"}})";

constexpr auto k_zero_should_succeed = true;
constexpr auto k_zero_input = "0";
constexpr auto k_zero_expected = R"({"Integer": {"value": "0"}})";

// String literals
constexpr auto k_string_should_succeed = true;
constexpr auto k_string_input = R"("hello")";
constexpr auto k_string_expected = R"({"String": {"value": "\"hello\""}})";

// Function calls - no arguments
constexpr auto k_function_call_should_succeed = true;
constexpr auto k_function_call_input = "hello()";
constexpr auto k_function_call_expected =
    R"({"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}}, "parameters": []}})";

constexpr auto k_function_call_with_path_should_succeed = true;
constexpr auto k_function_call_with_path_input = "hello.a.b()";
constexpr auto k_function_call_with_path_expected =
    R"({"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "a"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "b"}}]}}, "parameters": []}})";

constexpr auto k_function_call_with_namespace_should_succeed = true;
constexpr auto k_function_call_with_namespace_input = "A.B.hello()";
constexpr auto k_function_call_with_namespace_expected =
    R"({"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "A"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "B"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}}, "parameters": []}})";

// Function calls - with arguments
constexpr auto k_function_call_with_args_should_succeed = true;
constexpr auto k_function_call_with_args_input = "hello(a, b, c)";
constexpr auto k_function_call_with_args_expected =
    R"({"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}}, "parameters": [{"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "a"}}]}}, {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "b"}}]}}, {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "c"}}]}}]}})";

constexpr auto k_function_call_with_path_args_should_succeed = true;
constexpr auto k_function_call_with_path_args_input = "hello(a, b.c.world, c.world)";
constexpr auto k_function_call_with_path_args_expected =
    R"({"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}}, "parameters": [{"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "a"}}]}}, {"Field_Access_Expr": {"fieldName": "world", "object": {"Field_Access_Expr": {"fieldName": "c", "object": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "b"}}]}}}}}}, {"Field_Access_Expr": {"fieldName": "world", "object": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "c"}}]}}}}]}})";

// Function calls - nested
constexpr auto k_nested_function_calls_should_succeed = true;
constexpr auto k_nested_function_calls_input = "hello(A.B.a.d(), c.world(a))";
constexpr auto k_nested_function_calls_expected =
    R"({"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}}, "parameters": [{"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "A"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "B"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "a"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "d"}}]}}, "parameters": []}}, {"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "c"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "world"}}]}}, "parameters": [{"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "a"}}]}}]}}]}})";

constexpr auto k_with_trailing_text_should_succeed = true;
constexpr auto k_with_trailing_text_input = "hello )";
constexpr auto k_with_trailing_text_expected =
    R"({"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}})";

// Invalid cases
constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";
constexpr auto k_invalid_empty_expected = R"({"Variable_Name": {"segments": []}})";

}  // namespace

TEST_CASE("Parse Expr", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          {"simple variable_name", k_simple_variable_name_input, k_simple_variable_name_expected,
           k_simple_variable_name_should_succeed},
          {"dotted path", k_dotted_path_input, k_dotted_path_expected, k_dotted_path_should_succeed},
          {"integer", k_integer_input, k_integer_expected, k_integer_should_succeed},
          {"zero", k_zero_input, k_zero_expected, k_zero_should_succeed},
          {"string", k_string_input, k_string_expected, k_string_should_succeed},
          {"function call", k_function_call_input, k_function_call_expected, k_function_call_should_succeed},
          {"function call with path", k_function_call_with_path_input, k_function_call_with_path_expected,
           k_function_call_with_path_should_succeed},
          {"function call with namespace", k_function_call_with_namespace_input,
           k_function_call_with_namespace_expected, k_function_call_with_namespace_should_succeed},
          {"function call with args", k_function_call_with_args_input, k_function_call_with_args_expected,
           k_function_call_with_args_should_succeed},
          {"function call with path args", k_function_call_with_path_args_input,
           k_function_call_with_path_args_expected, k_function_call_with_path_args_should_succeed},
          {"nested function calls", k_nested_function_calls_input, k_nested_function_calls_expected,
           k_nested_function_calls_should_succeed},
          {"with trailing text", k_with_trailing_text_input, k_with_trailing_text_expected,
           k_with_trailing_text_should_succeed},
          {"invalid - empty", k_invalid_empty_input, k_invalid_empty_expected, k_invalid_empty_should_succeed},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}