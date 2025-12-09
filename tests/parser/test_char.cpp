#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Char;

PARSE_TEST(Char, char)

namespace {

// === Valid Character Literals ===

// Simple ASCII character
constexpr auto k_simple_char_should_succeed = true;
constexpr auto k_simple_char_input = "'a'";
inline auto const k_simple_char_expected = test_json::char_literal("'a'");

// Uppercase character
constexpr auto k_uppercase_char_should_succeed = true;
constexpr auto k_uppercase_char_input = "'X'";
inline auto const k_uppercase_char_expected = test_json::char_literal("'X'");

// Digit character
constexpr auto k_digit_char_should_succeed = true;
constexpr auto k_digit_char_input = "'9'";
inline auto const k_digit_char_expected = test_json::char_literal("'9'");

// Space character
constexpr auto k_space_char_should_succeed = true;
constexpr auto k_space_char_input = "' '";
inline auto const k_space_char_expected = test_json::char_literal("' '");

// Symbol characters
constexpr auto k_symbol_exclamation_should_succeed = true;
constexpr auto k_symbol_exclamation_input = "'!'";
inline auto const k_symbol_exclamation_expected = test_json::char_literal("'!'");

constexpr auto k_symbol_at_should_succeed = true;
constexpr auto k_symbol_at_input = "'@'";
inline auto const k_symbol_at_expected = test_json::char_literal("'@'");

// === Escaped Characters ===

// Escaped newline
constexpr auto k_escaped_newline_should_succeed = true;
constexpr auto k_escaped_newline_input = R"('\n')";
inline auto const k_escaped_newline_expected = test_json::char_literal(R"('\\n')");

// Escaped tab
constexpr auto k_escaped_tab_should_succeed = true;
constexpr auto k_escaped_tab_input = R"('\t')";
inline auto const k_escaped_tab_expected = test_json::char_literal(R"('\\t')");

// Escaped carriage return
constexpr auto k_escaped_cr_should_succeed = true;
constexpr auto k_escaped_cr_input = R"('\r')";
inline auto const k_escaped_cr_expected = test_json::char_literal(R"('\\r')");

// Escaped backslash
constexpr auto k_escaped_backslash_should_succeed = true;
constexpr auto k_escaped_backslash_input = R"('\\')";
inline auto const k_escaped_backslash_expected = test_json::char_literal(R"('\\\\')");

// Escaped single quote
constexpr auto k_escaped_quote_should_succeed = true;
constexpr auto k_escaped_quote_input = R"('\'')";
inline auto const k_escaped_quote_expected = test_json::char_literal(R"('\\'')");

// Escaped double quote (should also work)
constexpr auto k_escaped_double_quote_should_succeed = true;
constexpr auto k_escaped_double_quote_input = R"('\"')";
inline auto const k_escaped_double_quote_expected = test_json::char_literal(R"('\\\"')");

// Hex escape
constexpr auto k_hex_escape_should_succeed = true;
constexpr auto k_hex_escape_input = R"('\x41')";
inline auto const k_hex_escape_expected = test_json::char_literal(R"('\\x41')");

constexpr auto k_hex_escape_zero_should_succeed = true;
constexpr auto k_hex_escape_zero_input = R"('\x00')";
inline auto const k_hex_escape_zero_expected = test_json::char_literal(R"('\\x00')");

// === UTF-8 Characters ===

// Chinese character
constexpr auto k_utf8_chinese_should_succeed = true;
constexpr auto k_utf8_chinese_input = "'世'";
inline auto const k_utf8_chinese_expected = test_json::char_literal("'世'");

// Emoji
constexpr auto k_utf8_emoji_should_succeed = true;
constexpr auto k_utf8_emoji_input = "'🎉'";
inline auto const k_utf8_emoji_expected = test_json::char_literal("'🎉'");

// === With Trailing Content ===

constexpr auto k_with_trailing_text_should_succeed = true;
constexpr auto k_with_trailing_text_input = "'a' + 'b'";
inline auto const k_with_trailing_text_expected = test_json::char_literal("'a'");

// === Invalid Character Literals ===

// Unclosed character literal
constexpr auto k_invalid_unclosed_should_succeed = false;
constexpr auto k_invalid_unclosed_input = "'a";
constexpr auto k_invalid_unclosed_expected = "{}";

// Empty character literal
constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "''";
constexpr auto k_invalid_empty_expected = "{}";

// Multiple characters (should fail - only one char allowed)
constexpr auto k_invalid_multiple_chars_should_succeed = false;
constexpr auto k_invalid_multiple_chars_input = "'ab'";
constexpr auto k_invalid_multiple_chars_expected = "{}";

// Double quotes instead of single quotes
constexpr auto k_invalid_double_quotes_should_succeed = false;
constexpr auto k_invalid_double_quotes_input = "\"a\"";
constexpr auto k_invalid_double_quotes_expected = "{}";

// No quotes
constexpr auto k_invalid_no_quotes_should_succeed = false;
constexpr auto k_invalid_no_quotes_input = "a";
constexpr auto k_invalid_no_quotes_expected = "{}";

}  // namespace

