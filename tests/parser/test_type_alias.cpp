#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Type_Alias;

PARSE_TEST(Type_Alias, type_alias)

namespace {
// Simple type alias
constexpr auto k_simple_alias_should_succeed = true;
constexpr auto k_simple_alias_input = "type My_Type = I32;";
inline auto const k_simple_alias_expected = test_sexp::type_alias("My_Type", {}, test_sexp::type_name("I32"));

// Generic type alias with single parameter
constexpr auto k_generic_single_param_should_succeed = true;
constexpr auto k_generic_single_param_input = "type String_Map<T> = Map<String, T>;";
inline auto const k_generic_single_param_expected = test_sexp::type_alias(
    "String_Map",
    {test_sexp::type_param(test_sexp::type_name("T"))},
    R"((path ((type_segment "Map" ((path ((type_segment "String"))) (path ((type_segment "T"))))))))"
);

// Generic type alias with multiple parameters
constexpr auto k_generic_multi_param_should_succeed = true;
constexpr auto k_generic_multi_param_input = "type Pair<A, B> = Tuple<A, B>;";
inline auto const k_generic_multi_param_expected = test_sexp::type_alias(
    "Pair",
    {test_sexp::type_param(test_sexp::type_name("A")), test_sexp::type_param(test_sexp::type_name("B"))},
    R"((path ((type_segment "Tuple" ((path ((type_segment "A"))) (path ((type_segment "B"))))))))"
);

// Qualified type path
constexpr auto k_qualified_path_should_succeed = true;
constexpr auto k_qualified_path_input = "type My_String = Std.String;";
inline auto const k_qualified_path_expected =
    test_sexp::type_alias("My_String", {}, test_sexp::type_name_path({"Std", "String"}));

// Nested generic types
constexpr auto k_nested_generics_should_succeed = true;
constexpr auto k_nested_generics_input = "type Result_List<T, E> = Vec<Result<T, E>>;";
inline auto const k_nested_generics_expected = test_sexp::type_alias(
    "Result_List",
    {test_sexp::type_param(test_sexp::type_name("T")), test_sexp::type_param(test_sexp::type_name("E"))},
    "(path ((type_segment \"Vec\" ((path ((type_segment \"Result\" ((path ((type_segment \"T\"))) (path ((type_segment "
    "\"E\")))))))))))"
);

// Type parameter with trait bounds
constexpr auto k_type_param_with_bounds_should_succeed = true;
constexpr auto k_type_param_with_bounds_input = "type Sorted_Vec<T: Ord> = Vec<T>;";
inline auto const k_type_param_with_bounds_expected = test_sexp::type_alias(
    "Sorted_Vec",
    {R"((type_param (path ((type_segment "T"))) ((trait_bound (path ((type_segment "Ord")))))))"},
    R"((path ((type_segment "Vec" ((path ((type_segment "T"))))))))"
);

// Multiple trait bounds
constexpr auto k_multiple_bounds_should_succeed = true;
constexpr auto k_multiple_bounds_input = "type Display_Vec<T: Display + Clone> = Vec<T>;";
inline auto const k_multiple_bounds_expected = test_sexp::type_alias(
    "Display_Vec",
    {"(type_param (path ((type_segment \"T\"))) ((trait_bound (path ((type_segment \"Display\")))) (trait_bound (path "
     "((type_segment \"Clone\"))))))"},
    R"((path ((type_segment "Vec" ((path ((type_segment "T"))))))))"
);

// Trailing content (parser stops at semicolon)
constexpr auto k_with_trailing_content_should_succeed = false;
constexpr auto k_with_trailing_content_input = "type My_Int = I32; fn";
inline auto const k_with_trailing_content_expected = test_sexp::type_alias("My_Int", {}, test_sexp::type_name("I32"));

// Error: Missing semicolon
constexpr auto k_missing_semicolon_should_succeed = false;
constexpr auto k_missing_semicolon_input = "type My_Type = I32";
inline auto const k_missing_semicolon_expected = "";

// Error: Missing equals sign
constexpr auto k_missing_equals_should_succeed = false;
constexpr auto k_missing_equals_input = "type My_Type I32;";
inline auto const k_missing_equals_expected = "";

// Error: Missing type name
constexpr auto k_missing_type_should_succeed = false;
constexpr auto k_missing_type_input = "type My_Type = ;";
inline auto const k_missing_type_expected = "";

// Error: Missing alias name
constexpr auto k_missing_name_should_succeed = false;
constexpr auto k_missing_name_input = "type = I32;";
inline auto const k_missing_name_expected = "";

}  // namespace

TEST_CASE("Parse Type_Alias") {
  std::vector<Type_Alias_Params> const params_list = {
      {.name = "simple alias",
       .input = k_simple_alias_input,
       .expected = k_simple_alias_expected,
       .should_succeed = k_simple_alias_should_succeed},
      {.name = "generic single parameter",
       .input = k_generic_single_param_input,
       .expected = k_generic_single_param_expected,
       .should_succeed = k_generic_single_param_should_succeed},
      {.name = "generic multiple parameters",
       .input = k_generic_multi_param_input,
       .expected = k_generic_multi_param_expected,
       .should_succeed = k_generic_multi_param_should_succeed},
      {.name = "qualified path",
       .input = k_qualified_path_input,
       .expected = k_qualified_path_expected,
       .should_succeed = k_qualified_path_should_succeed},
      {.name = "nested generics",
       .input = k_nested_generics_input,
       .expected = k_nested_generics_expected,
       .should_succeed = k_nested_generics_should_succeed},
      {.name = "type parameter with bounds",
       .input = k_type_param_with_bounds_input,
       .expected = k_type_param_with_bounds_expected,
       .should_succeed = k_type_param_with_bounds_should_succeed},
      {.name = "multiple trait bounds",
       .input = k_multiple_bounds_input,
       .expected = k_multiple_bounds_expected,
       .should_succeed = k_multiple_bounds_should_succeed},
      {.name = "with trailing content",
       .input = k_with_trailing_content_input,
       .expected = k_with_trailing_content_expected,
       .should_succeed = k_with_trailing_content_should_succeed},
      {.name = "missing semicolon",
       .input = k_missing_semicolon_input,
       .expected = k_missing_semicolon_expected,
       .should_succeed = k_missing_semicolon_should_succeed},
      {.name = "missing equals",
       .input = k_missing_equals_input,
       .expected = k_missing_equals_expected,
       .should_succeed = k_missing_equals_should_succeed},
      {.name = "missing type",
       .input = k_missing_type_input,
       .expected = k_missing_type_expected,
       .should_succeed = k_missing_type_should_succeed},
      {.name = "missing name",
       .input = k_missing_name_input,
       .expected = k_missing_name_expected,
       .should_succeed = k_missing_name_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
