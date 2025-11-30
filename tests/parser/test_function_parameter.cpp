#include "utils.hpp"

using life_lang::ast::Function_Parameter;
using life_lang::ast::make_function_parameter;
using life_lang::ast::make_path;
using life_lang::ast::make_path_segment;

PARSE_TEST(Function_Parameter, function_parameter)

namespace {
// Simple types
constexpr auto k_simple_type_input = "x: Int";

// Whitespace variations
constexpr auto k_no_spaces_input = "hello:T";

// Qualified types
constexpr auto k_with_namespace_input = "arg: Std.String";

// Template types
constexpr auto k_template_parameter_input = "vec: Vec<Int>";
auto make_template_parameter_expected() {
  return make_function_parameter("vec", make_path(make_path_segment("Vec", {make_path("Int")})));
}

constexpr auto k_complex_nested_templates_input = "hello: A.B.Hello<Std.Array, A.B.C<Int, Double>>";
auto make_complex_nested_templates_expected() {
  return make_function_parameter(
      "hello", make_path(
                   "A", "B",
                   make_path_segment(
                       "Hello", {make_path("Std", "Array"),
                                 make_path("A", "B", make_path_segment("C", {make_path("Int"), make_path("Double")}))}
                   )
               )
  );
}

// Trailing content
constexpr auto k_with_trailing_comma_input = "x: Int,";

// Invalid cases
constexpr auto k_invalid_no_colon_input = "x Int";
constexpr auto k_invalid_no_type_input = "x:";
constexpr auto k_invalid_no_name_input = ": Int";
constexpr auto k_invalid_empty_input = "";
auto make_invalid_expected() { return make_function_parameter("", make_path()); }
}  // namespace

TEST_CASE("Parse Function_Parameter", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Function_Parameter_Params>({
          // Simple types
          {"simple type", k_simple_type_input, make_function_parameter("x", make_path("Int")), true, ""},

          // Whitespace variations
          {"no spaces", k_no_spaces_input, make_function_parameter("hello", make_path("T")), true, ""},

          // Qualified types
          {"with namespace", k_with_namespace_input, make_function_parameter("arg", make_path("Std", "String")), true,
           ""},

          // Template types
          {"template parameter", k_template_parameter_input, make_template_parameter_expected(), true, ""},
          {"complex nested templates", k_complex_nested_templates_input, make_complex_nested_templates_expected(), true,
           ""},

          // Trailing content
          {"with trailing comma", k_with_trailing_comma_input, make_function_parameter("x", make_path("Int")), true,
           ","},

          // Invalid cases
          {"invalid - no colon", k_invalid_no_colon_input, make_invalid_expected(), false, "Int"},
          {"invalid - no type", k_invalid_no_type_input, make_invalid_expected(), false, ""},
          {"invalid - no name", k_invalid_no_name_input, make_invalid_expected(), false, ": Int"},
          {"invalid - empty", k_invalid_empty_input, make_invalid_expected(), false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}