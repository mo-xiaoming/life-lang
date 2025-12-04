#include "utils.hpp"

using life_lang::ast::Struct_Field;

PARSE_TEST(Struct_Field, struct_field)

namespace {
// Simple types
constexpr auto k_simple_type_input = "x: I32";

// Whitespace variations
constexpr auto k_no_spaces_input = "name:String";

// Qualified types
constexpr auto k_with_namespace_input = "value: Std.String";

// Template types
constexpr auto k_template_parameter_input = "items: Vec<Int>";

constexpr auto k_complex_nested_templates_input = "data: A.B.Container<Std.Array, A.B.C<I32, F64>>";

// Trailing content
constexpr auto k_with_trailing_comma_input = "x: I32,";

// Invalid cases
constexpr auto k_invalid_no_colon_input = "x I32";
constexpr auto k_invalid_no_type_input = "x:";
constexpr auto k_invalid_no_name_input = ": I32";
constexpr auto k_invalid_empty_input = "";
}  // namespace

TEST_CASE("Parse Struct_Field", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Struct_Field_Params>({
          // Simple types
          {"simple type", k_simple_type_input,
           R"({"Struct_Field": {"name": "x", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"value": "I32", "templateParameters": []}}]}}}})",
           true, ""},

          // Whitespace variations
          {"no spaces", k_no_spaces_input,
           R"({"Struct_Field": {"name": "name", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"value": "String", "templateParameters": []}}]}}}})",
           true, ""},

          // Qualified types
          {"with namespace", k_with_namespace_input,
           R"({"Struct_Field": {"name": "value", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"value": "Std", "templateParameters": []}}, {"Type_Name_Segment": {"value": "String", "templateParameters": []}}]}}}})",
           true, ""},

          // Template types
          {"template parameter", k_template_parameter_input,
           R"({"Struct_Field": {"name": "items", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"value": "Vec", "templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"value": "Int", "templateParameters": []}}]}}]}}]}}}})",
           true, ""},
          {"complex nested templates", k_complex_nested_templates_input,
           R"({"Struct_Field": {"name": "data", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"value": "A", "templateParameters": []}}, {"Type_Name_Segment": {"value": "B", "templateParameters": []}}, {"Type_Name_Segment": {"value": "Container", "templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"value": "Std", "templateParameters": []}}, {"Type_Name_Segment": {"value": "Array", "templateParameters": []}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"value": "A", "templateParameters": []}}, {"Type_Name_Segment": {"value": "B", "templateParameters": []}}, {"Type_Name_Segment": {"value": "C", "templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"value": "I32", "templateParameters": []}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"value": "F64", "templateParameters": []}}]}}]}}]}}]}}]}}}})",
           true, ""},

          // Trailing content
          {"with trailing comma", k_with_trailing_comma_input,
           R"({"Struct_Field": {"name": "x", "type": {"Type_Name": {"segments": [{"Type_Name_Segment": {"value": "I32", "templateParameters": []}}]}}}})",
           true, ","},

          // Invalid cases
          {"invalid - no colon", k_invalid_no_colon_input,
           R"({"Struct_Field": {"name": "", "type": {"Type_Name": {"segments": []}}}})", false, "I32"},
          {"invalid - no type", k_invalid_no_type_input,
           R"({"Struct_Field": {"name": "", "type": {"Type_Name": {"segments": []}}}})", false, ""},
          {"invalid - no name", k_invalid_no_name_input,
           R"({"Struct_Field": {"name": "", "type": {"Type_Name": {"segments": []}}}})", false, ": I32"},
          {"invalid - empty", k_invalid_empty_input,
           R"({"Struct_Field": {"name": "", "type": {"Type_Name": {"segments": []}}}})", false, ""},
      })
  );

  INFO(params.name);
  check_parse(params);
}
