#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Integer;
using namespace test_sexp;

PARSE_TEST(Integer, integer)

namespace {

constexpr auto k_zero_should_succeed = true;
constexpr auto k_zero_input = "0";
inline auto const k_zero_expected = test_sexp::integer("0");

constexpr auto k_simple_number_should_succeed = true;
constexpr auto k_simple_number_input = "123";
inline auto const k_simple_number_expected = test_sexp::integer("123");

constexpr auto k_large_number_should_succeed = true;
constexpr auto k_large_number_input = "987654321";
inline auto const k_large_number_expected = test_sexp::integer("987654321");

constexpr auto k_with_underscores_should_succeed = true;
constexpr auto k_with_underscores_input = "12_34_5";
inline auto const k_with_underscores_expected = test_sexp::integer("12345");

constexpr auto k_multiple_underscores_should_succeed = true;
constexpr auto k_multiple_underscores_input = "1_2_3_4";
inline auto const k_multiple_underscores_expected = test_sexp::integer("1234");

constexpr auto k_with_trailing_text_should_succeed = false;  // New parser requires full consumption
constexpr auto k_with_trailing_text_input = "42 abc";

// With type suffixes
constexpr auto k_with_i32_suffix_should_succeed = true;
constexpr auto k_with_i32_suffix_input = "42I32";
inline auto const k_with_i32_suffix_expected = test_sexp::integer("42", "I32");

constexpr auto k_with_u8_suffix_should_succeed = true;
constexpr auto k_with_u8_suffix_input = "255U8";
inline auto const k_with_u8_suffix_expected = test_sexp::integer("255", "U8");

constexpr auto k_with_i64_suffix_should_succeed = true;
constexpr auto k_with_i64_suffix_input = "1000I64";
inline auto const k_with_i64_suffix_expected = test_sexp::integer("1000", "I64");

constexpr auto k_with_suffix_and_underscores_should_succeed = true;
constexpr auto k_with_suffix_and_underscores_input = "1_000_000I32";
inline auto const k_with_suffix_and_underscores_expected = test_sexp::integer("1000000", "I32");

// Invalid cases
constexpr auto k_invalid_starts_with_zero_should_succeed = false;
constexpr auto k_invalid_starts_with_zero_input = "0123";

constexpr auto k_invalid_starts_with_underscore_should_succeed = false;
constexpr auto k_invalid_starts_with_underscore_input = "_12";

constexpr auto k_invalid_ends_with_underscore_should_succeed = false;
constexpr auto k_invalid_ends_with_underscore_input = "12_";

constexpr auto k_invalid_zero_with_underscore_should_succeed = false;
constexpr auto k_invalid_zero_with_underscore_input = "0_";

constexpr auto k_invalid_underscore_before_zero_should_succeed = false;
constexpr auto k_invalid_underscore_before_zero_input = "_0";

constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";

constexpr auto k_invalid_letter_should_succeed = false;
constexpr auto k_invalid_letter_input = "abc";

// Hexadecimal literals
constexpr auto k_hex_lowercase_should_succeed = true;
constexpr auto k_hex_lowercase_input = "0xff";
inline auto const k_hex_lowercase_expected = test_sexp::integer("0xff");

constexpr auto k_hex_uppercase_should_succeed = true;
constexpr auto k_hex_uppercase_input = "0xFF";
inline auto const k_hex_uppercase_expected = test_sexp::integer("0xFF");

constexpr auto k_hex_mixed_case_should_succeed = true;
constexpr auto k_hex_mixed_case_input = "0xDeadBeef";
inline auto const k_hex_mixed_case_expected = test_sexp::integer("0xDeadBeef");

constexpr auto k_hex_with_underscores_should_succeed = true;
constexpr auto k_hex_with_underscores_input = "0xDEAD_BEEF";
inline auto const k_hex_with_underscores_expected = test_sexp::integer("0xDEADBEEF");

constexpr auto k_hex_single_digit_should_succeed = true;
constexpr auto k_hex_single_digit_input = "0xF";
inline auto const k_hex_single_digit_expected = test_sexp::integer("0xF");

constexpr auto k_hex_all_digits_should_succeed = true;
constexpr auto k_hex_all_digits_input = "0x1234567890";
inline auto const k_hex_all_digits_expected = test_sexp::integer("0x1234567890");

constexpr auto k_hex_all_letters_should_succeed = true;
constexpr auto k_hex_all_letters_input = "0xABCDEF";
inline auto const k_hex_all_letters_expected = test_sexp::integer("0xABCDEF");

constexpr auto k_hex_with_suffix_should_succeed = true;
constexpr auto k_hex_with_suffix_input = "0xFFU32";
inline auto const k_hex_with_suffix_expected = test_sexp::integer("0xFF", "U32");

constexpr auto k_hex_large_value_should_succeed = true;
constexpr auto k_hex_large_value_input = "0x1234_5678_90AB_CDEF";
inline auto const k_hex_large_value_expected = test_sexp::integer("0x1234567890ABCDEF");

// Invalid hexadecimal cases
constexpr auto k_hex_no_digits_should_succeed = false;
constexpr auto k_hex_no_digits_input = "0x";

constexpr auto k_hex_invalid_char_should_succeed = false;
constexpr auto k_hex_invalid_char_input = "0xGG";

constexpr auto k_hex_trailing_underscore_should_succeed = false;
constexpr auto k_hex_trailing_underscore_input = "0xFF_";

constexpr auto k_hex_leading_underscore_should_succeed = false;
constexpr auto k_hex_leading_underscore_input = "0x_FF";

constexpr auto k_hex_uppercase_x_should_succeed = true;
constexpr auto k_hex_uppercase_x_input = "0XFF";
inline auto const k_hex_uppercase_x_expected = integer("0xFF");

// Binary literals
constexpr auto k_binary_simple_should_succeed = true;
constexpr auto k_binary_simple_input = "0b1010";
inline auto const k_binary_simple_expected = integer("0b1010");

constexpr auto k_binary_all_ones_should_succeed = true;
constexpr auto k_binary_all_ones_input = "0b11111111";
inline auto const k_binary_all_ones_expected = integer("0b11111111");

constexpr auto k_binary_all_zeros_should_succeed = true;
constexpr auto k_binary_all_zeros_input = "0b00000000";
inline auto const k_binary_all_zeros_expected = integer("0b00000000");

constexpr auto k_binary_with_underscores_should_succeed = true;
constexpr auto k_binary_with_underscores_input = "0b1111_0000_1010_0101";
inline auto const k_binary_with_underscores_expected = integer("0b1111000010100101");

constexpr auto k_binary_single_digit_should_succeed = true;
constexpr auto k_binary_single_digit_input = "0b1";
inline auto const k_binary_single_digit_expected = integer("0b1");

constexpr auto k_binary_with_suffix_should_succeed = true;
constexpr auto k_binary_with_suffix_input = "0b11111111U8";
inline auto const k_binary_with_suffix_expected = integer("0b11111111", "U8");

constexpr auto k_binary_uppercase_b_should_succeed = true;
constexpr auto k_binary_uppercase_b_input = "0B1010";
inline auto const k_binary_uppercase_b_expected = integer("0b1010");

constexpr auto k_binary_byte_should_succeed = true;
constexpr auto k_binary_byte_input = "0b1010_1100";
inline auto const k_binary_byte_expected = integer("0b10101100");

// Invalid binary cases
constexpr auto k_binary_no_digits_should_succeed = false;
constexpr auto k_binary_no_digits_input = "0b";

constexpr auto k_binary_invalid_digit_should_succeed = false;
constexpr auto k_binary_invalid_digit_input = "0b102";

constexpr auto k_binary_trailing_underscore_should_succeed = false;
constexpr auto k_binary_trailing_underscore_input = "0b1010_";

constexpr auto k_binary_leading_underscore_should_succeed = false;
constexpr auto k_binary_leading_underscore_input = "0b_1010";

// Octal literals
constexpr auto k_octal_simple_should_succeed = true;
constexpr auto k_octal_simple_input = "0o755";
inline auto const k_octal_simple_expected = integer("0o755");

constexpr auto k_octal_lowercase_o_should_succeed = true;
constexpr auto k_octal_lowercase_o_input = "0o644";
inline auto const k_octal_lowercase_o_expected = integer("0o644");

constexpr auto k_octal_uppercase_o_should_succeed = true;
constexpr auto k_octal_uppercase_o_input = "0O777";
inline auto const k_octal_uppercase_o_expected = integer("0o777");

constexpr auto k_octal_with_underscores_should_succeed = true;
constexpr auto k_octal_with_underscores_input = "0o7_5_5";
inline auto const k_octal_with_underscores_expected = integer("0o755");

constexpr auto k_octal_zero_should_succeed = true;
constexpr auto k_octal_zero_input = "0o0";
inline auto const k_octal_zero_expected = integer("0o0");

constexpr auto k_octal_max_digit_should_succeed = true;
constexpr auto k_octal_max_digit_input = "0o777";
inline auto const k_octal_max_digit_expected = integer("0o777");

constexpr auto k_octal_with_suffix_should_succeed = true;
constexpr auto k_octal_with_suffix_input = "0o644U16";
inline auto const k_octal_with_suffix_expected = integer("0o644", "U16");

// Invalid octal cases
constexpr auto k_octal_no_digits_should_succeed = false;
constexpr auto k_octal_no_digits_input = "0o";

constexpr auto k_octal_invalid_digit_should_succeed = false;
constexpr auto k_octal_invalid_digit_input = "0o778";

constexpr auto k_octal_trailing_underscore_should_succeed = false;
constexpr auto k_octal_trailing_underscore_input = "0o755_";

constexpr auto k_octal_leading_underscore_should_succeed = false;
constexpr auto k_octal_leading_underscore_input = "0o_755";

}  // namespace

