#include "utils.hpp"

using life_lang::ast::Type_Name;

PARSE_TEST(Type_Name, type_name)

namespace {
// Simple paths
constexpr auto k_simple_path_input = "A";

constexpr auto k_dotted_path_input = "A.B.C";

constexpr auto k_with_spaces_around_input = " A.B ";

// Template parameters
constexpr auto k_single_template_param_input = "Vec<Int>";

constexpr auto k_multiple_template_params_input = "Map<Key, Value>";

constexpr auto k_nested_templates_input = "Vec<Vec<Int>>";

constexpr auto k_complex_nested_input = "A.B.World<Int<e>, Double.c>.Hi.a.b";

// Qualified paths in template parameters
constexpr auto k_qualified_template_param_input = "Array<Data.Model.User>";

constexpr auto k_multiple_qualified_params_input = "Map<Std.String, IO.Error>";

constexpr auto k_qualified_segment_with_template_input = "Std.Collections.Map<Key, Value>";

constexpr auto k_deeply_nested_qualified_input = "Network.Protocol<Http.Request, Http.Response>";

constexpr auto k_complex_qualified_params_input = "Parser<Input.Stream<Byte>, Output.Tree<AST.Node>>";

constexpr auto k_result_with_qualified_types_input = "IO.Result<Data.Error, Parser.AST>";

// Multiple templated segments (not just the last segment)
constexpr auto k_two_templated_segments_input = "Container<Int>.Iterator<Forward>";

constexpr auto k_three_templated_segments_input = "Parser<Token>.Result<AST>.Error<String>";

constexpr auto k_middle_template_input = "Db.Table<User>.Column<Name>.Validator";

constexpr auto k_mixed_templated_segments_input = "Std.Container<T>.Internal.Iterator<Forward>";

// Invalid cases
constexpr auto k_invalid_starts_with_digit_input = "9abc";
constexpr auto k_invalid_empty_input = "";
}  // namespace

TEST_CASE("Parse Type_Name", "[parser]") {
  auto const
      params =
          GENERATE(
              Catch::Generators::values<Type_Name_Params>(
                  {
                      // Simple paths
                      {"simple path", k_simple_path_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "A"}}]}})",
                       true, ""},
                      {"dotted path", k_dotted_path_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "A"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "B"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "C"}}]}})",
                       true, ""},
                      {"with spaces around", k_with_spaces_around_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "A"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "B"}}]}})",
                       true, ""},

                      // Template parameters
                      {"single template param", k_single_template_param_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}], "value": "Vec"}}]}})",
                       true, ""},
                      {"multiple template params", k_multiple_template_params_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Key"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Value"}}]}}], "value": "Map"}}]}})",
                       true, ""},
                      {"nested templates", k_nested_templates_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}], "value": "Vec"}}]}}], "value": "Vec"}}]}})",
                       true, ""},
                      {"complex nested", k_complex_nested_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "A"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "B"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "e"}}]}}], "value": "Int"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Double"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "c"}}]}}], "value": "World"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Hi"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "a"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "b"}}]}})",
                       true, ""},

                      // Qualified paths in template parameters
                      {"qualified template param", k_qualified_template_param_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Data"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Model"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "User"}}]}}], "value": "Array"}}]}})",
                       true, ""},
                      {"multiple qualified params", k_multiple_qualified_params_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Std"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "String"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "IO"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Error"}}]}}], "value": "Map"}}]}})",
                       true, ""},
                      {"qualified segment with template", k_qualified_segment_with_template_input, R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Std"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Collections"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Key"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Value"}}]}}], "value": "Map"}}]}})",
                       true, ""},
                      {"deeply nested qualified", k_deeply_nested_qualified_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Network"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Http"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Request"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Http"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Response"}}]}}], "value": "Protocol"}}]}})",
                       true, ""},
                      {"complex qualified params", k_complex_qualified_params_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Input"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Byte"}}]}}], "value": "Stream"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Output"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "AST"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Node"}}]}}], "value": "Tree"}}]}}], "value": "Parser"}}]}})",
                       true, ""},
                      {"result with qualified types", k_result_with_qualified_types_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "IO"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Data"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Error"}}]}}, {"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Parser"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "AST"}}]}}], "value": "Result"}}]}})",
                       true, ""},

                      // Multiple templated segments
                      {"two templated segments", k_two_templated_segments_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Int"}}]}}], "value": "Container"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Forward"}}]}}], "value": "Iterator"}}]}})",
                       true, ""},
                      {"three templated segments", k_three_templated_segments_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Token"}}]}}], "value": "Parser"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "AST"}}]}}], "value": "Result"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "String"}}]}}], "value": "Error"}}]}})",
                       true, ""},
                      {"middle template", k_middle_template_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Db"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "User"}}]}}], "value": "Table"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Name"}}]}}], "value": "Column"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Validator"}}]}})",
                       true, ""},
                      {"mixed templated segments", k_mixed_templated_segments_input,
                       R"({"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Std"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "T"}}]}}], "value": "Container"}}, {"Type_Name_Segment": {"templateParameters": [], "value": "Internal"}}, {"Type_Name_Segment": {"templateParameters": [{"Type_Name": {"segments": [{"Type_Name_Segment": {"templateParameters": [], "value": "Forward"}}]}}], "value": "Iterator"}}]}})",
                       true, ""},

                      // Invalid cases
                      {"invalid - starts with digit", k_invalid_starts_with_digit_input,
                       R"({"Type_Name": {"segments": []}})", false, "9abc"},
                      {"invalid - empty", k_invalid_empty_input, R"({"Type_Name": {"segments": []}})", false, ""},
                  }
              )
          );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}