TEST_CASE("Parse Char", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Char_Params>({
          {"simple char", k_simple_char_input, k_simple_char_expected, k_simple_char_should_succeed},
          {"uppercase char", k_uppercase_char_input, k_uppercase_char_expected, k_uppercase_char_should_succeed},
          {"digit char", k_digit_char_input, k_digit_char_expected, k_digit_char_should_succeed},
          {"space char", k_space_char_input, k_space_char_expected, k_space_char_should_succeed},
          {"symbol exclamation",
           k_symbol_exclamation_input,
           k_symbol_exclamation_expected,
           k_symbol_exclamation_should_succeed},
          {"symbol at", k_symbol_at_input, k_symbol_at_expected, k_symbol_at_should_succeed},
          {"escaped newline", k_escaped_newline_input, k_escaped_newline_expected, k_escaped_newline_should_succeed},
          {"escaped tab", k_escaped_tab_input, k_escaped_tab_expected, k_escaped_tab_should_succeed},
          {"escaped cr", k_escaped_cr_input, k_escaped_cr_expected, k_escaped_cr_should_succeed},
          {"escaped backslash",
           k_escaped_backslash_input,
           k_escaped_backslash_expected,
           k_escaped_backslash_should_succeed},
          {"escaped quote", k_escaped_quote_input, k_escaped_quote_expected, k_escaped_quote_should_succeed},
          {"escaped double quote",
           k_escaped_double_quote_input,
           k_escaped_double_quote_expected,
           k_escaped_double_quote_should_succeed},
          {"hex escape", k_hex_escape_input, k_hex_escape_expected, k_hex_escape_should_succeed},
          {"hex escape zero", k_hex_escape_zero_input, k_hex_escape_zero_expected, k_hex_escape_zero_should_succeed},
          {"utf8 chinese", k_utf8_chinese_input, k_utf8_chinese_expected, k_utf8_chinese_should_succeed},
          {"utf8 emoji", k_utf8_emoji_input, k_utf8_emoji_expected, k_utf8_emoji_should_succeed},
          {"with trailing text",
           k_with_trailing_text_input,
           k_with_trailing_text_expected,
           k_with_trailing_text_should_succeed},
          {"invalid - unclosed",
           k_invalid_unclosed_input,
           k_invalid_unclosed_expected,
           k_invalid_unclosed_should_succeed},
          {"invalid - empty", k_invalid_empty_input, k_invalid_empty_expected, k_invalid_empty_should_succeed},
          {"invalid - multiple chars",
           k_invalid_multiple_chars_input,
           k_invalid_multiple_chars_expected,
           k_invalid_multiple_chars_should_succeed},
          {"invalid - double quotes",
           k_invalid_double_quotes_input,
           k_invalid_double_quotes_expected,
           k_invalid_double_quotes_should_succeed},
          {"invalid - no quotes",
           k_invalid_no_quotes_input,
           k_invalid_no_quotes_expected,
           k_invalid_no_quotes_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
