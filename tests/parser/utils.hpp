// Parser Test Utilities
//
// STANDARD TEST PATTERN
// =====================
// All parser test files should follow this consistent pattern:
//
// 1. Use PARSE_TEST macro to generate test infrastructure:
//    PARSE_TEST(Ast_Type, parser_func_name)
//
// 2. Define constants in namespace{} with this ordering:
//    constexpr auto k_test_name_should_succeed = true/false;  // FIRST
//    constexpr auto k_test_name_input = "code";
//    inline auto const k_test_name_expected = R"(json)";      // LAST
//
// 3. Use test array with constant references:
//    {"test name", k_test_name_input, k_test_name_expected, k_test_name_should_succeed}
//
// EXAMPLE:
//   PARSE_TEST(Func_Call_Statement, function_call_statement)
//
//   namespace {
//   constexpr auto k_no_args_should_succeed = true;
//   constexpr auto k_no_args_input = "foo();";
//   inline auto const k_no_args_expected = R"({
//     "Func_Call_Statement": {
//       "expr": { ... }
//     }
//   })";
//   }  // namespace
//
//   TEST_CASE("Parse Func_Call_Statement", "[parser]") {
//     auto const params = GENERATE(
//       Catch::Generators::values<Func_Call_Statement_Params>({
//         {"no arguments", k_no_args_input, k_no_args_expected, k_no_args_should_succeed},
//       })
//     );
//     DYNAMIC_SECTION(params.name) { check_parse(params); }
//   }
//
// SPECIAL CASES:
// - If expected JSON is too complex (e.g., variant wrapping), you can skip JSON comparison
//   and only verify parsing success/failure. See test_field_access.cpp for example.
//
// The expected field accepts std::variant<AST_Type, std::string>
// JSON strings are automatically parsed and normalized for comparison

#include <fmt/core.h>
#include <fmt/format.h>

#include <boost/fusion/include/io.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <nlohmann/json.hpp>
#include <string_view>
#include <variant>

// Common JSON building helpers to reduce duplication
namespace test_json {

// Variable name with single segment (no templates)
inline std::string var_name(std::string_view a_name) {
  return fmt::format(
      R"({{
    "Var_Name": {{
      "segments": [
        {{
          "Var_Name_Segment": {{
            "value": "{}",
            "type_params": []
          }}
        }}
      ]
    }}
  }})",
      a_name
  );
}

// Variable name with multiple path segments (no templates)
inline std::string var_name_path(std::vector<std::string_view> const& a_segments) {
  std::string segments_json;
  for (size_t i = 0; i < a_segments.size(); ++i) {
    if (i > 0) {
      segments_json += ",";
    }
    segments_json += fmt::format(R"({{"Var_Name_Segment":{{"value":"{}","type_params":[]}}}})", a_segments[i]);
  }
  return fmt::format(R"({{"Var_Name":{{"segments":[{}]}}}})", segments_json);
}

// Type name with single segment (no templates)
inline std::string type_name(std::string_view a_name) {
  return fmt::format(
      R"({{
    "Type_Name": {{
      "segments": [
        {{
          "Type_Name_Segment": {{
            "value": "{}",
            "type_params": []
          }}
        }}
      ]
    }}
  }})",
      a_name
  );
}

// Type name with two segments (convenience overload for qualified names)
inline std::string type_name(std::string_view a_seg1, std::string_view a_seg2) {
  return fmt::format(
      R"({{
    "Type_Name": {{
      "segments": [
        {{
          "Type_Name_Segment": {{
            "value": "{}",
            "type_params": []
          }}
        }},
        {{
          "Type_Name_Segment": {{
            "value": "{}",
            "type_params": []
          }}
        }}
      ]
    }}
  }})",
      a_seg1,
      a_seg2
  );
}

// Type name with multiple path segments (no templates)
inline std::string type_name_path(std::vector<std::string_view> const& a_segments) {
  std::string segments_json;
  for (size_t i = 0; i < a_segments.size(); ++i) {
    if (i > 0) {
      segments_json += ",";
    }
    segments_json += fmt::format(R"({{"Type_Name_Segment":{{"value":"{}","type_params":[]}}}})", a_segments[i]);
  }
  return fmt::format(R"({{"Type_Name":{{"segments":[{}]}}}})", segments_json);
}