TEST_CASE("Parse Integer") {
  std::vector<Integer_Params> const params_list = {
      {.name = "zero", .input = k_zero_input, .expected = k_zero_expected, .should_succeed = k_zero_should_succeed},
      {.name = "simple number",
       .input = k_simple_number_input,
       .expected = k_simple_number_expected,
       .should_succeed = k_simple_number_should_succeed},
      {.name = "large number",
       .input = k_large_number_input,
       .expected = k_large_number_expected,
       .should_succeed = k_large_number_should_succeed},
      {.name = "with underscores",
       .input = k_with_underscores_input,
       .expected = k_with_underscores_expected,
       .should_succeed = k_with_underscores_should_succeed},
      {.name = "multiple underscores",
       .input = k_multiple_underscores_input,
       .expected = k_multiple_underscores_expected,
       .should_succeed = k_multiple_underscores_should_succeed},
      {.name = "with trailing text",
       .input = k_with_trailing_text_input,
       .expected = std::nullopt,
       .should_succeed = k_with_trailing_text_should_succeed},
      {.name = "with I32 suffix",
       .input = k_with_i32_suffix_input,
       .expected = k_with_i32_suffix_expected,
       .should_succeed = k_with_i32_suffix_should_succeed},
      {.name = "with U8 suffix",
       .input = k_with_u8_suffix_input,
       .expected = k_with_u8_suffix_expected,
       .should_succeed = k_with_u8_suffix_should_succeed},
      {.name = "with I64 suffix",
       .input = k_with_i64_suffix_input,
       .expected = k_with_i64_suffix_expected,
       .should_succeed = k_with_i64_suffix_should_succeed},
      {.name = "with suffix and underscores",
       .input = k_with_suffix_and_underscores_input,
       .expected = k_with_suffix_and_underscores_expected,
       .should_succeed = k_with_suffix_and_underscores_should_succeed},
      {.name = "invalid - starts with zero",
       .input = k_invalid_starts_with_zero_input,
       .expected = std::nullopt,
       .should_succeed = k_invalid_starts_with_zero_should_succeed},
      {.name = "invalid - starts with underscore",
       .input = k_invalid_starts_with_underscore_input,
       .expected = std::nullopt,
       .should_succeed = k_invalid_starts_with_underscore_should_succeed},
      {.name = "invalid - ends with underscore",
       .input = k_invalid_ends_with_underscore_input,
       .expected = std::nullopt,
       .should_succeed = k_invalid_ends_with_underscore_should_succeed},
      {.name = "invalid - zero with underscore",
       .input = k_invalid_zero_with_underscore_input,
       .expected = std::nullopt,
       .should_succeed = k_invalid_zero_with_underscore_should_succeed},
      {.name = "invalid - underscore before zero",
       .input = k_invalid_underscore_before_zero_input,
       .expected = std::nullopt,
       .should_succeed = k_invalid_underscore_before_zero_should_succeed},
      {.name = "invalid - empty",
       .input = k_invalid_empty_input,
       .expected = std::nullopt,
       .should_succeed = k_invalid_empty_should_succeed},
      {.name = "invalid letter",
       .input = k_invalid_letter_input,
       .expected = std::nullopt,
       .should_succeed = k_invalid_letter_should_succeed},
      // Hexadecimal literals
      {.name = "hex lowercase",
       .input = k_hex_lowercase_input,
       .expected = k_hex_lowercase_expected,
       .should_succeed = k_hex_lowercase_should_succeed},
      {.name = "hex uppercase",
       .input = k_hex_uppercase_input,
       .expected = k_hex_uppercase_expected,
       .should_succeed = k_hex_uppercase_should_succeed},
      {.name = "hex mixed case",
       .input = k_hex_mixed_case_input,
       .expected = k_hex_mixed_case_expected,
       .should_succeed = k_hex_mixed_case_should_succeed},
      {.name = "hex with underscores",
       .input = k_hex_with_underscores_input,
       .expected = k_hex_with_underscores_expected,
       .should_succeed = k_hex_with_underscores_should_succeed},
      {.name = "hex single digit",
       .input = k_hex_single_digit_input,
       .expected = k_hex_single_digit_expected,
       .should_succeed = k_hex_single_digit_should_succeed},
      {.name = "hex all digits",
       .input = k_hex_all_digits_input,
       .expected = k_hex_all_digits_expected,
       .should_succeed = k_hex_all_digits_should_succeed},
      {.name = "hex all letters",
       .input = k_hex_all_letters_input,
       .expected = k_hex_all_letters_expected,
       .should_succeed = k_hex_all_letters_should_succeed},
      {.name = "hex with suffix",
       .input = k_hex_with_suffix_input,
       .expected = k_hex_with_suffix_expected,
       .should_succeed = k_hex_with_suffix_should_succeed},
      {.name = "hex large value",
       .input = k_hex_large_value_input,
       .expected = k_hex_large_value_expected,
       .should_succeed = k_hex_large_value_should_succeed},
      // Invalid hex cases
      {.name = "hex no digits",
       .input = k_hex_no_digits_input,
       .expected = std::nullopt,
       .should_succeed = k_hex_no_digits_should_succeed},
      {.name = "hex invalid char",
       .input = k_hex_invalid_char_input,
       .expected = std::nullopt,
       .should_succeed = k_hex_invalid_char_should_succeed},
      {.name = "hex trailing underscore",
       .input = k_hex_trailing_underscore_input,
       .expected = std::nullopt,
       .should_succeed = k_hex_trailing_underscore_should_succeed},
      {.name = "hex leading underscore",
       .input = k_hex_leading_underscore_input,
       .expected = std::nullopt,
       .should_succeed = k_hex_leading_underscore_should_succeed},
      {.name = "hex uppercase X",
       .input = k_hex_uppercase_x_input,
       .expected = k_hex_uppercase_x_expected,
       .should_succeed = k_hex_uppercase_x_should_succeed},
      // Binary literals
      {.name = "binary simple",
       .input = k_binary_simple_input,
       .expected = k_binary_simple_expected,
       .should_succeed = k_binary_simple_should_succeed},
      {.name = "binary all ones",
       .input = k_binary_all_ones_input,
       .expected = k_binary_all_ones_expected,
       .should_succeed = k_binary_all_ones_should_succeed},
      {.name = "binary all zeros",
       .input = k_binary_all_zeros_input,
       .expected = k_binary_all_zeros_expected,
       .should_succeed = k_binary_all_zeros_should_succeed},
      {.name = "binary with underscores",
       .input = k_binary_with_underscores_input,
       .expected = k_binary_with_underscores_expected,
       .should_succeed = k_binary_with_underscores_should_succeed},
      {.name = "binary single digit",
       .input = k_binary_single_digit_input,
       .expected = k_binary_single_digit_expected,
       .should_succeed = k_binary_single_digit_should_succeed},
      {.name = "binary with suffix",
       .input = k_binary_with_suffix_input,
       .expected = k_binary_with_suffix_expected,
       .should_succeed = k_binary_with_suffix_should_succeed},
      {.name = "binary uppercase B",
       .input = k_binary_uppercase_b_input,
       .expected = k_binary_uppercase_b_expected,
       .should_succeed = k_binary_uppercase_b_should_succeed},
      {.name = "binary byte",
       .input = k_binary_byte_input,
       .expected = k_binary_byte_expected,
       .should_succeed = k_binary_byte_should_succeed},
      // Invalid binary cases
      {.name = "binary no digits",
       .input = k_binary_no_digits_input,
       .expected = std::nullopt,
       .should_succeed = k_binary_no_digits_should_succeed},
      {.name = "binary invalid digit",
       .input = k_binary_invalid_digit_input,
       .expected = std::nullopt,
       .should_succeed = k_binary_invalid_digit_should_succeed},
      {.name = "binary trailing underscore",
       .input = k_binary_trailing_underscore_input,
       .expected = std::nullopt,
       .should_succeed = k_binary_trailing_underscore_should_succeed},
      {.name = "binary leading underscore",
       .input = k_binary_leading_underscore_input,
       .expected = std::nullopt,
       .should_succeed = k_binary_leading_underscore_should_succeed},
      // Octal literals
      {.name = "octal simple",
       .input = k_octal_simple_input,
       .expected = k_octal_simple_expected,
       .should_succeed = k_octal_simple_should_succeed},
      {.name = "octal lowercase o",
       .input = k_octal_lowercase_o_input,
       .expected = k_octal_lowercase_o_expected,
       .should_succeed = k_octal_lowercase_o_should_succeed},
      {.name = "octal uppercase O",
       .input = k_octal_uppercase_o_input,
       .expected = k_octal_uppercase_o_expected,
       .should_succeed = k_octal_uppercase_o_should_succeed},
      {.name = "octal with underscores",
       .input = k_octal_with_underscores_input,
       .expected = k_octal_with_underscores_expected,
       .should_succeed = k_octal_with_underscores_should_succeed},
      {.name = "octal zero",
       .input = k_octal_zero_input,
       .expected = k_octal_zero_expected,
       .should_succeed = k_octal_zero_should_succeed},
      {.name = "octal max digit",
       .input = k_octal_max_digit_input,
       .expected = k_octal_max_digit_expected,
       .should_succeed = k_octal_max_digit_should_succeed},
      {.name = "octal with suffix",
       .input = k_octal_with_suffix_input,
       .expected = k_octal_with_suffix_expected,
       .should_succeed = k_octal_with_suffix_should_succeed},
      // Invalid octal cases
      {.name = "octal no digits",
       .input = k_octal_no_digits_input,
       .expected = std::nullopt,
       .should_succeed = k_octal_no_digits_should_succeed},
      {.name = "octal invalid digit",
       .input = k_octal_invalid_digit_input,
       .expected = std::nullopt,
       .should_succeed = k_octal_invalid_digit_should_succeed},
      {.name = "octal trailing underscore",
       .input = k_octal_trailing_underscore_input,
       .expected = std::nullopt,
       .should_succeed = k_octal_trailing_underscore_should_succeed},
      {.name = "octal leading underscore",
       .input = k_octal_leading_underscore_input,
       .expected = std::nullopt,
       .should_succeed = k_octal_leading_underscore_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}