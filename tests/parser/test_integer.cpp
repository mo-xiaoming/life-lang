#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Integer;

PARSE_TEST(Integer, integer)

namespace {

constexpr auto k_zero_should_succeed = true;
constexpr auto k_zero_input = "0";
constexpr auto k_zero_expected = R"((integer "0"))";

constexpr auto k_simple_number_should_succeed = true;
constexpr auto k_simple_number_input = "123";
constexpr auto k_simple_number_expected = R"((integer "123"))";

constexpr auto k_large_number_should_succeed = true;
constexpr auto k_large_number_input = "987654321";
constexpr auto k_large_number_expected = R"((integer "987654321"))";

constexpr auto k_with_underscores_should_succeed = true;
constexpr auto k_with_underscores_input = "12_34_5";
constexpr auto k_with_underscores_expected = R"((integer "12345"))";

constexpr auto k_multiple_underscores_should_succeed = true;
constexpr auto k_multiple_underscores_input = "1_2_3_4";
constexpr auto k_multiple_underscores_expected = R"((integer "1234"))";

constexpr auto k_with_trailing_text_should_succeed = false;  // New parser requires full consumption
constexpr auto k_with_trailing_text_input = "42 abc";

// With type suffixes
constexpr auto k_with_i32_suffix_should_succeed = true;
constexpr auto k_with_i32_suffix_input = "42I32";
constexpr auto k_with_i32_suffix_expected = R"((integer "42" "I32"))";

constexpr auto k_with_u8_suffix_should_succeed = true;
constexpr auto k_with_u8_suffix_input = "255U8";
constexpr auto k_with_u8_suffix_expected = R"((integer "255" "U8"))";

constexpr auto k_with_i64_suffix_should_succeed = true;
constexpr auto k_with_i64_suffix_input = "1000I64";
constexpr auto k_with_i64_suffix_expected = R"((integer "1000" "I64"))";

constexpr auto k_with_suffix_and_underscores_should_succeed = true;
constexpr auto k_with_suffix_and_underscores_input = "1_000_000I32";
constexpr auto k_with_suffix_and_underscores_expected = R"((integer "1000000" "I32"))";

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
      {.name = "invalid - letter",
       .input = k_invalid_letter_input,
       .expected = std::nullopt,
       .should_succeed = k_invalid_letter_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}