// Type parameter (wraps a Type_Name in Type_Param for generic declarations)
inline std::string type_param(std::string_view a_type_name_json) {
  return fmt::format(R"({{"Type_Param":{{"name":{}}}}})", a_type_name_json);
}

// Integer literal
inline std::string integer(std::string_view a_value) {
  return fmt::format(R"({{"Integer":{{"value":"{}"}}}})", a_value);
}

// String literal
inline std::string string(std::string_view a_value) {
  return fmt::format(R"({{"String":{{"value":"{}"}}}})", a_value);
}

// Character literal
inline std::string char_literal(std::string_view a_value) {
  return fmt::format(R"({{"Char":{{"value":"{}"}}}})", a_value);
}

// Wildcard pattern
inline std::string wildcard_pattern() {
  return R"({"Wildcard_Pattern":{}})";
}

// Literal pattern (wraps an expression)
inline std::string literal_pattern(std::string_view a_expr_json) {
  return fmt::format(R"({{"Literal_Pattern":{{"value":{}}}}})", a_expr_json);
}

// Simple pattern (identifier)
inline std::string simple_pattern(std::string_view a_name) {
  return fmt::format(R"({{"Simple_Pattern":{{"name":"{}"}}}})", a_name);
}

// Tuple pattern
inline std::string tuple_pattern(std::vector<std::string> const& a_elements) {
  std::string elements = "[";
  for (size_t i = 0; i < a_elements.size(); ++i) {
    if (i > 0) {
      elements += ",";
    }
    elements += a_elements[i];
  }
  elements += "]";
  return fmt::format(R"({{"Tuple_Pattern":{{"elements":{}}}}})", elements);
}

// Field pattern (name: pattern)
inline std::string field_pattern(std::string_view a_name, std::string_view a_pattern) {
  return fmt::format(R"({{"Field_Pattern":{{"name":"{}","pattern":{}}}}})", a_name, a_pattern);
}

// Struct pattern
inline std::string struct_pattern(std::string_view a_type_name, std::vector<std::string> const& a_fields) {
  std::string fields = "[";
  for (size_t i = 0; i < a_fields.size(); ++i) {
    if (i > 0) {
      fields += ",";
    }
    fields += a_fields[i];
  }
  fields += "]";
  return fmt::format(R"({{"Struct_Pattern":{{"type_name":{},"fields":{}}}}})", a_type_name, fields);
}

// Match arm without guard
inline std::string match_arm(std::string_view a_pattern, std::string_view a_result) {
  return fmt::format(R"({{"Match_Arm":{{"pattern":{},"result":{}}}}})", a_pattern, a_result);
}

// Match arm with guard
inline std::string
match_arm_with_guard(std::string_view a_pattern, std::string_view a_guard, std::string_view a_result) {
  return fmt::format(R"({{"Match_Arm":{{"pattern":{},"guard":{},"result":{}}}}})", a_pattern, a_guard, a_result);
}

// Binary expression
inline std::string binary_expr(std::string_view a_op, std::string_view a_lhs, std::string_view a_rhs) {
  return fmt::format(R"({{"Binary_Expr":{{"lhs":{},"op":"{}","rhs":{}}}}})", a_lhs, a_op, a_rhs);
}

// Function call expression
inline std::string function_call(std::string_view a_name, std::vector<std::string> const& a_args) {
  std::string params = "[";
  for (size_t i = 0; i < a_args.size(); ++i) {
    if (i > 0) {
      params += ",";
    }
    params += a_args[i];
  }
  params += "]";
  return fmt::format(R"({{"Func_Call_Expr":{{"name":{},"params":{}}}}})", a_name, params);
}

// Match expression
inline std::string match_expr(std::string_view a_scrutinee, std::vector<std::string> const& a_arms) {
  std::string arms = "[";
  for (size_t i = 0; i < a_arms.size(); ++i) {
    if (i > 0) {
      arms += ",";
    }
    arms += a_arms[i];
  }
  arms += "]";
  return fmt::format(R"({{"Match_Expr":{{"scrutinee":{},"arms":{}}}}})", a_scrutinee, arms);
}

