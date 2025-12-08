#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Struct_Definition;

PARSE_TEST(Struct_Definition, struct_definition)

namespace {
// Empty struct
constexpr auto k_empty_struct_should_succeed = true;
constexpr auto k_empty_struct_input = "struct Empty {}";
inline auto const k_empty_struct_expected = test_json::struct_definition("Empty", {});

// Single field
constexpr auto k_single_field_should_succeed = true;
constexpr auto k_single_field_input = "struct Point { x: I32 }";
inline auto const k_single_field_expected =
    test_json::struct_definition("Point", {test_json::struct_field("x", test_json::type_name("I32"))});

// Two fields
constexpr auto k_two_fields_should_succeed = true;
constexpr auto k_two_fields_input = "struct Point { x: I32, y: I32 }";
inline auto const k_two_fields_expected = test_json::struct_definition(
    "Point",
    {test_json::struct_field("x", test_json::type_name("I32")),
     test_json::struct_field("y", test_json::type_name("I32"))}
);

// Multiple fields
constexpr auto k_multiple_fields_should_succeed = true;
constexpr auto k_multiple_fields_input = "struct Person { name: String, age: I32, active: Bool }";
inline auto const k_multiple_fields_expected = test_json::struct_definition(
    "Person",
    {test_json::struct_field("name", test_json::type_name("String")),
     test_json::struct_field("age", test_json::type_name("I32")),
     test_json::struct_field("active", test_json::type_name("Bool"))}
);

// Qualified types
constexpr auto k_qualified_types_should_succeed = true;
constexpr auto k_qualified_types_input = "struct Data { value: Std.String, count: Std.I32 }";
inline auto const k_qualified_types_expected = test_json::struct_definition(
    "Data",
    {test_json::struct_field("value", test_json::type_name_path({"Std", "String"})),
     test_json::struct_field("count", test_json::type_name_path({"Std", "I32"}))}
);

// Template types
constexpr auto k_template_types_should_succeed = true;
constexpr auto k_template_types_input = "struct Container { items: Vec<I32>, names: Array<String> }";
inline auto const k_template_types_expected = R"(
{
  "Struct_Definition": {
    "fields": [
      {
        "Struct_Field": {
          "name": "items",
          "type": {
            "Type_Name": {
              "segments": [
                {
                  "Type_Name_Segment": {
                    "template_parameters": [
                      {"Type_Name": {"segments": [{"Type_Name_Segment": {"template_parameters": [], "value": "I32"}}]}}
                    ],
                    "value": "Vec"
                  }
                }
              ]
            }
          }
        }
      },
      {
        "Struct_Field": {
          "name": "names",
          "type": {
            "Type_Name": {
              "segments": [
                {
                  "Type_Name_Segment": {
                    "template_parameters": [
                      {"Type_Name": {"segments": [{"Type_Name_Segment": {"template_parameters": [], "value": "String"}}]}}
                    ],
                    "value": "Array"
                  }
                }
              ]
            }
          }
        }
      }
    ],
    "name": "Container"
  }
}
)";

// Complex nested templates
constexpr auto k_complex_nested_should_succeed = true;
constexpr auto k_complex_nested_input = "struct Complex { data: Map<String, Vec<I32>> }";
inline auto const k_complex_nested_expected = R"(
{
  "Struct_Definition": {
    "fields": [
      {
        "Struct_Field": {
          "name": "data",
          "type": {
            "Type_Name": {
              "segments": [
                {
                  "Type_Name_Segment": {
                    "template_parameters": [
                      {"Type_Name": {"segments": [{"Type_Name_Segment": {"template_parameters": [], "value": "String"}}]}},
                      {
                        "Type_Name": {
                          "segments": [
                            {
                              "Type_Name_Segment": {
                                "template_parameters": [
                                  {"Type_Name": {"segments": [{"Type_Name_Segment": {"template_parameters": [], "value": "I32"}}]}}
                                ],
                                "value": "Vec"
                              }
                            }
                          ]
                        }
                      }
                    ],
                    "value": "Map"
                  }
                }
              ]
            }
          }
        }
      }
    ],
    "name": "Complex"
  }
}
)";

// Whitespace variations
constexpr auto k_no_spaces_should_succeed = true;
constexpr auto k_no_spaces_input = "struct Foo{x:I32,y:I32}";
inline auto const k_no_spaces_expected = test_json::struct_definition(
    "Foo",
    {test_json::struct_field("x", test_json::type_name("I32")),
     test_json::struct_field("y", test_json::type_name("I32"))}
);

constexpr auto k_multiline_should_succeed = true;
constexpr auto k_multiline_input = R"(struct Point {
  x: I32,
  y: I32
})";
inline auto const k_multiline_expected = test_json::struct_definition(
    "Point",
    {test_json::struct_field("x", test_json::type_name("I32")),
     test_json::struct_field("y", test_json::type_name("I32"))}
);

// Trailing comma (should be allowed)
constexpr auto k_trailing_comma_should_succeed = true;
constexpr auto k_trailing_comma_input = "struct Point { x: I32, y: I32, }";
inline auto const k_trailing_comma_expected = test_json::struct_definition(
    "Point",
    {test_json::struct_field("x", test_json::type_name("I32")),
     test_json::struct_field("y", test_json::type_name("I32"))}
);

