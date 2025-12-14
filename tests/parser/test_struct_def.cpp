#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Struct_Def;

PARSE_TEST(Struct_Def, struct_def)

namespace {
// Empty struct
constexpr auto k_empty_struct_should_succeed = true;
constexpr auto k_empty_struct_input = "struct Empty {}";
inline auto const k_empty_struct_expected = test_sexp::struct_def("Empty", {});

// Single field
constexpr auto k_single_field_should_succeed = true;
constexpr auto k_single_field_input = "struct Point { x: I32 }";
inline auto const k_single_field_expected =
    test_sexp::struct_def("Point", {test_sexp::struct_field("x", test_sexp::type_name("I32"))});

// Two fields
constexpr auto k_two_fields_should_succeed = true;
constexpr auto k_two_fields_input = "struct Point { x: I32, y: I32 }";
inline auto const k_two_fields_expected = test_sexp::struct_def(
    "Point",
    {test_sexp::struct_field("x", test_sexp::type_name("I32")),
     test_sexp::struct_field("y", test_sexp::type_name("I32"))}
);

// Multiple fields
constexpr auto k_multiple_fields_should_succeed = true;
constexpr auto k_multiple_fields_input = "struct Person { name: String, age: I32, active: Bool }";
inline auto const k_multiple_fields_expected = test_sexp::struct_def(
    "Person",
    {test_sexp::struct_field("name", test_sexp::type_name("String")),
     test_sexp::struct_field("age", test_sexp::type_name("I32")),
     test_sexp::struct_field("active", test_sexp::type_name("Bool"))}
);

// Qualified types
constexpr auto k_qualified_types_should_succeed = true;
constexpr auto k_qualified_types_input = "struct Data { value: Std.String, count: Std.I32 }";
inline auto const k_qualified_types_expected = test_sexp::struct_def(
    "Data",
    {test_sexp::struct_field("value", test_sexp::type_name_path({"Std", "String"})),
     test_sexp::struct_field("count", test_sexp::type_name_path({"Std", "I32"}))}
);

// Template types
constexpr auto k_template_types_should_succeed = true;
constexpr auto k_template_types_input = "struct Container { items: Vec<I32>, names: Array<String> }";
inline auto const k_template_types_expected = test_sexp::struct_def(
    "Container",
    {test_sexp::struct_field("items", R"((path ((type_segment "Vec" ((path ((type_segment "I32"))))))))"),
     test_sexp::struct_field("names", R"((path ((type_segment "Array" ((path ((type_segment "String"))))))))")}
);

// Complex nested templates
constexpr auto k_complex_nested_should_succeed = true;
constexpr auto k_complex_nested_input = "struct Complex { data: Map<String, Vec<I32>> }";
inline auto const k_complex_nested_expected = test_sexp::struct_def(
    "Complex",
    {test_sexp::struct_field(
        "data",
        "(path ((type_segment \"Map\" ((path ((type_segment \"String\"))) (path ((type_segment \"Vec\" ((path "
        "((type_segment \"I32\")))))))))))"
    )}
);

// Whitespace variations
constexpr auto k_no_spaces_should_succeed = true;
constexpr auto k_no_spaces_input = "struct Foo{x:I32,y:I32}";
inline auto const k_no_spaces_expected = test_sexp::struct_def(
    "Foo",
    {test_sexp::struct_field("x", test_sexp::type_name("I32")),
     test_sexp::struct_field("y", test_sexp::type_name("I32"))}
);

constexpr auto k_multiline_should_succeed = true;
constexpr auto k_multiline_input = R"(struct Point {
  x: I32,
  y: I32
})";
inline auto const k_multiline_expected = test_sexp::struct_def(
    "Point",
    {test_sexp::struct_field("x", test_sexp::type_name("I32")),
     test_sexp::struct_field("y", test_sexp::type_name("I32"))}
);

// Trailing comma (should be allowed)
constexpr auto k_trailing_comma_should_succeed = true;
constexpr auto k_trailing_comma_input = "struct Point { x: I32, y: I32, }";
inline auto const k_trailing_comma_expected = test_sexp::struct_def(
    "Point",
    {test_sexp::struct_field("x", test_sexp::type_name("I32")),
     test_sexp::struct_field("y", test_sexp::type_name("I32"))}
);

// Struct name variations (Camel_Snake_Case)
constexpr auto k_camel_case_name_should_succeed = true;
constexpr auto k_camel_case_name_input = "struct MyStruct { value: I32 }";
inline auto const k_camel_case_name_expected =
    test_sexp::struct_def("MyStruct", {test_sexp::struct_field("value", test_sexp::type_name("I32"))});

