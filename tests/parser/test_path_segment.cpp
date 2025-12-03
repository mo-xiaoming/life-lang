#include "utils.hpp"

using life_lang::ast::make_path;
using life_lang::ast::make_path_segment;
using life_lang::ast::Path_Segment;

PARSE_TEST(Path_Segment, path_segment)

namespace {
// Simple identifiers
constexpr auto k_simple_identifier_input = "hello";

constexpr auto k_with_underscore_input = "hello_world";

constexpr auto k_with_digits_input = "h340";

constexpr auto k_uppercase_start_input = "Int";

constexpr auto k_with_trailing_space_input = "Int  {";

// Template parameters
constexpr auto k_single_template_param_input = "Hello<Int>";
auto make_single_template_param_expected() { return make_path_segment("Hello", {make_path("Int")}); }

constexpr auto k_multiple_template_params_input = "Hello<Int, Double>";
auto make_multiple_template_params_expected() {
  return make_path_segment("Hello", {make_path("Int"), make_path("Double")});
}

constexpr auto k_nested_template_input = "Vec<Vec<Int>>";
auto make_nested_template_expected() {
  return make_path_segment("Vec", {make_path(make_path_segment("Vec", {make_path("Int")}))});
}

constexpr auto k_template_with_spaces_input = "Map < Key , Value >";
auto make_template_with_spaces_expected() { return make_path_segment("Map", {make_path("Key"), make_path("Value")}); }

// Qualified paths in template parameters
constexpr auto k_qualified_single_param_input = "Array<Data.Model.User>";
auto make_qualified_single_param_expected() { return make_path_segment("Array", {make_path("Data", "Model", "User")}); }

constexpr auto k_qualified_multiple_params_input = "Map<Std.String, IO.Error>";
auto make_qualified_multiple_params_expected() {
  return make_path_segment("Map", {make_path("Std", "String"), make_path("IO", "Error")});
}

constexpr auto k_nested_qualified_input = "Parser<Input.Stream<Byte>>";
auto make_nested_qualified_expected() {
  return make_path_segment("Parser", {make_path("Input", make_path_segment("Stream", {make_path("Byte")}))});
}

constexpr auto k_complex_qualified_input = "Result<Data.Error, Value.Type>";
auto make_complex_qualified_expected() {
  return make_path_segment("Result", {make_path("Data", "Error"), make_path("Value", "Type")});
}

constexpr auto k_deeply_nested_qualified_input = "Wrapper<Network.Protocol<Http.Request, Http.Response>>";
auto make_deeply_nested_qualified_expected() {
  return make_path_segment(
      "Wrapper",
      {make_path(
          "Network", make_path_segment("Protocol", {make_path("Http", "Request"), make_path("Http", "Response")})
      )}
  );
}

// Invalid cases
constexpr auto k_invalid_starts_with_digit_input = "0abc";
constexpr auto k_invalid_starts_with_underscore_input = "_hello";
constexpr auto k_invalid_empty_input = "";
}  // namespace

TEST_CASE("Parse Path_Segment", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Path_Segment_Params>({
          {"simple identifier", k_simple_identifier_input, make_path_segment("hello"), true, ""},
          {"with underscore", k_with_underscore_input, make_path_segment("hello_world"), true, ""},
          {"with digits", k_with_digits_input, make_path_segment("h340"), true, ""},
          {"uppercase start", k_uppercase_start_input, make_path_segment("Int"), true, ""},
          {"with trailing space", k_with_trailing_space_input, make_path_segment("Int"), true, "{"},
          // Template parameters
          {"single template param", k_single_template_param_input, make_single_template_param_expected(), true, ""},
          {"multiple template params", k_multiple_template_params_input, make_multiple_template_params_expected(), true,
           ""},
          {"nested template", k_nested_template_input, make_nested_template_expected(), true, ""},
          {"template with spaces", k_template_with_spaces_input, make_template_with_spaces_expected(), true, ""},
          // Complex examples with qualified paths in templates
          {"qualified single param", k_qualified_single_param_input, make_qualified_single_param_expected(), true, ""},
          {"qualified multiple params", k_qualified_multiple_params_input, make_qualified_multiple_params_expected(),
           true, ""},
          {"nested qualified", k_nested_qualified_input, make_nested_qualified_expected(), true, ""},
          {"complex qualified", k_complex_qualified_input, make_complex_qualified_expected(), true, ""},
          {"deeply nested qualified", k_deeply_nested_qualified_input, make_deeply_nested_qualified_expected(), true,
           ""},
          // Invalid cases
          {"invalid - starts with digit", k_invalid_starts_with_digit_input, make_path_segment(""), false, "0abc"},
          {"invalid - starts with underscore", k_invalid_starts_with_underscore_input, make_path_segment(""), false,
           "_hello"},
          {"invalid - empty", k_invalid_empty_input, make_path_segment(""), false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}