// Struct name variations (Camel_Snake_Case)
constexpr auto k_camel_case_name_should_succeed = true;
constexpr auto k_camel_case_name_input = "struct MyStruct { value: I32 }";
inline auto const k_camel_case_name_expected =
    test_json::struct_definition("MyStruct", {test_json::struct_field("value", test_json::type_name("I32"))});

constexpr auto k_camel_snake_case_name_should_succeed = true;
constexpr auto k_camel_snake_case_name_input = "struct My_Struct { value: I32 }";
inline auto const k_camel_snake_case_name_expected =
    test_json::struct_definition("My_Struct", {test_json::struct_field("value", test_json::type_name("I32"))});

constexpr auto k_http_response_name_should_succeed = true;
constexpr auto k_http_response_name_input = "struct HTTP_Response { code: I32 }";
inline auto const k_http_response_name_expected =
    test_json::struct_definition("HTTP_Response", {test_json::struct_field("code", test_json::type_name("I32"))});

// Trailing content
constexpr auto k_with_trailing_content_should_succeed = true;
constexpr auto k_with_trailing_content_input = "struct Point { x: I32 } fn";
inline auto const k_with_trailing_content_expected =
    test_json::struct_definition("Point", {test_json::struct_field("x", test_json::type_name("I32"))});

// Parser accepts any identifier - naming conventions checked at semantic analysis
constexpr auto k_lowercase_name_accepted_should_succeed = true;
constexpr auto k_lowercase_name_accepted_input = "struct point { x: I32 }";
inline auto const k_lowercase_name_accepted_expected =
    test_json::struct_definition("point", {test_json::struct_field("x", test_json::type_name("I32"))});

// Invalid cases
constexpr auto k_invalid_no_name_should_succeed = false;
constexpr auto k_invalid_no_name_input = "struct { x: I32 }";
constexpr auto k_invalid_no_name_expected = R"({"Struct_Definition": {"fields": [], "name": ""}})";

constexpr auto k_invalid_no_braces_should_succeed = false;
constexpr auto k_invalid_no_braces_input = "struct Point";
constexpr auto k_invalid_no_braces_expected = R"({"Struct_Definition": {"fields": [], "name": ""}})";

constexpr auto k_invalid_missing_closing_should_succeed = false;
constexpr auto k_invalid_missing_closing_input = "struct Point { x: I32";
constexpr auto k_invalid_missing_closing_expected = R"({"Struct_Definition": {"fields": [], "name": ""}})";

constexpr auto k_invalid_missing_field_type_should_succeed = false;
constexpr auto k_invalid_missing_field_type_input = "struct Point { x: }";
constexpr auto k_invalid_missing_field_type_expected = R"({"Struct_Definition": {"fields": [], "name": ""}})";

constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";
constexpr auto k_invalid_empty_expected = R"({"Struct_Definition": {"fields": [], "name": ""}})";
}  // namespace

TEST_CASE("Parse Struct_Definition", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Struct_Definition_Params>({
          {"empty struct", k_empty_struct_input, k_empty_struct_expected, k_empty_struct_should_succeed},
          {"single field", k_single_field_input, k_single_field_expected, k_single_field_should_succeed},
          {"two fields", k_two_fields_input, k_two_fields_expected, k_two_fields_should_succeed},
          {"multiple fields", k_multiple_fields_input, k_multiple_fields_expected, k_multiple_fields_should_succeed},
          {"qualified types", k_qualified_types_input, k_qualified_types_expected, k_qualified_types_should_succeed},
          {"template types", k_template_types_input, k_template_types_expected, k_template_types_should_succeed},
          {"complex nested", k_complex_nested_input, k_complex_nested_expected, k_complex_nested_should_succeed},
          {"no spaces", k_no_spaces_input, k_no_spaces_expected, k_no_spaces_should_succeed},
          {"multiline", k_multiline_input, k_multiline_expected, k_multiline_should_succeed},
          {"trailing comma", k_trailing_comma_input, k_trailing_comma_expected, k_trailing_comma_should_succeed},
          {"camel case name", k_camel_case_name_input, k_camel_case_name_expected, k_camel_case_name_should_succeed},
          {"camel snake case name",
           k_camel_snake_case_name_input,
           k_camel_snake_case_name_expected,
           k_camel_snake_case_name_should_succeed},
          {"HTTP response name",
           k_http_response_name_input,
           k_http_response_name_expected,
           k_http_response_name_should_succeed},
          {"with trailing content",
           k_with_trailing_content_input,
           k_with_trailing_content_expected,
           k_with_trailing_content_should_succeed},
          {"lowercase name accepted",
           k_lowercase_name_accepted_input,
           k_lowercase_name_accepted_expected,
           k_lowercase_name_accepted_should_succeed},
          {"invalid - no name", k_invalid_no_name_input, k_invalid_no_name_expected, k_invalid_no_name_should_succeed},
          {"invalid - no braces",
           k_invalid_no_braces_input,
           k_invalid_no_braces_expected,
           k_invalid_no_braces_should_succeed},
          {"invalid - missing closing",
           k_invalid_missing_closing_input,
           k_invalid_missing_closing_expected,
           k_invalid_missing_closing_should_succeed},
          {"invalid - missing field type",
           k_invalid_missing_field_type_input,
           k_invalid_missing_field_type_expected,
           k_invalid_missing_field_type_should_succeed},
          {"invalid - empty", k_invalid_empty_input, k_invalid_empty_expected, k_invalid_empty_should_succeed},
      })
  );

  INFO(params.name);
  check_parse(params);
}