constexpr auto k_camel_snake_case_name_should_succeed = true;
constexpr auto k_camel_snake_case_name_input = "struct My_Struct { value: I32 }";
inline auto const k_camel_snake_case_name_expected =
    test_sexp::struct_def("My_Struct", {test_sexp::struct_field("value", test_sexp::type_name("I32"))});

constexpr auto k_http_response_name_should_succeed = true;
constexpr auto k_http_response_name_input = "struct HTTP_Response { code: I32 }";
inline auto const k_http_response_name_expected =
    test_sexp::struct_def("HTTP_Response", {test_sexp::struct_field("code", test_sexp::type_name("I32"))});

// Trailing content
constexpr auto k_with_trailing_content_should_succeed = false;
constexpr auto k_with_trailing_content_input = "struct Point { x: I32 } fn";
inline auto const k_with_trailing_content_expected =
    test_sexp::struct_def("Point", {test_sexp::struct_field("x", test_sexp::type_name("I32"))});

// Parser accepts any identifier - naming conventions checked at semantic analysis
constexpr auto k_lowercase_name_accepted_should_succeed = true;
constexpr auto k_lowercase_name_accepted_input = "struct point { x: I32 }";
inline auto const k_lowercase_name_accepted_expected =
    test_sexp::struct_def("point", {test_sexp::struct_field("x", test_sexp::type_name("I32"))});

// Generic structs with type parameters
constexpr auto k_generic_single_param_should_succeed = true;
constexpr auto k_generic_single_param_input = "struct Box<T> { value: T }";
inline auto const k_generic_single_param_expected = test_sexp::struct_def(
    "Box",
    {"(type_param (path ((type_segment \"T\"))))"},
    {test_sexp::struct_field("value", test_sexp::type_name("T"))}
);

constexpr auto k_generic_two_params_should_succeed = true;
constexpr auto k_generic_two_params_input = "struct Pair<T, U> { first: T, second: U }";
inline auto const k_generic_two_params_expected = test_sexp::struct_def(
    "Pair",
    {"(type_param (path ((type_segment \"T\"))))", "(type_param (path ((type_segment \"U\"))))"},
    {test_sexp::struct_field("first", test_sexp::type_name("T")),
     test_sexp::struct_field("second", test_sexp::type_name("U"))}
);

constexpr auto k_generic_map_should_succeed = true;
constexpr auto k_generic_map_input = "struct Map<K, V> { keys: Vec<K>, values: Vec<V> }";
inline auto const k_generic_map_expected = test_sexp::struct_def(
    "Map",
    {"(type_param (path ((type_segment \"K\"))))", "(type_param (path ((type_segment \"V\"))))"},
    {test_sexp::struct_field("keys", R"((path ((type_segment "Vec" ((path ((type_segment "K"))))))))"),
     test_sexp::struct_field("values", R"((path ((type_segment "Vec" ((path ((type_segment "V"))))))))")}
);

constexpr auto k_generic_empty_should_succeed = true;
constexpr auto k_generic_empty_input = "struct Empty<T> {}";
inline auto const k_generic_empty_expected =
    test_sexp::struct_def("Empty", {"(type_param (path ((type_segment \"T\"))))"}, {});

// Invalid cases
constexpr auto k_invalid_no_name_should_succeed = false;
constexpr auto k_invalid_no_name_input = "struct { x: I32 }";
constexpr auto k_invalid_no_name_expected = R"({"Struct_Def": {"fields": [], "name": ""}})";

constexpr auto k_invalid_no_braces_should_succeed = false;
constexpr auto k_invalid_no_braces_input = "struct Point";
constexpr auto k_invalid_no_braces_expected = R"({"Struct_Def": {"fields": [], "name": ""}})";

constexpr auto k_invalid_missing_closing_should_succeed = false;
constexpr auto k_invalid_missing_closing_input = "struct Point { x: I32";
constexpr auto k_invalid_missing_closing_expected = R"({"Struct_Def": {"fields": [], "name": ""}})";

constexpr auto k_invalid_missing_field_type_should_succeed = false;
constexpr auto k_invalid_missing_field_type_input = "struct Point { x: }";
constexpr auto k_invalid_missing_field_type_expected = R"({"Struct_Def": {"fields": [], "name": ""}})";

constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";
constexpr auto k_invalid_empty_expected = R"({"Struct_Def": {"fields": [], "name": ""}})";
}  // namespace

