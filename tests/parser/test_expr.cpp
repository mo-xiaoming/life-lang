#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// Path expressions
constexpr auto k_simple_path_input = "hello";
constexpr auto k_dotted_path_input = "a.b.c";

// Integer literals
constexpr auto k_integer_input = "42";
constexpr auto k_zero_input = "0";

// String literals
constexpr auto k_string_input = R"("hello")";

// Function calls - no arguments
constexpr auto k_function_call_input = "hello()";
constexpr auto k_function_call_with_path_input = "hello.a.b()";
constexpr auto k_function_call_with_namespace_input = "A.B.hello()";

// Function calls - with arguments
constexpr auto k_function_call_with_args_input = "hello(a, b, c)";
constexpr auto k_function_call_with_path_args_input = "hello(a, b.c.world, c.world)";
constexpr auto k_nested_function_calls_input = "hello(A.B.a.d(), c.world(a))";

constexpr auto k_with_trailing_text_input = "hello )";

// Invalid cases
constexpr auto k_invalid_empty_input = "";
}  // namespace

TEST_CASE("Parse Expr", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          // Variable_Name expressions
          {"simple variable_name", k_simple_path_input,
           R"({"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}})",
           true, ""},
          {"dotted path", k_dotted_path_input,
           R"({"Field_Access_Expr": {"fieldName": "c", "object": {"Field_Access_Expr": {"fieldName": "b", "object": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "a"}}]}}}}}})",
           true, ""},

          // Integer literals
          {"integer", k_integer_input, R"({"Integer": {"value": "42"}})", true, ""},
          {"zero", k_zero_input, R"({"Integer": {"value": "0"}})", true, ""},

          // String literals
          {"string", k_string_input, R"({"String": {"value": "\"hello\""}})", true, ""},

          // Function calls - no arguments
          {"function call", k_function_call_input,
           R"({"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}}, "parameters": []}})",
           true, ""},
          {"function call with path", k_function_call_with_path_input,
           R"({"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "a"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "b"}}]}}, "parameters": []}})",
           true, ""},
          {"function call with namespace", k_function_call_with_namespace_input,
           R"({"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "A"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "B"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}}, "parameters": []}})",
           true, ""},

          // Function calls - with arguments
          {"function call with args", k_function_call_with_args_input,
           R"({"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}}, "parameters": [{"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "a"}}]}}, {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "b"}}]}}, {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "c"}}]}}]}})",
           true, ""},
          {"function call with path args", k_function_call_with_path_args_input,
           R"({"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}}, "parameters": [{"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "a"}}]}}, {"Field_Access_Expr": {"fieldName": "world", "object": {"Field_Access_Expr": {"fieldName": "c", "object": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "b"}}]}}}}}}, {"Field_Access_Expr": {"fieldName": "world", "object": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "c"}}]}}}}]}})",
           true, ""},

          // Function calls - nested
          {"nested function calls", k_nested_function_calls_input,
           R"({"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}}, "parameters": [{"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "A"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "B"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "a"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "d"}}]}}, "parameters": []}}, {"Function_Call_Expr": {"name": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "c"}}, {"Variable_Name_Segment": {"templateParameters": [], "value": "world"}}]}}, "parameters": [{"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "a"}}]}}]}}]}})",
           true, ""},

          {"with trailing text", k_with_trailing_text_input,
           R"({"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "hello"}}]}})",
           true, ")"},

          // Invalid cases
          {"invalid - empty", k_invalid_empty_input, R"({"Variable_Name": {"segments": []}})", false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}