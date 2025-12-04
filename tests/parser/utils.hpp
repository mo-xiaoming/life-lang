// Parser Test Utilities
//
// Usage: Test parser rules using either AST objects or JSON strings
//
// Example with JSON (recommended for readability):
//   constexpr auto k_input = "foo();";
//   constexpr auto k_expected = R"({
//     "Function_Call_Statement": {
//       "expr": { ... }
//     }
//   })";
//   {"test name", k_input, k_expected, true, ""}
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

#include "internal_rules.hpp"

// Common JSON building helpers to reduce duplication
namespace test_json {

// Variable name with single segment (no templates)
inline std::string var_name(std::string_view a_name) {
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

// Type name with single segment (no templates)
inline std::string type_name(std::string_view a_name) {
  return fmt::format(
      R"({{
    "Type_Name": {{
      "segments": [
        {{
          "Type_Name_Segment": {{
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

#define PARSE_TEST(AstType, fn_name)                                               \
  namespace {                                                                      \
  using AstType##_Params = Parse_Test_Params<AstType>;                             \
  void check_parse(AstType##_Params const& params) {                               \
    auto input_start = params.input.cbegin();                                      \
    auto const input_end = params.input.cend();                                    \
    auto const got = life_lang::internal::parse_##fn_name(input_start, input_end); \
    CHECK(params.should_succeed == bool(got));                                     \
    if (params.should_succeed != bool(got)) {                                      \
      if (got) {                                                                   \
        UNSCOPED_INFO(to_json_string(*got, 2));                                    \
      } else {                                                                     \
        UNSCOPED_INFO(got.error());                                                \
      }                                                                            \
    }                                                                              \
    auto const rest = std::string_view{input_start, input_end};                    \
    CHECK(params.rest == rest);                                                    \
    if (got) {                                                                     \
      auto const expected_json = get_expected_json(params.expected, 2);            \
      auto const actual_json = to_json_string(*got, 2);                            \
      CHECK(expected_json == actual_json);                                         \
    }                                                                              \
  }                                                                                \
  }  // namespace

template <typename Ast_Type>
struct Parse_Test_Params {
  std::string_view name;
  std::string input;
  std::variant<Ast_Type, std::string> expected;  // Can be AST object or JSON string
  bool should_succeed{};
  std::string_view rest;
};

// Catch2 uses operator<< to print test parameters in failure messages
// Note: Uses indent=-1 for compact single-line JSON output
template <typename T>
std::ostream& operator<<(std::ostream& a_os, Parse_Test_Params<T> const& a_params) {
  return a_os << fmt::format(
             R"({{.input = "{}", .expected = {}, .shouldSucceed = {}, .rest = "{}"}})", a_params.input,
             get_expected_json(a_params.expected, -1), a_params.should_succeed, a_params.rest
         );
}