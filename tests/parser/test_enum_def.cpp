#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Enum_Def;

PARSE_TEST(Enum_Def, enum_def)

namespace {
// Unit variants
constexpr auto k_unit_single_should_succeed = true;
constexpr auto k_unit_single_input = "enum Color { Red }";
inline auto const k_unit_single_expected = test_sexp::enum_def("Color", {}, {test_sexp::enum_variant("Red")});

constexpr auto k_unit_multiple_should_succeed = true;
constexpr auto k_unit_multiple_input = "enum Color { Red, Green, Blue }";
inline auto const k_unit_multiple_expected = test_sexp::enum_def(
    "Color",
    {},
    {test_sexp::enum_variant("Red"), test_sexp::enum_variant("Green"), test_sexp::enum_variant("Blue")}
);

constexpr auto k_unit_trailing_comma_should_succeed = true;
constexpr auto k_unit_trailing_comma_input = "enum Status { Idle, Running, }";
inline auto const k_unit_trailing_comma_expected =
    test_sexp::enum_def("Status", {}, {test_sexp::enum_variant("Idle"), test_sexp::enum_variant("Running")});

// Tuple variants
constexpr auto k_tuple_single_field_should_succeed = true;
constexpr auto k_tuple_single_field_input = "enum Option { Some(I32) }";
inline auto const k_tuple_single_field_expected =
    test_sexp::enum_def("Option", {}, {test_sexp::enum_variant("Some", {test_sexp::type_name("I32")})});

constexpr auto k_tuple_multiple_fields_should_succeed = true;
constexpr auto k_tuple_multiple_fields_input = "enum Color { Rgb(I32, I32, I32) }";
inline auto const k_tuple_multiple_fields_expected = test_sexp::enum_def(
    "Color",
    {},
    {test_sexp::enum_variant(
        "Rgb",
        {test_sexp::type_name("I32"), test_sexp::type_name("I32"), test_sexp::type_name("I32")}
    )}
);

constexpr auto k_tuple_trailing_comma_should_succeed = true;
constexpr auto k_tuple_trailing_comma_input = "enum Data { Point(I32, I32,) }";
inline auto const k_tuple_trailing_comma_expected = test_sexp::enum_def(
    "Data",
    {},
    {test_sexp::enum_variant("Point", {test_sexp::type_name("I32"), test_sexp::type_name("I32")})}
);

constexpr auto k_struct_single_field_should_succeed = true;
constexpr auto k_struct_single_field_input = "enum Message { Write { text: String } }";
inline auto const k_struct_single_field_expected = test_sexp::enum_def(
    "Message",
    {},
    {test_sexp::enum_variant("Write", {test_sexp::struct_field("text", test_sexp::type_name("String"))})}
);

constexpr auto k_struct_multiple_fields_should_succeed = true;
constexpr auto k_struct_multiple_fields_input = "enum Message { Move { x: I32, y: I32 } }";
inline auto const k_struct_multiple_fields_expected = test_sexp::enum_def(
    "Message",
    {},
    {test_sexp::enum_variant(
        "Move",
        {test_sexp::struct_field("x", test_sexp::type_name("I32")),
         test_sexp::struct_field("y", test_sexp::type_name("I32"))}
    )}
);

// Mixed variants
constexpr auto k_mixed_variants_should_succeed = true;
constexpr auto k_mixed_variants_input = "enum Message { Quit, Move { x: I32, y: I32 }, Write(String) }";
inline auto const k_mixed_variants_expected = test_sexp::enum_def(
    "Message",
    {},
    {test_sexp::enum_variant("Quit"),
     test_sexp::enum_variant(
         "Move",
         {test_sexp::struct_field("x", test_sexp::type_name("I32")),
          test_sexp::struct_field("y", test_sexp::type_name("I32"))}
     ),
     test_sexp::enum_variant("Write", {test_sexp::type_name("String")})}
);

// Generic enums
constexpr auto k_generic_single_param_should_succeed = true;
constexpr auto k_generic_single_param_input = "enum Option<T> { Some(T), None }";
inline auto const k_generic_single_param_expected = test_sexp::enum_def(
    "Option",
    {test_sexp::type_param(test_sexp::type_name("T"))},
    {test_sexp::enum_variant("Some", {test_sexp::type_name("T")}), test_sexp::enum_variant("None")}
);

constexpr auto k_generic_multiple_params_should_succeed = true;
constexpr auto k_generic_multiple_params_input = "enum Result<T, E> { Ok(T), Err(E) }";
inline auto const k_generic_multiple_params_expected = test_sexp::enum_def(
    "Result",
    {test_sexp::type_param(test_sexp::type_name("T")), test_sexp::type_param(test_sexp::type_name("E"))},
    {test_sexp::enum_variant("Ok", {test_sexp::type_name("T")}),
     test_sexp::enum_variant("Err", {test_sexp::type_name("E")})}
);

// Complex nested types
constexpr auto k_nested_types_should_succeed = true;
constexpr auto k_nested_types_input = "enum Tree<T> { Leaf(T), Node(Tree<T>, Tree<T>) }";
inline auto const k_nested_types_expected = test_sexp::enum_def(
    "Tree",
    {test_sexp::type_param(test_sexp::type_name("T"))},
    {test_sexp::enum_variant("Leaf", {test_sexp::type_name("T")}),
     test_sexp::enum_variant(
         "Node",
         {R"((path ((type_segment "Tree" ((path ((type_segment "T"))))))))",
          R"((path ((type_segment "Tree" ((path ((type_segment "T"))))))))"}
     )}
);

// Qualified types in variants
constexpr auto k_qualified_types_should_succeed = true;
constexpr auto k_qualified_types_input = "enum Value { Int(Std.I32), Str(Std.String) }";
inline auto const k_qualified_types_expected = test_sexp::enum_def(
    "Value",
    {},
    {test_sexp::enum_variant("Int", {test_sexp::type_name_path({"Std", "I32"})}),
     test_sexp::enum_variant("Str", {test_sexp::type_name_path({"Std", "String"})})}
);

// Error cases (semantic errors, not parse errors - empty enums would be caught in semantic analysis)
constexpr auto k_empty_variants_should_succeed = true;
constexpr auto k_empty_variants_input = "enum Empty { }";
inline auto const k_empty_variants_expected = test_sexp::enum_def("Empty", {}, {});

constexpr auto k_missing_brace_should_succeed = false;
constexpr auto k_missing_brace_input = "enum Color { Red";
inline auto const k_missing_brace_expected = "";

constexpr auto k_missing_name_should_succeed = false;
constexpr auto k_missing_name_input = "enum { Red, Blue }";
inline auto const k_missing_name_expected = "";

}  // namespace

