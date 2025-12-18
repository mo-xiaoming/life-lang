#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Type_Name;

PARSE_TEST(Type_Name, type_name)

namespace {

// Single element with trailing comma
constexpr auto k_single_element_trailing_comma_should_succeed = true;
constexpr auto k_single_element_trailing_comma_input = "(I32,)";
inline auto const k_single_element_trailing_comma_expected = R"((tuple_type ((path ((type_segment "I32"))))))";

// Two elements
constexpr auto k_two_elements_should_succeed = true;
constexpr auto k_two_elements_input = "(I32, String)";
inline auto const k_two_elements_expected =
    R"((tuple_type ((path ((type_segment "I32"))) (path ((type_segment "String"))))))";

// Three elements
constexpr auto k_three_elements_should_succeed = true;
constexpr auto k_three_elements_input = "(I32, String, Bool)";
inline auto const k_three_elements_expected =
    R"((tuple_type ((path ((type_segment "I32"))) (path ((type_segment "String"))) (path ((type_segment "Bool"))))))";

// Multiple elements with trailing comma
constexpr auto k_multiple_trailing_comma_should_succeed = true;
constexpr auto k_multiple_trailing_comma_input = "(I32, String, Bool,)";
inline auto const k_multiple_trailing_comma_expected =
    R"((tuple_type ((path ((type_segment "I32"))) (path ((type_segment "String"))) (path ((type_segment "Bool"))))))";

// Nested tuple types
constexpr auto k_nested_tuples_should_succeed = true;
constexpr auto k_nested_tuples_input = "((I32, I32), String)";
inline auto const k_nested_tuples_expected =
    R"((tuple_type ((tuple_type ((path ((type_segment "I32"))) (path ((type_segment "I32"))))) (path ((type_segment "String"))))))";

// Tuple with qualified types
constexpr auto k_with_qualified_types_should_succeed = true;
constexpr auto k_with_qualified_types_input = "(Std.String, Std.Vec)";
inline auto const k_with_qualified_types_expected =
    R"((tuple_type ((path ((type_segment "Std") (type_segment "String"))) (path ((type_segment "Std") (type_segment "Vec"))))))";

// Tuple with generic types
constexpr auto k_with_generic_types_should_succeed = true;
constexpr auto k_with_generic_types_input = "(Vec<I32>, Map<String, I32>)";
inline auto const k_with_generic_types_expected =
    R"((tuple_type ((path ((type_segment "Vec" ((path ((type_segment "I32"))))))) (path ((type_segment "Map" ((path ((type_segment "String"))) (path ((type_segment "I32"))))))))))";

// Tuple with array types
constexpr auto k_with_array_types_should_succeed = true;
constexpr auto k_with_array_types_input = "([I32; 4], [String; 10])";
inline auto const k_with_array_types_expected =
    R"((tuple_type ((array_type (path ((type_segment "I32"))) "4") (array_type (path ((type_segment "String"))) "10"))))";

// Tuple with function types
constexpr auto k_with_function_types_should_succeed = true;
constexpr auto k_with_function_types_input = "(fn(I32): Bool, fn(): ())";
inline auto const k_with_function_types_expected =
    R"foo((tuple_type ((func_type ((path ((type_segment "I32")))) (path ((type_segment "Bool")))) (func_type () (path ((type_segment "()")))))))foo";

// Large tuple (5 elements)
constexpr auto k_large_tuple_should_succeed = true;
constexpr auto k_large_tuple_input = "(I32, String, Bool, F64, Char)";
inline auto const k_large_tuple_expected =
    R"((tuple_type ((path ((type_segment "I32"))) (path ((type_segment "String"))) (path ((type_segment "Bool"))) (path ((type_segment "F64"))) (path ((type_segment "Char"))))))";

// Complex nested tuple with generics
constexpr auto k_complex_nested_should_succeed = true;
constexpr auto k_complex_nested_input = "((Vec<I32>, String), (Bool, Map<String, I32>))";
inline auto const k_complex_nested_expected =
    R"((tuple_type ((tuple_type ((path ((type_segment "Vec" ((path ((type_segment "I32"))))))) (path ((type_segment "String"))))) (tuple_type ((path ((type_segment "Bool"))) (path ((type_segment "Map" ((path ((type_segment "String"))) (path ((type_segment "I32"))))))))))))";
// === Parenthesized Type Tests (NOT tuples) ===

// Single element without trailing comma - parenthesized type
constexpr auto k_parenthesized_type_should_succeed = true;
constexpr auto k_parenthesized_type_input = "(I32)";
inline auto const k_parenthesized_type_expected =
    R"((path ((type_segment "I32"))))";  // Just the type, no tuple wrapper

// Complex parenthesized type with generics
constexpr auto k_complex_parenthesized_should_succeed = true;
constexpr auto k_complex_parenthesized_input = "(Vec<I32>)";
inline auto const k_complex_parenthesized_expected = R"((path ((type_segment "Vec" ((path ((type_segment "I32"))))))))";

// === Invalid Tuple Types ===

// Missing closing paren
constexpr auto k_missing_closing_paren_should_succeed = false;
constexpr auto k_missing_closing_paren_input = "(I32, String";

// Missing opening paren
constexpr auto k_missing_opening_paren_should_succeed = false;
constexpr auto k_missing_opening_paren_input = "I32, String)";

// Double comma
constexpr auto k_double_comma_should_succeed = false;
constexpr auto k_double_comma_input = "(I32,, String)";

// Leading comma
constexpr auto k_leading_comma_should_succeed = false;
constexpr auto k_leading_comma_input = "(, I32, String)";

// Empty tuple (should parse as unit type, not tuple type)
constexpr auto k_empty_tuple_should_succeed = true;
constexpr auto k_empty_tuple_input = "()";
inline auto const k_empty_tuple_expected = R"foo((path ((type_segment "()"))))foo";  // Unit type, not tuple

}  // namespace

