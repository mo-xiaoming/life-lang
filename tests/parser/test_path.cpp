#include "utils.hpp"

using life_lang::ast::make_path;
using life_lang::ast::make_path_segment;
using life_lang::ast::Path;

PARSE_TEST(Path, path)

namespace {
// Simple paths
constexpr auto k_simple_path_input = "A";

constexpr auto k_dotted_path_input = "A.B.C";

constexpr auto k_with_spaces_around_input = " A.B ";

// Template parameters
constexpr auto k_single_template_param_input = "Vec<Int>";
auto make_single_template_param_expected() { return make_path(make_path_segment("Vec", {make_path("Int")})); }

constexpr auto k_multiple_template_params_input = "Map<Key, Value>";
auto make_multiple_template_params_expected() {
  return make_path(make_path_segment("Map", {make_path("Key"), make_path("Value")}));
}

constexpr auto k_nested_templates_input = "Vec<Vec<Int>>";
auto make_nested_templates_expected() {
  return make_path(make_path_segment("Vec", {make_path(make_path_segment("Vec", {make_path("Int")}))}));
}

constexpr auto k_complex_nested_input = "A.B.World<Int<e>, Double.c>.Hi.a.b";
auto make_complex_nested_expected() {
  return make_path(
      "A", "B",
      make_path_segment("World", {make_path(make_path_segment("Int", {make_path("e")})), make_path("Double", "c")}),
      "Hi", "a", "b"
  );
}

// Qualified paths in template parameters
constexpr auto k_qualified_template_param_input = "Array<Data.Model.User>";
auto make_qualified_template_param_expected() {
  return make_path(make_path_segment("Array", {make_path("Data", "Model", "User")}));
}

constexpr auto k_multiple_qualified_params_input = "Map<Std.String, IO.Error>";
auto make_multiple_qualified_params_expected() {
  return make_path(make_path_segment("Map", {make_path("Std", "String"), make_path("IO", "Error")}));
}

constexpr auto k_qualified_segment_with_template_input = "Std.Collections.Map<Key, Value>";
auto make_qualified_segment_with_template_expected() {
  return make_path("Std", "Collections", make_path_segment("Map", {make_path("Key"), make_path("Value")}));
}

constexpr auto k_deeply_nested_qualified_input = "Network.Protocol<Http.Request, Http.Response>";
auto make_deeply_nested_qualified_expected() {
  return make_path(
      "Network", make_path_segment("Protocol", {make_path("Http", "Request"), make_path("Http", "Response")})
  );
}

constexpr auto k_complex_qualified_params_input = "Parser<Input.Stream<Byte>, Output.Tree<AST.Node>>";
auto make_complex_qualified_params_expected() {
  return make_path(make_path_segment(
      "Parser", {make_path("Input", make_path_segment("Stream", {make_path("Byte")})),
                 make_path("Output", make_path_segment("Tree", {make_path("AST", "Node")}))}
  ));
}

constexpr auto k_result_with_qualified_types_input = "IO.Result<Data.Error, Parser.AST>";
auto make_result_with_qualified_types_expected() {
  return make_path("IO", make_path_segment("Result", {make_path("Data", "Error"), make_path("Parser", "AST")}));
}

// Multiple templated segments (not just the last segment)
constexpr auto k_two_templated_segments_input = "Container<Int>.Iterator<Forward>";
auto make_two_templated_segments_expected() {
  return make_path(
      make_path_segment("Container", {make_path("Int")}), make_path_segment("Iterator", {make_path("Forward")})
  );
}

constexpr auto k_three_templated_segments_input = "Parser<Token>.Result<AST>.Error<String>";
auto make_three_templated_segments_expected() {
  return make_path(
      make_path_segment("Parser", {make_path("Token")}), make_path_segment("Result", {make_path("AST")}),
      make_path_segment("Error", {make_path("String")})
  );
}

constexpr auto k_middle_template_input = "Db.Table<User>.Column<Name>.Validator";
auto make_middle_template_expected() {
  return make_path(
      "Db", make_path_segment("Table", {make_path("User")}), make_path_segment("Column", {make_path("Name")}),
      "Validator"
  );
}

constexpr auto k_mixed_templated_segments_input = "Std.Container<T>.Internal.Iterator<Forward>";
auto make_mixed_templated_segments_expected() {
  return make_path(
      "Std", make_path_segment("Container", {make_path("T")}), "Internal",
      make_path_segment("Iterator", {make_path("Forward")})
  );
}

// Invalid cases
constexpr auto k_invalid_starts_with_digit_input = "9abc";
constexpr auto k_invalid_empty_input = "";
}  // namespace

TEST_CASE("Parse Path", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Path_Params>({
          // Simple paths
          {"simple path", k_simple_path_input, make_path("A"), true, ""},
          {"dotted path", k_dotted_path_input, make_path("A", "B", "C"), true, ""},
          {"with spaces around", k_with_spaces_around_input, make_path("A", "B"), true, ""},

          // Template parameters
          {"single template param", k_single_template_param_input, make_single_template_param_expected(), true, ""},
          {"multiple template params", k_multiple_template_params_input, make_multiple_template_params_expected(), true,
           ""},
          {"nested templates", k_nested_templates_input, make_nested_templates_expected(), true, ""},
          {"complex nested", k_complex_nested_input, make_complex_nested_expected(), true, ""},

          // Qualified paths in template parameters
          {"qualified template param", k_qualified_template_param_input, make_qualified_template_param_expected(), true,
           ""},
          {"multiple qualified params", k_multiple_qualified_params_input, make_multiple_qualified_params_expected(),
           true, ""},
          {"qualified segment with template", k_qualified_segment_with_template_input,
           make_qualified_segment_with_template_expected(), true, ""},
          {"deeply nested qualified", k_deeply_nested_qualified_input, make_deeply_nested_qualified_expected(), true,
           ""},
          {"complex qualified params", k_complex_qualified_params_input, make_complex_qualified_params_expected(), true,
           ""},
          {"result with qualified types", k_result_with_qualified_types_input,
           make_result_with_qualified_types_expected(), true, ""},

          // Multiple templated segments
          {"two templated segments", k_two_templated_segments_input, make_two_templated_segments_expected(), true, ""},
          {"three templated segments", k_three_templated_segments_input, make_three_templated_segments_expected(), true,
           ""},
          {"middle template", k_middle_template_input, make_middle_template_expected(), true, ""},
          {"mixed templated segments", k_mixed_templated_segments_input, make_mixed_templated_segments_expected(), true,
           ""},

          // Invalid cases
          {"invalid - starts with digit", k_invalid_starts_with_digit_input, make_path(), false, "9abc"},
          {"invalid - empty", k_invalid_empty_input, make_path(), false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}