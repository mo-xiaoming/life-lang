#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Float;

PARSE_TEST(Float, float)

namespace {

constexpr auto k_simple_float_should_succeed = true;
constexpr auto k_simple_float_input = "3.14";
constexpr auto k_simple_float_expected = R"({
  "Float": {
    "value": "3.14"
  }
})";

constexpr auto k_zero_point_zero_should_succeed = true;
constexpr auto k_zero_point_zero_input = "0.0";
constexpr auto k_zero_point_zero_expected = R"({
  "Float": {
    "value": "0.0"
  }
})";

constexpr auto k_one_point_zero_should_succeed = true;
constexpr auto k_one_point_zero_input = "1.0";
constexpr auto k_one_point_zero_expected = R"({
  "Float": {
    "value": "1.0"
  }
})";

constexpr auto k_many_decimals_should_succeed = true;
constexpr auto k_many_decimals_input = "123.456789";
constexpr auto k_many_decimals_expected = R"({
  "Float": {
    "value": "123.456789"
  }
})";

constexpr auto k_with_underscores_integer_should_succeed = true;
constexpr auto k_with_underscores_integer_input = "1_000.5";
constexpr auto k_with_underscores_integer_expected = R"({
  "Float": {
    "value": "1000.5"
  }
})";

constexpr auto k_with_underscores_decimal_should_succeed = true;
constexpr auto k_with_underscores_decimal_input = "123.456_789";
constexpr auto k_with_underscores_decimal_expected = R"({
  "Float": {
    "value": "123.456789"
  }
})";

constexpr auto k_with_underscores_both_should_succeed = true;
constexpr auto k_with_underscores_both_input = "1_234.567_890";
constexpr auto k_with_underscores_both_expected = R"({
  "Float": {
    "value": "1234.567890"
  }
})";

constexpr auto k_scientific_lowercase_e_should_succeed = true;
constexpr auto k_scientific_lowercase_e_input = "1.0e10";
constexpr auto k_scientific_lowercase_e_expected = R"({
  "Float": {
    "value": "1.0e10"
  }
})";

constexpr auto k_scientific_uppercase_e_should_succeed = true;
constexpr auto k_scientific_uppercase_e_input = "2.5E10";
constexpr auto k_scientific_uppercase_e_expected = R"({
  "Float": {
    "value": "2.5E10"
  }
})";

constexpr auto k_scientific_negative_exp_should_succeed = true;
constexpr auto k_scientific_negative_exp_input = "1.5e-10";
constexpr auto k_scientific_negative_exp_expected = R"({
  "Float": {
    "value": "1.5e-10"
  }
})";

constexpr auto k_scientific_positive_exp_should_succeed = true;
constexpr auto k_scientific_positive_exp_input = "3.0e+5";
constexpr auto k_scientific_positive_exp_expected = R"({
  "Float": {
    "value": "3.0e+5"
  }
})";

constexpr auto k_scientific_without_decimal_should_succeed = true;
constexpr auto k_scientific_without_decimal_input = "5e10";
constexpr auto k_scientific_without_decimal_expected = R"({
  "Float": {
    "value": "5e10"
  }
})";

constexpr auto k_scientific_with_underscores_should_succeed = true;
constexpr auto k_scientific_with_underscores_input = "1_234.567e1_0";
constexpr auto k_scientific_with_underscores_expected = R"({
  "Float": {
    "value": "1234.567e10"
  }
})";

constexpr auto k_with_trailing_text_should_succeed = true;
constexpr auto k_with_trailing_text_input = "3.14 abc";
constexpr auto k_with_trailing_text_expected = R"({
  "Float": {
    "value": "3.14"
  }
})";

// Invalid cases
constexpr auto k_invalid_leading_dot_should_succeed = false;
constexpr auto k_invalid_leading_dot_input = ".5";
constexpr auto k_invalid_leading_dot_expected = "{}";

constexpr auto k_invalid_trailing_dot_should_succeed = false;
constexpr auto k_invalid_trailing_dot_input = "5.";
constexpr auto k_invalid_trailing_dot_expected = "{}";

constexpr auto k_invalid_no_dot_no_exp_should_succeed = false;
constexpr auto k_invalid_no_dot_no_exp_input = "123";
constexpr auto k_invalid_no_dot_no_exp_expected = "{}";

