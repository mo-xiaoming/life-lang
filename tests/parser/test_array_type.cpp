#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Array_Type;

PARSE_TEST(Array_Type, array_type)

namespace {

// Simple array type with primitive
constexpr auto k_simple_primitive_should_succeed = true;
constexpr auto k_simple_primitive_input = "[I32; 4]";
inline std::string const k_simple_primitive_expected = test_sexp::array_type(test_sexp::type_name("I32"), "4");

// Array of strings
constexpr auto k_string_array_should_succeed = true;
constexpr auto k_string_array_input = "[String; 10]";
inline std::string const k_string_array_expected = test_sexp::array_type(test_sexp::type_name("String"), "10");

// Array with qualified type
constexpr auto k_qualified_type_should_succeed = true;
constexpr auto k_qualified_type_input = "[Std.String; 5]";
inline std::string const k_qualified_type_expected =
    test_sexp::array_type(test_sexp::type_name_path({"Std", "String"}), "5");

// Array with generic type
constexpr auto k_generic_type_should_succeed = true;
constexpr auto k_generic_type_input = "[Vec<I32>; 3]";
inline auto const k_generic_type_expected =
    R"((array_type (path ((type_segment "Vec" ((path ((type_segment "I32"))))))) "3"))";

// Nested array (array of arrays)
constexpr auto k_nested_array_should_succeed = true;
constexpr auto k_nested_array_input = "[[I32; 4]; 3]";
inline std::string const k_nested_array_expected =
    test_sexp::array_type(test_sexp::array_type(test_sexp::type_name("I32"), "4"), "3");

// Large array size
constexpr auto k_large_size_should_succeed = true;
constexpr auto k_large_size_input = "[I32; 1000]";
inline std::string const k_large_size_expected = test_sexp::array_type(test_sexp::type_name("I32"), "1000");

// Array with size 0
constexpr auto k_zero_size_should_succeed = true;
constexpr auto k_zero_size_input = "[I32; 0]";
inline std::string const k_zero_size_expected = test_sexp::array_type(test_sexp::type_name("I32"), "0");

// Array with size 1
constexpr auto k_size_one_should_succeed = true;
constexpr auto k_size_one_input = "[Bool; 1]";
inline std::string const k_size_one_expected = test_sexp::array_type(test_sexp::type_name("Bool"), "1");

// Unsized array - basic
constexpr auto k_unsized_basic_should_succeed = true;
constexpr auto k_unsized_basic_input = "[I32]";
inline std::string const k_unsized_basic_expected = test_sexp::array_type(test_sexp::type_name("I32"));

// Unsized array with String
constexpr auto k_unsized_string_should_succeed = true;
constexpr auto k_unsized_string_input = "[String]";
inline std::string const k_unsized_string_expected = test_sexp::array_type(test_sexp::type_name("String"));

// Unsized array with qualified type
constexpr auto k_unsized_qualified_should_succeed = true;
constexpr auto k_unsized_qualified_input = "[Std.Vec]";
inline std::string const k_unsized_qualified_expected =
    test_sexp::array_type(test_sexp::type_name_path({"Std", "Vec"}));

// Unsized array with generic type
constexpr auto k_unsized_generic_should_succeed = true;
constexpr auto k_unsized_generic_input = "[Vec<I32>]";
inline std::string const k_unsized_generic_expected =
    test_sexp::array_type(R"((path ((type_segment "Vec" ((path ((type_segment "I32"))))))))");

// Unsized nested array (array of unsized arrays)
constexpr auto k_unsized_nested_should_succeed = true;
constexpr auto k_unsized_nested_input = "[[I32]]";
inline std::string const k_unsized_nested_expected =
    test_sexp::array_type(test_sexp::array_type(test_sexp::type_name("I32")));

// Array with spaces
constexpr auto k_with_spaces_should_succeed = true;
constexpr auto k_with_spaces_input = "[ I32 ; 4 ]";
inline std::string const k_with_spaces_expected = test_sexp::array_type(test_sexp::type_name("I32"), "4");

// Array with function type element
constexpr auto k_func_type_element_should_succeed = true;
constexpr auto k_func_type_element_input = "[fn(I32): Bool; 2]";
inline std::string const k_func_type_element_expected =
    test_sexp::array_type(test_sexp::func_type({test_sexp::type_name("I32")}, test_sexp::type_name("Bool")), "2");

// Invalid: missing size
constexpr auto k_missing_size_should_succeed = false;
constexpr auto k_missing_size_input = "[I32; ]";

// Invalid: missing semicolon
constexpr auto k_missing_semicolon_should_succeed = false;
constexpr auto k_missing_semicolon_input = "[I32 4]";

// Invalid: missing closing bracket
constexpr auto k_missing_close_bracket_should_succeed = false;
constexpr auto k_missing_close_bracket_input = "[I32; 4";

// Invalid: missing element type
constexpr auto k_missing_element_type_should_succeed = false;
constexpr auto k_missing_element_type_input = "[; 4]";

// Invalid: non-integer size
constexpr auto k_non_integer_size_should_succeed = false;
constexpr auto k_non_integer_size_input = "[I32; foo]";

}  // namespace

