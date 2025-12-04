#include "utils.hpp"

using life_lang::ast::Type_Name_Segment;

PARSE_TEST(Type_Name_Segment, type_name_segment)

namespace {
// Simple variable_names
constexpr auto k_simple_variable_name_input = "hello";

constexpr auto k_with_underscore_input = "hello_world";

constexpr auto k_with_digits_input = "h340";

constexpr auto k_uppercase_start_input = "Int";

constexpr auto k_with_trailing_space_input = "Int  {";

// Template parameters
constexpr auto k_single_template_param_input = "Hello<Int>";

constexpr auto k_multiple_template_params_input = "Hello<Int, Double>";

constexpr auto k_nested_template_input = "Vec<Vec<Int>>";

constexpr auto k_template_with_spaces_input = "Map < Key , Value >";

// Qualified paths in template parameters
constexpr auto k_qualified_single_param_input = "Array<Data.Model.User>";

constexpr auto k_qualified_multiple_params_input = "Map<Std.String, IO.Error>";

constexpr auto k_nested_qualified_input = "Parser<Input.Stream<Byte>>";

constexpr auto k_complex_qualified_input = "Result<Data.Error, Value.Type>";

constexpr auto k_deeply_nested_qualified_input = "Wrapper<Network.Protocol<Http.Request, Http.Response>>";

// Invalid cases
constexpr auto k_invalid_starts_with_digit_input = "0abc";
constexpr auto k_invalid_starts_with_underscore_input = "_hello";
constexpr auto k_invalid_empty_input = "";
}  // namespace

TEST_CASE("Parse Type_Name_Segment", "[parser]") {
  auto const params =
      GENERATE(
          Catch::Generators::values<Type_Name_Segment_Params>(
              {
                  {"simple variable_name", k_simple_variable_name_input,
                   R"({"Type_Name_Segment": {"templateParameters": [], "value": "hello"}})", true, ""},
                  {"with underscore", k_with_underscore_input,
                   R"({"Type_Name_Segment": {"templateParameters": [], "value": "hello_world"}})", true, ""},
                  {"with digits", k_with_digits_input,
                   R"({"Type_Name_Segment": {"templateParameters": [], "value": "h340"}})", true, ""},
                  {"uppercase start", k_uppercase_start_input,
                   R"({"Type_Name_Segment": {"templateParameters": [], "value": "Int"}})", true, ""},
                  {"with trailing space", k_with_trailing_space_input,
                   R"({"Type_Name_Segment": {"templateParameters": [], "value": "Int"}})", true, "{"},
                  // Template parameters
                  {"single template param", k_single_template_param_input,
                   R"({"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}], "value": "Hello"}})",
                   true, ""},
                  {"multiple template params", k_multiple_template_params_input,
                   R"({"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Double"}}]}}], "value": "Hello"}})",
                   true, ""},
                  {"nested template", k_nested_template_input,
                   R"({"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}], "value": "Vec"}}]}}], "value": "Vec"}})",
                   true, ""},
                  {"template with spaces", k_template_with_spaces_input,
                   R"({"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Key"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Value"}}]}}], "value": "Map"}})",
                   true, ""},
                  // Complex examples with qualified paths in templates
                  {"qualified single param", k_qualified_single_param_input,
                   R"({"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Data"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Model"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "User"}}]}}], "value": "Array"}})",
                   true, ""},
                  {"qualified multiple params", k_qualified_multiple_params_input,
                   R"({"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Std"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "String"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "IO"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Error"}}]}}], "value": "Map"}})",
                   true, ""},
                  {"nested qualified", k_nested_qualified_input,
                   R"({"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Input"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Byte"}}]}}], "value": "Stream"}}]}}], "value": "Parser"}})",
                   true, ""},
                  {"complex qualified", k_complex_qualified_input,
                   R"({"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Data"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Error"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Value"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Type"}}]}}], "value": "Result"}})",
                   true, ""},
                  {"deeply nested qualified", k_deeply_nested_qualified_input, R"({"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Network"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Http"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Request"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Http"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Response"}}]}}], "value": "Protocol"}}]}}], "value": "Wrapper"}})",
                   true, ""},
                  // Invalid cases
                  {"invalid - starts with digit", k_invalid_starts_with_digit_input,
                   R"({"Type_Name_Segment": {"templateParameters": [], "value": ""}})", false, "0abc"},
                  {"invalid - starts with underscore", k_invalid_starts_with_underscore_input,
                   R"({"Type_Name_Segment": {"templateParameters": [], "value": ""}})", false, "_hello"},
                  {"invalid - empty", k_invalid_empty_input,
                   R"({"Type_Name_Segment": {"templateParameters": [], "value": ""}})", false, ""},
              }
          )
      );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}