TEST_CASE("Parse Tuple_Type") {
  std::vector<Type_Name_Params> const params_list = {
      {.name = "single element with trailing comma",
       .input = k_single_element_trailing_comma_input,
       .expected = k_single_element_trailing_comma_expected,
       .should_succeed = k_single_element_trailing_comma_should_succeed},
      {.name = "two elements",
       .input = k_two_elements_input,
       .expected = k_two_elements_expected,
       .should_succeed = k_two_elements_should_succeed},
      {.name = "three elements",
       .input = k_three_elements_input,
       .expected = k_three_elements_expected,
       .should_succeed = k_three_elements_should_succeed},
      {.name = "multiple with trailing comma",
       .input = k_multiple_trailing_comma_input,
       .expected = k_multiple_trailing_comma_expected,
       .should_succeed = k_multiple_trailing_comma_should_succeed},
      {.name = "nested tuples",
       .input = k_nested_tuples_input,
       .expected = k_nested_tuples_expected,
       .should_succeed = k_nested_tuples_should_succeed},
      {.name = "with qualified types",
       .input = k_with_qualified_types_input,
       .expected = k_with_qualified_types_expected,
       .should_succeed = k_with_qualified_types_should_succeed},
      {.name = "with generic types",
       .input = k_with_generic_types_input,
       .expected = k_with_generic_types_expected,
       .should_succeed = k_with_generic_types_should_succeed},
      {.name = "with array types",
       .input = k_with_array_types_input,
       .expected = k_with_array_types_expected,
       .should_succeed = k_with_array_types_should_succeed},
      {.name = "with function types",
       .input = k_with_function_types_input,
       .expected = k_with_function_types_expected,
       .should_succeed = k_with_function_types_should_succeed},
      {.name = "large tuple",
       .input = k_large_tuple_input,
       .expected = k_large_tuple_expected,
       .should_succeed = k_large_tuple_should_succeed},
      {.name = "complex nested",
       .input = k_complex_nested_input,
       .expected = k_complex_nested_expected,
       .should_succeed = k_complex_nested_should_succeed},
      // Parenthesized types (NOT tuples)
      {.name = "parenthesized type",
       .input = k_parenthesized_type_input,
       .expected = k_parenthesized_type_expected,
       .should_succeed = k_parenthesized_type_should_succeed},
      {.name = "complex parenthesized",
       .input = k_complex_parenthesized_input,
       .expected = k_complex_parenthesized_expected,
       .should_succeed = k_complex_parenthesized_should_succeed},
      // Invalid cases
      {.name = "missing closing paren",
       .input = k_missing_closing_paren_input,
       .expected = "",
       .should_succeed = k_missing_closing_paren_should_succeed},
      {.name = "missing opening paren",
       .input = k_missing_opening_paren_input,
       .expected = "",
       .should_succeed = k_missing_opening_paren_should_succeed},
      {.name = "double comma",
       .input = k_double_comma_input,
       .expected = "",
       .should_succeed = k_double_comma_should_succeed},
      {.name = "leading comma",
       .input = k_leading_comma_input,
       .expected = "",
       .should_succeed = k_leading_comma_should_succeed},
      {.name = "empty tuple (unit type)",
       .input = k_empty_tuple_input,
       .expected = k_empty_tuple_expected,
       .should_succeed = k_empty_tuple_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