TEST_CASE("Parse Array_Type") {
  std::vector<Array_Type_Params> const params_list = {
      {.name = "simple primitive",
       .input = k_simple_primitive_input,
       .expected = k_simple_primitive_expected,
       .should_succeed = k_simple_primitive_should_succeed},
      {.name = "string array",
       .input = k_string_array_input,
       .expected = k_string_array_expected,
       .should_succeed = k_string_array_should_succeed},
      {.name = "qualified type",
       .input = k_qualified_type_input,
       .expected = k_qualified_type_expected,
       .should_succeed = k_qualified_type_should_succeed},
      {.name = "generic type",
       .input = k_generic_type_input,
       .expected = k_generic_type_expected,
       .should_succeed = k_generic_type_should_succeed},
      {.name = "nested array",
       .input = k_nested_array_input,
       .expected = k_nested_array_expected,
       .should_succeed = k_nested_array_should_succeed},
      {.name = "large size",
       .input = k_large_size_input,
       .expected = k_large_size_expected,
       .should_succeed = k_large_size_should_succeed},
      {.name = "zero size",
       .input = k_zero_size_input,
       .expected = k_zero_size_expected,
       .should_succeed = k_zero_size_should_succeed},
      {.name = "size one",
       .input = k_size_one_input,
       .expected = k_size_one_expected,
       .should_succeed = k_size_one_should_succeed},
      {.name = "unsized basic",
       .input = k_unsized_basic_input,
       .expected = k_unsized_basic_expected,
       .should_succeed = k_unsized_basic_should_succeed},
      {.name = "unsized string",
       .input = k_unsized_string_input,
       .expected = k_unsized_string_expected,
       .should_succeed = k_unsized_string_should_succeed},
      {.name = "unsized qualified",
       .input = k_unsized_qualified_input,
       .expected = k_unsized_qualified_expected,
       .should_succeed = k_unsized_qualified_should_succeed},
      {.name = "unsized generic",
       .input = k_unsized_generic_input,
       .expected = k_unsized_generic_expected,
       .should_succeed = k_unsized_generic_should_succeed},
      {.name = "unsized nested",
       .input = k_unsized_nested_input,
       .expected = k_unsized_nested_expected,
       .should_succeed = k_unsized_nested_should_succeed},
      {.name = "with spaces",
       .input = k_with_spaces_input,
       .expected = k_with_spaces_expected,
       .should_succeed = k_with_spaces_should_succeed},
      {.name = "func type element",
       .input = k_func_type_element_input,
       .expected = k_func_type_element_expected,
       .should_succeed = k_func_type_element_should_succeed},
      {.name = "missing size",
       .input = k_missing_size_input,
       .expected = std::nullopt,
       .should_succeed = k_missing_size_should_succeed},
      {.name = "missing semicolon",
       .input = k_missing_semicolon_input,
       .expected = std::nullopt,
       .should_succeed = k_missing_semicolon_should_succeed},
      {.name = "missing close bracket",
       .input = k_missing_close_bracket_input,
       .expected = std::nullopt,
       .should_succeed = k_missing_close_bracket_should_succeed},
      {.name = "missing element type",
       .input = k_missing_element_type_input,
       .expected = std::nullopt,
       .should_succeed = k_missing_element_type_should_succeed},
      {.name = "non-integer size",
       .input = k_non_integer_size_input,
       .expected = std::nullopt,
       .should_succeed = k_non_integer_size_should_succeed},
  };

  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
