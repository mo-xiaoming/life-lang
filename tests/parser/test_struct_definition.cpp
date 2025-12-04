#include "utils.hpp"

using life_lang::ast::Struct_Definition;

PARSE_TEST(Struct_Definition, struct_definition)

namespace {
// Empty struct
constexpr auto k_empty_struct_input = "struct Empty {}";

// Single field
constexpr auto k_single_field_input = "struct Point { x: I32 }";

// Two fields
constexpr auto k_two_fields_input = "struct Point { x: I32, y: I32 }";

// Multiple fields
constexpr auto k_multiple_fields_input = "struct Person { name: String, age: I32, active: Bool }";

// Qualified types
constexpr auto k_qualified_types_input = "struct Data { value: Std.String, count: Std.I32 }";

// Template types
constexpr auto k_template_types_input = "struct Container { items: Vec<I32>, names: Array<String> }";

// Complex nested templates
constexpr auto k_complex_nested_input = "struct Complex { data: Map<String, Vec<I32>> }";

// Whitespace variations
constexpr auto k_no_spaces_input = "struct Foo{x:I32,y:I32}";
constexpr auto k_multiline_input = R"(struct Point {
  x: I32,
  y: I32
})";

// Trailing comma (should be allowed)
constexpr auto k_trailing_comma_input = "struct Point { x: I32, y: I32, }";

// Struct name variations (Camel_Snake_Case)
constexpr auto k_camel_case_name_input = "struct MyStruct { value: I32 }";
constexpr auto k_camel_snake_case_name_input = "struct My_Struct { value: I32 }";
constexpr auto k_http_response_name_input = "struct HTTP_Response { code: I32 }";

// Trailing content
constexpr auto k_with_trailing_content_input = "struct Point { x: I32 } fn";

// Invalid cases
constexpr auto k_invalid_no_name_input = "struct { x: I32 }";
constexpr auto k_invalid_no_braces_input = "struct Point";
constexpr auto k_invalid_missing_closing_input = "struct Point { x: I32";
constexpr auto k_invalid_missing_field_type_input = "struct Point { x: }";
constexpr auto k_invalid_lowercase_name_input = "struct point { x: I32 }";
constexpr auto k_invalid_empty_input = "";
}  // namespace

TEST_CASE("Parse Struct_Definition", "[parser]") {
  auto const
      params =
          GENERATE(
              Catch::Generators::values<Struct_Definition_Params>(
                  {
                      // Empty struct
                      {"empty struct", k_empty_struct_input,
                       R"({"Struct_Definition": {"fields": [], "name": "Empty"}})", true, ""},

                      // Single field
                      {"single field", k_single_field_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "x", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}], "name": "Point"}})",
                       true, ""},

                      // Two fields
                      {"two fields", k_two_fields_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "x", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}, {"Struct_Field": {"name": "y", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}], "name": "Point"}})",
                       true, ""},

                      // Multiple fields
                      {"multiple fields", k_multiple_fields_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "name", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "String"}}]}}}}, {"Struct_Field": {"name": "age", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}, {"Struct_Field": {"name": "active", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Bool"}}]}}}}], "name": "Person"}})",
                       true, ""},

                      // Qualified types
                      {"qualified types", k_qualified_types_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "value", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Std"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "String"}}]}}}}, {"Struct_Field": {"name": "count", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Std"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}], "name": "Data"}})",
                       true, ""},

                      // Template types
                      {"template types", k_template_types_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "items", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}], "value": "Vec"}}]}}}}, {"Struct_Field": {"name": "names", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "String"}}]}}], "value": "Array"}}]}}}}], "name": "Container"}})",
                       true, ""},

                      // Complex nested templates
                      {"complex nested", k_complex_nested_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "data", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "String"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}], "value": "Vec"}}]}}], "value": "Map"}}]}}}}], "name": "Complex"}})", true,
                       ""},

                      // Whitespace variations
                      {"no spaces", k_no_spaces_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "x", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}, {"Struct_Field": {"name": "y", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}], "name": "Foo"}})",
                       true, ""},
                      {"multiline", k_multiline_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "x", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}, {"Struct_Field": {"name": "y", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}], "name": "Point"}})",
                       true, ""},

                      // Trailing comma
                      {"trailing comma", k_trailing_comma_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "x", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}, {"Struct_Field": {"name": "y", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}], "name": "Point"}})",
                       true, ""},

                      // Struct name variations (Camel_Snake_Case)
                      {"camel case name", k_camel_case_name_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "value", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}], "name": "MyStruct"}})",
                       true, ""},
                      {"camel snake case name", k_camel_snake_case_name_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "value", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}], "name": "My_Struct"}})",
                       true, ""},
                      {"HTTP response name", k_http_response_name_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "code", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}], "name": "HTTP_Response"}})",
                       true, ""},

                      // Trailing content
                      {"with trailing content", k_with_trailing_content_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "x", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}], "name": "Point"}})",
                       true, "fn"},

                      // Parser accepts any identifier - naming conventions checked at semantic analysis
                      {"lowercase name accepted", k_invalid_lowercase_name_input,
                       R"({"Struct_Definition": {"fields": [{"Struct_Field": {"name": "x", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "I32"}}]}}}}], "name": "point"}})",
                       true, ""},

                      // Invalid cases
                      {"invalid - no name", k_invalid_no_name_input,
                       R"({"Struct_Definition": {"fields": [], "name": ""}})", false, "{ x: I32 }"},
                      {"invalid - no braces", k_invalid_no_braces_input,
                       R"({"Struct_Definition": {"fields": [], "name": ""}})", false, ""},
                      {"invalid - missing closing", k_invalid_missing_closing_input,
                       R"({"Struct_Definition": {"fields": [], "name": ""}})", false, ""},
                      {"invalid - missing field type", k_invalid_missing_field_type_input,
                       R"({"Struct_Definition": {"fields": [], "name": ""}})", false, "x: }"},
                      {"invalid - empty", k_invalid_empty_input, R"({"Struct_Definition": {"fields": [], "name": ""}})",
                       false, ""},
                  }
              )
          );

  INFO(params.name);
  check_parse(params);
}
