#include "utils.hpp"

using life_lang::ast::Function_Declaration;
using life_lang::ast::make_function_declaration;
using life_lang::ast::make_function_parameter;
using life_lang::ast::make_path;
using life_lang::ast::make_path_segment;

PARSE_TEST(Function_Declaration, function_declaration)

namespace {
// Basic declarations
constexpr auto k_no_parameters_input = "fn foo(): Int";

// Parameter variations
constexpr auto k_one_parameter_input = "fn foo(x: Int): Int";
auto make_one_parameter_expected() {
  return make_function_declaration("foo", {make_function_parameter("x", make_path("Int"))}, make_path("Int"));
}

constexpr auto k_two_parameters_input = "fn foo(hello: T, world: U): Int";
auto make_two_parameters_expected() {
  return make_function_declaration(
      "foo", {make_function_parameter("hello", make_path("T")), make_function_parameter("world", make_path("U"))},
      make_path("Int")
  );
}

// Return type variations
constexpr auto k_namespace_return_type_input = "fn foo(): Std.String";
auto make_namespace_return_type_expected() { return make_function_declaration("foo", {}, make_path("Std", "String")); }

constexpr auto k_template_return_type_input = "fn foo(): Vec<Int>";
auto make_template_return_type_expected() {
  return make_function_declaration("foo", {}, make_path(make_path_segment("Vec", {make_path("Int")})));
}

// Complex template types
constexpr auto k_complex_templates_input = "fn foo(hello: A.B.Hello<Std.Array, B.C<Int, Double>>): A.B.C<Int>";
auto make_complex_templates_expected() {
  return make_function_declaration(
      "foo",
      {make_function_parameter(
          "hello", make_path(
                       "A", "B",
                       make_path_segment(
                           "Hello", {make_path("Std", "Array"),
                                     make_path("B", make_path_segment("C", {make_path("Int"), make_path("Double")}))}
                       )
                   )
      )},
      make_path("A", "B", make_path_segment("C", {make_path("Int")}))
  );
}

// Whitespace handling
constexpr auto k_with_spaces_input = "fn  foo  (  x  :  Int  )  :  Int";
auto make_with_spaces_expected() {
  return make_function_declaration("foo", {make_function_parameter("x", make_path("Int"))}, make_path("Int"));
}

// Trailing content
constexpr auto k_trailing_body_input = "fn foo(): Int {";
auto make_trailing_body_expected() { return make_function_declaration("foo", {}, make_path("Int")); }

// Invalid cases
constexpr auto k_invalid_no_fn_keyword_input = "foo(): Int";
constexpr auto k_invalid_no_return_type_input = "fn foo()";
constexpr auto k_invalid_no_parentheses_input = "fn foo: Int";
auto make_invalid_expected() { return make_function_declaration("", {}, make_path()); }
}  // namespace

TEST_CASE("Parse Function_Declaration", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Function_Declaration_Params>({
          // Basic declarations
          {"no parameters", k_no_parameters_input, make_function_declaration("foo", {}, make_path("Int")), true, ""},

          // Parameter variations
          {"one parameter", k_one_parameter_input, make_one_parameter_expected(), true, ""},
          {"two parameters", k_two_parameters_input, make_two_parameters_expected(), true, ""},

          // Return type variations
          {"namespace return type", k_namespace_return_type_input, make_namespace_return_type_expected(), true, ""},
          {"template return type", k_template_return_type_input, make_template_return_type_expected(), true, ""},

          // Complex template types
          {"complex templates", k_complex_templates_input, make_complex_templates_expected(), true, ""},

          // Whitespace handling
          {"with spaces", k_with_spaces_input, make_with_spaces_expected(), true, ""},

          // Trailing content
          {"trailing body", k_trailing_body_input, make_trailing_body_expected(), true, "{"},

          // Invalid cases
          {"invalid - no fn keyword", k_invalid_no_fn_keyword_input, make_invalid_expected(), false, "foo(): Int"},
          {"invalid - no return type", k_invalid_no_return_type_input, make_invalid_expected(), false, ""},
          {"invalid - no parentheses", k_invalid_no_parentheses_input, make_invalid_expected(), false, ": Int"},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}