#include "utils.hpp"

using life_lang::ast::Function_Declaration;

PARSE_TEST(Function_Declaration, function_declaration)

namespace {
// Basic declarations
constexpr auto k_no_parameters_input = "fn foo(): Int";

// Parameter variations
constexpr auto k_one_parameter_input = "fn foo(x: Int): Int";
constexpr auto k_two_parameters_input = "fn foo(hello: T, world: U): Int";

// Return type variations
constexpr auto k_namespace_return_type_input = "fn foo(): Std.String";
constexpr auto k_template_return_type_input = "fn foo(): Vec<Int>";

// Complex template types
constexpr auto k_complex_templates_input = "fn foo(hello: A.B.Hello<Std.Array, B.C<Int, Double>>): A.B.C<Int>";

// Whitespace handling
constexpr auto k_with_spaces_input = "fn  foo  (  x  :  Int  )  :  Int";

// Trailing content
constexpr auto k_trailing_body_input = "fn foo(): Int {";

// Invalid cases
constexpr auto k_invalid_no_fn_keyword_input = "foo(): Int";
constexpr auto k_invalid_no_return_type_input = "fn foo()";
constexpr auto k_invalid_no_parentheses_input = "fn foo: Int";
}  // namespace

TEST_CASE("Parse Function_Declaration", "[parser]") {
  auto const
      params =
          GENERATE(
              Catch::Generators::values<Function_Declaration_Params>(
                  {
                      // Basic declarations
                      {"no parameters", k_no_parameters_input,
                       R"({"Function_Declaration": {"name": "foo", "parameters": [], "returnType": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}}})",
                       true, ""},

                      // Parameter variations
                      {"one parameter", k_one_parameter_input,
                       R"({"Function_Declaration": {"name": "foo", "parameters": [{"Function_Parameter": {"name": "x", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}}}], "returnType": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}}})",
                       true, ""},
                      {"two parameters", k_two_parameters_input,
                       R"({"Function_Declaration": {"name": "foo", "parameters": [{"Function_Parameter": {"name": "hello", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "T"}}]}}}}, {"Function_Parameter": {"name": "world", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "U"}}]}}}}], "returnType": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}}})",
                       true, ""},

                      // Return type variations
                      {"namespace return type", k_namespace_return_type_input,
                       R"({"Function_Declaration": {"name": "foo", "parameters": [], "returnType": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Std"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "String"}}]}}}})",
                       true, ""},
                      {"template return type", k_template_return_type_input,
                       R"({"Function_Declaration": {"name": "foo", "parameters": [], "returnType": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}], "value": "Vec"}}]}}}})",
                       true, ""},

                      // Complex template types
                      {"complex templates", k_complex_templates_input, R"({"Function_Declaration": {"name": "foo", "parameters": [{"Function_Parameter": {"name": "hello", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "A"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "B"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Std"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Array"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "B"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Double"}}]}}], "value": "C"}}]}}], "value": "Hello"}}]}}}}], "returnType": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "A"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "B"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}], "value": "C"}}]}}}})",
                       true, ""},

                      // Whitespace handling
                      {"with spaces", k_with_spaces_input,
                       R"({"Function_Declaration": {"name": "foo", "parameters": [{"Function_Parameter": {"name": "x", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}}}], "returnType": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}}})",
                       true, ""},

                      // Trailing content
                      {"trailing body", k_trailing_body_input,
                       R"({"Function_Declaration": {"name": "foo", "parameters": [], "returnType": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}}})",
                       true, "{"},

                      // Invalid cases
                      {"invalid - no fn keyword", k_invalid_no_fn_keyword_input,
                       R"({"Function_Declaration": {"name": "", "parameters": [], "returnType": {"Type_Name": {"segments": []}}}})",
                       false, "foo(): Int"},
                      {"invalid - no return type", k_invalid_no_return_type_input,
                       R"({"Function_Declaration": {"name": "", "parameters": [], "returnType": {"Type_Name": {"segments": []}}}})",
                       false, ""},
                      {"invalid - no parentheses", k_invalid_no_parentheses_input,
                       R"({"Function_Declaration": {"name": "", "parameters": [], "returnType": {"Type_Name": {"segments": []}}}})",
                       false, ": Int"},
                  }
              )
          );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}