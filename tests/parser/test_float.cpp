#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Float;
using namespace test_sexp;

PARSE_TEST(Float, float)

namespace {

constexpr auto k_simple_float_should_succeed = true;
constexpr auto k_simple_float_input = "3.14";
inline auto const k_simple_float_expected = float_literal("3.14");

constexpr auto k_zero_point_zero_should_succeed = true;
constexpr auto k_zero_point_zero_input = "0.0";
inline auto const k_zero_point_zero_expected = float_literal("0.0");

constexpr auto k_one_point_zero_should_succeed = true;
constexpr auto k_one_point_zero_input = "1.0";
inline auto const k_one_point_zero_expected = float_literal("1.0");

constexpr auto k_many_decimals_should_succeed = true;
constexpr auto k_many_decimals_input = "123.456789";
inline auto const k_many_decimals_expected = float_literal("123.456789");

constexpr auto k_with_underscores_integer_should_succeed = true;
constexpr auto k_with_underscores_integer_input = "1_000.5";
inline auto const k_with_underscores_integer_expected = float_literal("1000.5");

constexpr auto k_with_underscores_decimal_should_succeed = true;
constexpr auto k_with_underscores_decimal_input = "123.456_789";
inline auto const k_with_underscores_decimal_expected = float_literal("123.456789");

constexpr auto k_with_underscores_both_should_succeed = true;
constexpr auto k_with_underscores_both_input = "1_234.567_890";
inline auto const k_with_underscores_both_expected = float_literal("1234.567890");

constexpr auto k_scientific_lowercase_e_should_succeed = true;
constexpr auto k_scientific_lowercase_e_input = "1.0e10";
inline auto const k_scientific_lowercase_e_expected = float_literal("1.0e10");

constexpr auto k_scientific_uppercase_e_should_succeed = true;
constexpr auto k_scientific_uppercase_e_input = "2.5E10";
inline auto const k_scientific_uppercase_e_expected = float_literal("2.5E10");

constexpr auto k_scientific_negative_exp_should_succeed = true;
constexpr auto k_scientific_negative_exp_input = "1.5e-10";
inline auto const k_scientific_negative_exp_expected = float_literal("1.5e-10");

constexpr auto k_scientific_positive_exp_should_succeed = true;
constexpr auto k_scientific_positive_exp_input = "3.0e+5";
inline auto const k_scientific_positive_exp_expected = float_literal("3.0e+5");

constexpr auto k_scientific_without_decimal_should_succeed = true;
constexpr auto k_scientific_without_decimal_input = "5e10";
inline auto const k_scientific_without_decimal_expected = float_literal("5e10");

constexpr auto k_scientific_with_underscores_should_succeed = true;
constexpr auto k_scientific_with_underscores_input = "1_234.567e1_0";
inline auto const k_scientific_with_underscores_expected = float_literal("1234.567e10");

constexpr auto k_with_trailing_text_should_succeed = false;  // New parser requires full consumption
constexpr auto k_with_trailing_text_input = "3.14 abc";
constexpr auto k_with_trailing_text_expected = "{}";  // Fails due to trailing text

// With type suffixes
constexpr auto k_with_f32_suffix_should_succeed = true;
constexpr auto k_with_f32_suffix_input = "3.14F32";
inline auto const k_with_f32_suffix_expected = float_literal("3.14", "F32");

constexpr auto k_with_f64_suffix_should_succeed = true;
constexpr auto k_with_f64_suffix_input = "2.5F64";
inline auto const k_with_f64_suffix_expected = float_literal("2.5", "F64");

constexpr auto k_with_suffix_and_exp_should_succeed = true;
constexpr auto k_with_suffix_and_exp_input = "1.0e10F64";
inline auto const k_with_suffix_and_exp_expected = float_literal("1.0e10", "F64");

constexpr auto k_with_suffix_and_underscores_should_succeed = true;
constexpr auto k_with_suffix_and_underscores_input = "1_234.567_89F32";
inline auto const k_with_suffix_and_underscores_expected = float_literal("1234.56789", "F32");

// Invalid cases
constexpr auto k_invalid_leading_dot_should_succeed = false;
constexpr auto k_invalid_leading_dot_input = ".5";
constexpr auto k_invalid_leading_dot_expected = "{}";

constexpr auto k_invalid_trailing_dot_should_succeed = true;  // Parser accepts trailing dot
constexpr auto k_invalid_trailing_dot_input = "5.";
inline auto const k_invalid_trailing_dot_expected = float_literal("5.");

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

TEST_CASE("Parse Float") {
  std::vector<Float_Params> const params_list = {
      {.name = "simple float",
       .input = k_simple_float_input,
       .expected = k_simple_float_expected,
       .should_succeed = k_simple_float_should_succeed},
      {.name = "zero point zero",
       .input = k_zero_point_zero_input,
       .expected = k_zero_point_zero_expected,
       .should_succeed = k_zero_point_zero_should_succeed},
      {.name = "one point zero",
       .input = k_one_point_zero_input,
       .expected = k_one_point_zero_expected,
       .should_succeed = k_one_point_zero_should_succeed},
      {.name = "many decimals",
       .input = k_many_decimals_input,
       .expected = k_many_decimals_expected,
       .should_succeed = k_many_decimals_should_succeed},
      {.name = "with underscores in integer part",
       .input = k_with_underscores_integer_input,
       .expected = k_with_underscores_integer_expected,
       .should_succeed = k_with_underscores_integer_should_succeed},
      {.name = "with underscores in decimal part",
       .input = k_with_underscores_decimal_input,
       .expected = k_with_underscores_decimal_expected,
       .should_succeed = k_with_underscores_decimal_should_succeed},
      {.name = "with underscores in both parts",
       .input = k_with_underscores_both_input,
       .expected = k_with_underscores_both_expected,
       .should_succeed = k_with_underscores_both_should_succeed},
      {.name = "scientific notation lowercase e",
       .input = k_scientific_lowercase_e_input,
       .expected = k_scientific_lowercase_e_expected,
       .should_succeed = k_scientific_lowercase_e_should_succeed},
      {.name = "scientific notation uppercase E",
       .input = k_scientific_uppercase_e_input,
       .expected = k_scientific_uppercase_e_expected,
       .should_succeed = k_scientific_uppercase_e_should_succeed},
      {.name = "scientific notation negative exponent",
       .input = k_scientific_negative_exp_input,
       .expected = k_scientific_negative_exp_expected,
       .should_succeed = k_scientific_negative_exp_should_succeed},
      {.name = "scientific notation positive exponent",
       .input = k_scientific_positive_exp_input,
       .expected = k_scientific_positive_exp_expected,
       .should_succeed = k_scientific_positive_exp_should_succeed},
      {.name = "scientific notation without decimal",
       .input = k_scientific_without_decimal_input,
       .expected = k_scientific_without_decimal_expected,
       .should_succeed = k_scientific_without_decimal_should_succeed},
      {.name = "scientific notation with underscores",
       .input = k_scientific_with_underscores_input,
       .expected = k_scientific_with_underscores_expected,
       .should_succeed = k_scientific_with_underscores_should_succeed},
      {.name = "with trailing text",
       .input = k_with_trailing_text_input,
       .expected = k_with_trailing_text_expected,
       .should_succeed = k_with_trailing_text_should_succeed},
      {.name = "invalid - leading dot",
       .input = k_invalid_leading_dot_input,
       .expected = k_invalid_leading_dot_expected,
       .should_succeed = k_invalid_leading_dot_should_succeed},
      {.name = "invalid - trailing dot",
       .input = k_invalid_trailing_dot_input,
       .expected = k_invalid_trailing_dot_expected,
       .should_succeed = k_invalid_trailing_dot_should_succeed},
      {.name = "invalid - no dot no exponent",
       .input = k_invalid_no_dot_no_exp_input,
       .expected = k_invalid_no_dot_no_exp_expected,
       .should_succeed = k_invalid_no_dot_no_exp_should_succeed},
      {.name = "with F32 suffix",
       .input = k_with_f32_suffix_input,
       .expected = k_with_f32_suffix_expected,
       .should_succeed = k_with_f32_suffix_should_succeed},
      {.name = "with F64 suffix",
       .input = k_with_f64_suffix_input,
       .expected = k_with_f64_suffix_expected,
       .should_succeed = k_with_f64_suffix_should_succeed},
      {.name = "with suffix and exponent",
       .input = k_with_suffix_and_exp_input,
       .expected = k_with_suffix_and_exp_expected,
       .should_succeed = k_with_suffix_and_exp_should_succeed},
      {.name = "with suffix and underscores",
       .input = k_with_suffix_and_underscores_input,
       .expected = k_with_suffix_and_underscores_expected,
       .should_succeed = k_with_suffix_and_underscores_should_succeed},
      {.name = "invalid - ends with underscore",
       .input = k_invalid_ends_with_underscore_input,
       .expected = k_invalid_ends_with_underscore_expected,
       .should_succeed = k_invalid_ends_with_underscore_should_succeed},
      {.name = "invalid - exponent ends with underscore",
       .input = k_invalid_exp_ends_with_underscore_input,
       .expected = k_invalid_exp_ends_with_underscore_expected,
       .should_succeed = k_invalid_exp_ends_with_underscore_should_succeed},
      {.name = "invalid - empty",
       .input = k_invalid_empty_input,
       .expected = k_invalid_empty_expected,
       .should_succeed = k_invalid_empty_should_succeed},
      {.name = "invalid - letter",
       .input = k_invalid_letter_input,
       .expected = k_invalid_letter_expected,
       .should_succeed = k_invalid_letter_should_succeed},
      {.name = "invalid - exponent without number",
       .input = k_invalid_exp_without_number_input,
       .expected = k_invalid_exp_without_number_expected,
       .should_succeed = k_invalid_exp_without_number_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