TEST_CASE("Parse Enum_Def") {
  std::vector<Enum_Def_Params> const params_list = {
      {.name = "unit single",
       .input = k_unit_single_input,
       .expected = k_unit_single_expected,
       .should_succeed = k_unit_single_should_succeed},
      {.name = "unit multiple",
       .input = k_unit_multiple_input,
       .expected = k_unit_multiple_expected,
       .should_succeed = k_unit_multiple_should_succeed},
      {.name = "unit trailing comma",
       .input = k_unit_trailing_comma_input,
       .expected = k_unit_trailing_comma_expected,
       .should_succeed = k_unit_trailing_comma_should_succeed},
      {.name = "tuple single field",
       .input = k_tuple_single_field_input,
       .expected = k_tuple_single_field_expected,
       .should_succeed = k_tuple_single_field_should_succeed},
      {.name = "tuple multiple fields",
       .input = k_tuple_multiple_fields_input,
       .expected = k_tuple_multiple_fields_expected,
       .should_succeed = k_tuple_multiple_fields_should_succeed},
      {.name = "tuple trailing comma",
       .input = k_tuple_trailing_comma_input,
       .expected = k_tuple_trailing_comma_expected,
       .should_succeed = k_tuple_trailing_comma_should_succeed},
      {.name = "struct single field",
       .input = k_struct_single_field_input,
       .expected = k_struct_single_field_expected,
       .should_succeed = k_struct_single_field_should_succeed},
      {.name = "struct multiple fields",
       .input = k_struct_multiple_fields_input,
       .expected = k_struct_multiple_fields_expected,
       .should_succeed = k_struct_multiple_fields_should_succeed},
      {.name = "mixed variants",
       .input = k_mixed_variants_input,
       .expected = k_mixed_variants_expected,
       .should_succeed = k_mixed_variants_should_succeed},
      {.name = "generic single param",
       .input = k_generic_single_param_input,
       .expected = k_generic_single_param_expected,
       .should_succeed = k_generic_single_param_should_succeed},
      {.name = "generic multiple params",
       .input = k_generic_multiple_params_input,
       .expected = k_generic_multiple_params_expected,
       .should_succeed = k_generic_multiple_params_should_succeed},
      {.name = "nested types",
       .input = k_nested_types_input,
       .expected = k_nested_types_expected,
       .should_succeed = k_nested_types_should_succeed},
      {.name = "qualified types",
       .input = k_qualified_types_input,
       .expected = k_qualified_types_expected,
       .should_succeed = k_qualified_types_should_succeed},
      {.name = "empty variants error",
       .input = k_empty_variants_input,
       .expected = k_empty_variants_expected,
       .should_succeed = k_empty_variants_should_succeed},
      {.name = "missing brace error",
       .input = k_missing_brace_input,
       .expected = k_missing_brace_expected,
       .should_succeed = k_missing_brace_should_succeed},
      {.name = "missing name error",
       .input = k_missing_name_input,
       .expected = k_missing_name_expected,
       .should_succeed = k_missing_name_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