TEST_CASE("Parse Struct_Def") {
  std::vector<Struct_Def_Params> const params_list = {
      {.name = "empty struct",
       .input = k_empty_struct_input,
       .expected = k_empty_struct_expected,
       .should_succeed = k_empty_struct_should_succeed},
      {.name = "single field",
       .input = k_single_field_input,
       .expected = k_single_field_expected,
       .should_succeed = k_single_field_should_succeed},
      {.name = "two fields",
       .input = k_two_fields_input,
       .expected = k_two_fields_expected,
       .should_succeed = k_two_fields_should_succeed},
      {.name = "multiple fields",
       .input = k_multiple_fields_input,
       .expected = k_multiple_fields_expected,
       .should_succeed = k_multiple_fields_should_succeed},
      {.name = "qualified types",
       .input = k_qualified_types_input,
       .expected = k_qualified_types_expected,
       .should_succeed = k_qualified_types_should_succeed},
      {.name = "template types",
       .input = k_template_types_input,
       .expected = k_template_types_expected,
       .should_succeed = k_template_types_should_succeed},
      {.name = "complex nested",
       .input = k_complex_nested_input,
       .expected = k_complex_nested_expected,
       .should_succeed = k_complex_nested_should_succeed},
      {.name = "no spaces",
       .input = k_no_spaces_input,
       .expected = k_no_spaces_expected,
       .should_succeed = k_no_spaces_should_succeed},
      {.name = "multiline",
       .input = k_multiline_input,
       .expected = k_multiline_expected,
       .should_succeed = k_multiline_should_succeed},
      {.name = "trailing comma",
       .input = k_trailing_comma_input,
       .expected = k_trailing_comma_expected,
       .should_succeed = k_trailing_comma_should_succeed},
      {.name = "camel case name",
       .input = k_camel_case_name_input,
       .expected = k_camel_case_name_expected,
       .should_succeed = k_camel_case_name_should_succeed},
      {.name = "camel snake case name",
       .input = k_camel_snake_case_name_input,
       .expected = k_camel_snake_case_name_expected,
       .should_succeed = k_camel_snake_case_name_should_succeed},
      {.name = "HTTP response name",
       .input = k_http_response_name_input,
       .expected = k_http_response_name_expected,
       .should_succeed = k_http_response_name_should_succeed},
      {.name = "with trailing content",
       .input = k_with_trailing_content_input,
       .expected = k_with_trailing_content_expected,
       .should_succeed = k_with_trailing_content_should_succeed},
      {.name = "lowercase name accepted",
       .input = k_lowercase_name_accepted_input,
       .expected = k_lowercase_name_accepted_expected,
       .should_succeed = k_lowercase_name_accepted_should_succeed},
      {.name = "generic single param",
       .input = k_generic_single_param_input,
       .expected = k_generic_single_param_expected,
       .should_succeed = k_generic_single_param_should_succeed},
      {.name = "generic two params",
       .input = k_generic_two_params_input,
       .expected = k_generic_two_params_expected,
       .should_succeed = k_generic_two_params_should_succeed},
      {.name = "generic map",
       .input = k_generic_map_input,
       .expected = k_generic_map_expected,
       .should_succeed = k_generic_map_should_succeed},
      {.name = "generic empty",
       .input = k_generic_empty_input,
       .expected = k_generic_empty_expected,
       .should_succeed = k_generic_empty_should_succeed},
      {.name = "invalid - no name",
       .input = k_invalid_no_name_input,
       .expected = k_invalid_no_name_expected,
       .should_succeed = k_invalid_no_name_should_succeed},
      {.name = "invalid - no braces",
       .input = k_invalid_no_braces_input,
       .expected = k_invalid_no_braces_expected,
       .should_succeed = k_invalid_no_braces_should_succeed},
      {.name = "invalid - missing closing",
       .input = k_invalid_missing_closing_input,
       .expected = k_invalid_missing_closing_expected,
       .should_succeed = k_invalid_missing_closing_should_succeed},
      {.name = "invalid - missing field type",
       .input = k_invalid_missing_field_type_input,
       .expected = k_invalid_missing_field_type_expected,
       .should_succeed = k_invalid_missing_field_type_should_succeed},
      {.name = "invalid - empty",
       .input = k_invalid_empty_input,
       .expected = k_invalid_empty_expected,
       .should_succeed = k_invalid_empty_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      INFO(params.name);
      check_parse(params);
    }
  }
}
