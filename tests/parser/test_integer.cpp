#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Integer;

PARSE_TEST(Integer, integer)

namespace {

constexpr auto k_zero_should_succeed = true;
constexpr auto k_zero_input = "0";
constexpr auto k_zero_expected = R"({
  "Integer": {
    "value": "0"
  }
})";

constexpr auto k_simple_number_should_succeed = true;
constexpr auto k_simple_number_input = "123";
constexpr auto k_simple_number_expected = R"({
  "Integer": {
    "value": "123"
  }
})";

constexpr auto k_large_number_should_succeed = true;
constexpr auto k_large_number_input = "987654321";
constexpr auto k_large_number_expected = R"({
  "Integer": {
    "value": "987654321"
  }
})";

constexpr auto k_with_underscores_should_succeed = true;
constexpr auto k_with_underscores_input = "12_34_5";
constexpr auto k_with_underscores_expected = R"({
  "Integer": {
    "value": "12345"
  }
})";

constexpr auto k_multiple_underscores_should_succeed = true;
constexpr auto k_multiple_underscores_input = "1_2_3_4";
constexpr auto k_multiple_underscores_expected = R"({
  "Integer": {
    "value": "1234"
  }
})";

constexpr auto k_with_trailing_text_should_succeed = true;
constexpr auto k_with_trailing_text_input = "42 abc";
constexpr auto k_with_trailing_text_expected = R"({
  "Integer": {
    "value": "42"
  }
})";

// With type suffixes
constexpr auto k_with_i32_suffix_should_succeed = true;
constexpr auto k_with_i32_suffix_input = "42I32";
constexpr auto k_with_i32_suffix_expected = R"({
  "Integer": {
    "value": "42",
    "suffix": "I32"
  }
})";

constexpr auto k_with_u8_suffix_should_succeed = true;
constexpr auto k_with_u8_suffix_input = "255U8";
constexpr auto k_with_u8_suffix_expected = R"({
  "Integer": {
    "value": "255",
    "suffix": "U8"
  }
})";

constexpr auto k_with_i64_suffix_should_succeed = true;
constexpr auto k_with_i64_suffix_input = "1000I64";
constexpr auto k_with_i64_suffix_expected = R"({
  "Integer": {
    "value": "1000",
    "suffix": "I64"
  }
})";

constexpr auto k_with_suffix_and_underscores_should_succeed = true;
constexpr auto k_with_suffix_and_underscores_input = "1_000_000I32";
constexpr auto k_with_suffix_and_underscores_expected = R"({
  "Integer": {
    "value": "1000000",
    "suffix": "I32"
  }
})";

// Invalid cases
constexpr auto k_invalid_starts_with_zero_should_succeed = false;
constexpr auto k_invalid_starts_with_zero_input = "0123";
constexpr auto k_invalid_starts_with_zero_expected = "{}";

constexpr auto k_invalid_starts_with_underscore_should_succeed = false;
constexpr auto k_invalid_starts_with_underscore_input = "_12";
constexpr auto k_invalid_starts_with_underscore_expected = "{}";

constexpr auto k_invalid_ends_with_underscore_should_succeed = false;
constexpr auto k_invalid_ends_with_underscore_input = "12_";
constexpr auto k_invalid_ends_with_underscore_expected = "{}";

constexpr auto k_invalid_zero_with_underscore_should_succeed = false;
constexpr auto k_invalid_zero_with_underscore_input = "0_";
constexpr auto k_invalid_zero_with_underscore_expected = "{}";

constexpr auto k_invalid_underscore_before_zero_should_succeed = false;
constexpr auto k_invalid_underscore_before_zero_input = "_0";
constexpr auto k_invalid_underscore_before_zero_expected = "{}";

constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";
constexpr auto k_invalid_empty_expected = "{}";

constexpr auto k_invalid_letter_should_succeed = false;
constexpr auto k_invalid_letter_input = "abc";
constexpr auto k_invalid_letter_expected = "{}";

}  // namespace

TEST_CASE("Parse Integer", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Integer_Params>({
          {"zero", k_zero_input, k_zero_expected, k_zero_should_succeed},
          {"simple number", k_simple_number_input, k_simple_number_expected, k_simple_number_should_succeed},
          {"large number", k_large_number_input, k_large_number_expected, k_large_number_should_succeed},
          {"with underscores",
           k_with_underscores_input,
           k_with_underscores_expected,
           k_with_underscores_should_succeed},
          {"multiple underscores",
           k_multiple_underscores_input,
           k_multiple_underscores_expected,
           k_multiple_underscores_should_succeed},
          {"with trailing text",
           k_with_trailing_text_input,
           k_with_trailing_text_expected,
           k_with_trailing_text_should_succeed},
          {"with I32 suffix", k_with_i32_suffix_input, k_with_i32_suffix_expected, k_with_i32_suffix_should_succeed},
          {"with U8 suffix", k_with_u8_suffix_input, k_with_u8_suffix_expected, k_with_u8_suffix_should_succeed},
          {"with I64 suffix", k_with_i64_suffix_input, k_with_i64_suffix_expected, k_with_i64_suffix_should_succeed},
          {"with suffix and underscores",
           k_with_suffix_and_underscores_input,
           k_with_suffix_and_underscores_expected,
           k_with_suffix_and_underscores_should_succeed},
          {"invalid - starts with zero",
           k_invalid_starts_with_zero_input,
           k_invalid_starts_with_zero_expected,
           k_invalid_starts_with_zero_should_succeed},
          {"invalid - starts with underscore",
           k_invalid_starts_with_underscore_input,
           k_invalid_starts_with_underscore_expected,
           k_invalid_starts_with_underscore_should_succeed},
          {"invalid - ends with underscore",
           k_invalid_ends_with_underscore_input,
           k_invalid_ends_with_underscore_expected,
           k_invalid_ends_with_underscore_should_succeed},
          {"invalid - zero with underscore",
           k_invalid_zero_with_underscore_input,
           k_invalid_zero_with_underscore_expected,
           k_invalid_zero_with_underscore_should_succeed},
          {"invalid - underscore before zero",
           k_invalid_underscore_before_zero_input,
           k_invalid_underscore_before_zero_expected,
           k_invalid_underscore_before_zero_should_succeed},
          {"invalid - empty", k_invalid_empty_input, k_invalid_empty_expected, k_invalid_empty_should_succeed},
          {"invalid - letter", k_invalid_letter_input, k_invalid_letter_expected, k_invalid_letter_should_succeed},
      })
  );

  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}