constexpr auto k_invalid_ends_with_underscore_should_succeed = false;
constexpr auto k_invalid_ends_with_underscore_input = "12.34_";
constexpr auto k_invalid_ends_with_underscore_expected = "{}";

constexpr auto k_invalid_exp_ends_with_underscore_should_succeed = false;
constexpr auto k_invalid_exp_ends_with_underscore_input = "1.2e3_";
constexpr auto k_invalid_exp_ends_with_underscore_expected = "{}";

constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";
constexpr auto k_invalid_empty_expected = "{}";

constexpr auto k_invalid_letter_should_succeed = false;
constexpr auto k_invalid_letter_input = "abc";
constexpr auto k_invalid_letter_expected = "{}";

constexpr auto k_invalid_exp_without_number_should_succeed = false;
constexpr auto k_invalid_exp_without_number_input = "e10";
constexpr auto k_invalid_exp_without_number_expected = "{}";

}  // namespace

TEST_CASE("Parse Float", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Float_Params>({
          {"simple float", k_simple_float_input, k_simple_float_expected, k_simple_float_should_succeed},
          {"zero point zero", k_zero_point_zero_input, k_zero_point_zero_expected, k_zero_point_zero_should_succeed},
          {"one point zero", k_one_point_zero_input, k_one_point_zero_expected, k_one_point_zero_should_succeed},
          {"many decimals", k_many_decimals_input, k_many_decimals_expected, k_many_decimals_should_succeed},
          {"with underscores in integer part",
           k_with_underscores_integer_input,
           k_with_underscores_integer_expected,
           k_with_underscores_integer_should_succeed},
          {"with underscores in decimal part",
           k_with_underscores_decimal_input,
           k_with_underscores_decimal_expected,
           k_with_underscores_decimal_should_succeed},
          {"with underscores in both parts",
           k_with_underscores_both_input,
           k_with_underscores_both_expected,
           k_with_underscores_both_should_succeed},
          {"scientific notation lowercase e",
           k_scientific_lowercase_e_input,
           k_scientific_lowercase_e_expected,
           k_scientific_lowercase_e_should_succeed},
          {"scientific notation uppercase E",
           k_scientific_uppercase_e_input,
           k_scientific_uppercase_e_expected,
           k_scientific_uppercase_e_should_succeed},
          {"scientific notation negative exponent",
           k_scientific_negative_exp_input,
           k_scientific_negative_exp_expected,
           k_scientific_negative_exp_should_succeed},
          {"scientific notation positive exponent",
           k_scientific_positive_exp_input,
           k_scientific_positive_exp_expected,
           k_scientific_positive_exp_should_succeed},
          {"scientific notation without decimal",
           k_scientific_without_decimal_input,
           k_scientific_without_decimal_expected,
           k_scientific_without_decimal_should_succeed},
          {"scientific notation with underscores",
           k_scientific_with_underscores_input,
           k_scientific_with_underscores_expected,
           k_scientific_with_underscores_should_succeed},
          {"with trailing text",
           k_with_trailing_text_input,
           k_with_trailing_text_expected,
           k_with_trailing_text_should_succeed},
          {"invalid - leading dot",
           k_invalid_leading_dot_input,
           k_invalid_leading_dot_expected,
           k_invalid_leading_dot_should_succeed},
          {"invalid - trailing dot",
           k_invalid_trailing_dot_input,
           k_invalid_trailing_dot_expected,
           k_invalid_trailing_dot_should_succeed},
          {"invalid - no dot no exponent",
           k_invalid_no_dot_no_exp_input,
           k_invalid_no_dot_no_exp_expected,
           k_invalid_no_dot_no_exp_should_succeed},
          {"invalid - ends with underscore",
           k_invalid_ends_with_underscore_input,
           k_invalid_ends_with_underscore_expected,
           k_invalid_ends_with_underscore_should_succeed},
          {"invalid - exponent ends with underscore",
           k_invalid_exp_ends_with_underscore_input,
           k_invalid_exp_ends_with_underscore_expected,
           k_invalid_exp_ends_with_underscore_should_succeed},
          {"invalid - empty", k_invalid_empty_input, k_invalid_empty_expected, k_invalid_empty_should_succeed},
          {"invalid - letter", k_invalid_letter_input, k_invalid_letter_expected, k_invalid_letter_should_succeed},
          {"invalid - exponent without number",
           k_invalid_exp_without_number_input,
           k_invalid_exp_without_number_expected,
           k_invalid_exp_without_number_should_succeed},
      })
  );

  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