// Field access expression
inline std::string field_access(std::string_view a_object, std::string_view a_field_name) {
  return fmt::format(R"({{"Field_Access_Expr":{{"object":{},"field_name":"{}"}}}})", a_object, a_field_name);
}

// Overload for integer that accepts int directly
inline std::string integer(int a_value) {
  return fmt::format(R"({{"Integer":{{"value":"{}"}}}})", a_value);
}

// Block with statements
inline std::string block(std::vector<std::string> const& a_statements) {
  std::string statements_json;
  for (size_t i = 0; i < a_statements.size(); ++i) {
    if (i > 0) {
      statements_json += ",";
    }
    statements_json += a_statements[i];
  }
  return fmt::format(R"({{"Block":{{"statements":[{}]}}}})", statements_json);
}

// Return statement
inline std::string return_statement(std::string_view a_expr) {
  return fmt::format(R"({{"Return_Statement":{{"expr":{}}}}})", a_expr);
}

// Function call statement
inline std::string function_call_statement(std::string_view a_expr) {
  return fmt::format(R"({{"Func_Call_Statement":{{"expr":{}}}}})", a_expr);
}

// Assignment expression
inline std::string assignment_expr(std::string_view a_target, std::string_view a_value) {
  return fmt::format(R"({{"Assignment_Expr":{{"target":{},"value":{}}}}})", a_target, a_value);
}

// Let statement
inline std::string let_statement(
    std::string_view a_pattern,
    std::string_view a_value,
    bool a_is_mut = false,
    std::string_view a_type = "null"
) {
  return fmt::format(
      R"({{"Let_Statement":{{"is_mut":{},"pattern":{},"type":{},"value":{}}}}})",
      a_is_mut ? "true" : "false",
      a_pattern,
      a_type,
      a_value
  );
}

// If expression without else
inline std::string if_expr(std::string_view a_condition, std::string_view a_then_block) {
  return fmt::format(R"({{"If_Expr":{{"condition":{},"then_block":{}}}}})", a_condition, a_then_block);
}

// If expression with else
inline std::string
if_else_expr(std::string_view a_condition, std::string_view a_then_block, std::string_view a_else_block) {
  return fmt::format(
      R"({{"If_Expr":{{"condition":{},"then_block":{},"else_block":{}}}}})",
      a_condition,
      a_then_block,
      a_else_block
  );
}

// While expression
inline std::string while_expr(std::string_view a_condition, std::string_view a_body) {
  return fmt::format(R"({{"While_Expr":{{"condition":{},"body":{}}}}})", a_condition, a_body);
}

// Range expression
inline std::string range_expr(std::string_view a_start, std::string_view a_end, bool a_inclusive) {
  return fmt::format(
      R"({{"Range_Expr":{{"start":{},"end":{},"inclusive":{}}}}})",
      a_start,
      a_end,
      a_inclusive ? "true" : "false"
  );
}

// For expression
inline std::string for_expr(std::string_view a_pattern, std::string_view a_iterator, std::string_view a_body) {
  return fmt::format(R"({{"For_Expr":{{"pattern":{},"iterator":{},"body":{}}}}})", a_pattern, a_iterator, a_body);
}

// Break statement (with optional value)
inline std::string break_statement(std::string_view a_value = "null") {
  return fmt::format(R"({{"Break_Statement":{{"value":{}}}}})", a_value);
}

// Continue statement
inline std::string continue_statement() {
  return R"({"Continue_Statement":null})";
}

// Unary expression
inline std::string unary_expr(std::string_view a_op, std::string_view a_operand) {
  return fmt::format(R"({{"Unary_Expr":{{"op":"{}","operand":{}}}}})", a_op, a_operand);
}

// Struct field
inline std::string struct_field(std::string_view a_name, std::string_view a_type) {
  return fmt::format(R"({{"Struct_Field":{{"name":"{}","type":{}}}}})", a_name, a_type);
}

// Struct definition
inline std::string struct_def(std::string_view a_name, std::vector<std::string> const& a_fields) {
  std::string fields = "[";
  for (size_t i = 0; i < a_fields.size(); ++i) {
    if (i > 0) {
      fields += ",";
    }
    fields += a_fields[i];
  }
  fields += "]";
  return fmt::format(R"({{"Struct_Def":{{"fields":{},"name":"{}"}}}})", fields, a_name);
}

// Function parameter
inline std::string function_parameter(std::string_view a_name, std::string_view a_type, bool a_is_mut = false) {
  return fmt::format(
      R"({{"Func_Param":{{"is_mut":{},"name":"{}","type":{}}}}})",
      a_is_mut ? "true" : "false",
      a_name,
      a_type
  );
}

// Function declaration
inline std::string func_decl(
    std::string_view a_name,
    std::vector<std::string> const& a_type_params,
    std::vector<std::string> const& a_params,
    std::string_view a_return_type
) {
  std::string type_params = "[";
  for (size_t i = 0; i < a_type_params.size(); ++i) {
    if (i > 0) {
      type_params += ",";
    }
    type_params += a_type_params[i];
  }
  type_params += "]";
  std::string params = "[";
  for (size_t i = 0; i < a_params.size(); ++i) {
    if (i > 0) {
      params += ",";
    }
    params += a_params[i];
  }
  params += "]";
  return fmt::format(
      R"({{"Func_Decl":{{"name":"{}","params":{},"return_type":{},"type_params":{}}}}})",
      a_name,
      params,
      a_return_type,
      type_params
  );
}

// Function definition
inline std::string func_def(std::string_view a_declaration, std::string_view a_body) {
  return fmt::format(R"({{"Func_Def":{{"decl":{},"body":{}}}}})", a_declaration, a_body);
}

}  // namespace test_json

// Helper to get expected JSON - either from AST object or JSON string
template <typename Ast_Type>
std::string get_expected_json(std::variant<Ast_Type, std::string> const& a_expected, int a_indent) {
  if (std::holds_alternative<Ast_Type>(a_expected)) {
    return to_json_string(std::get<Ast_Type>(a_expected), a_indent);
  }
  // Parse and re-dump JSON string to normalize formatting
  auto json = nlohmann::json::parse(std::get<std::string>(a_expected));
  return json.dump(a_indent);
}

#define PARSE_TEST(AstType, fn_name)                                                                           \
  namespace {                                                                                                  \
  using AstType##_Params = Parse_Test_Params<AstType>;                                                         \
  void check_parse(AstType##_Params const& params) {                                                           \
    auto input_start = params.input.cbegin();                                                                  \
    auto const input_end = params.input.cend();                                                                \
    auto const got = life_lang::internal::parse_##fn_name(input_start, input_end);                             \
    CHECK(params.should_succeed == bool(got));                                                                 \
    if (params.should_succeed != bool(got)) {                                                                  \
      if (got) {                                                                                               \
        UNSCOPED_INFO(to_json_string(*got, 2));                                                                \
      } else {                                                                                                 \
        UNSCOPED_INFO(got.error());                                                                            \
      }                                                                                                        \
    }                                                                                                          \
    if (got) {                                                                                                 \
      bool const has_expected =                                                                                \
          std::holds_alternative<AstType>(params.expected) || !std::get<std::string>(params.expected).empty(); \
      if (has_expected) {                                                                                      \
        auto const expected_json = get_expected_json(params.expected, 2);                                      \
        auto const actual_json = to_json_string(*got, 2);                                                      \
        CHECK(expected_json == actual_json);                                                                   \
      }                                                                                                        \
    }                                                                                                          \
  }                                                                                                            \
  }  // namespace

template <typename Ast_Type>
struct Parse_Test_Params {
  std::string_view name;
  std::string input;
  std::variant<Ast_Type, std::string> expected;  // Can be AST object or JSON string
  bool should_succeed{};
};

// Catch2 uses operator<< to print test parameters in failure messages
// Note: Uses indent=-1 for compact single-line JSON output
template <typename T>
std::ostream& operator<<(std::ostream& a_os, Parse_Test_Params<T> const& a_params) {
  return a_os << fmt::format(
             R"({{.input = "{}", .expected = {}, .shouldSucceed = {}}})",
             a_params.input,
             get_expected_json(a_params.expected, -1),
             a_params.should_succeed
         );
}