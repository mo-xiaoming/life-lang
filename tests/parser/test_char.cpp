#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Char;

PARSE_TEST(Char, char)

namespace {

// === Valid Character Literals ===

// Simple ASCII character
constexpr auto k_simple_char_should_succeed = true;
constexpr auto k_simple_char_input = "'a'";
inline auto const k_simple_char_expected = test_sexp::char_literal("'a'");

// Uppercase character
constexpr auto k_uppercase_char_should_succeed = true;
constexpr auto k_uppercase_char_input = "'X'";
inline auto const k_uppercase_char_expected = test_sexp::char_literal("'X'");

// Digit character
constexpr auto k_digit_char_should_succeed = true;
constexpr auto k_digit_char_input = "'9'";
inline auto const k_digit_char_expected = test_sexp::char_literal("'9'");

// Space character
constexpr auto k_space_char_should_succeed = true;
constexpr auto k_space_char_input = "' '";
inline auto const k_space_char_expected = test_sexp::char_literal("' '");

// Symbol characters
constexpr auto k_symbol_exclamation_should_succeed = true;
constexpr auto k_symbol_exclamation_input = "'!'";
inline auto const k_symbol_exclamation_expected = test_sexp::char_literal("'!'");

constexpr auto k_symbol_at_should_succeed = true;
constexpr auto k_symbol_at_input = "'@'";
inline auto const k_symbol_at_expected = test_sexp::char_literal("'@'");

// === Escaped Characters ===

// Escaped newline
constexpr auto k_escaped_newline_should_succeed = true;
constexpr auto k_escaped_newline_input = R"('\n')";
inline auto const k_escaped_newline_expected = test_sexp::char_literal(R"('\\n')");

// Escaped tab
constexpr auto k_escaped_tab_should_succeed = true;
constexpr auto k_escaped_tab_input = R"('\t')";
inline auto const k_escaped_tab_expected = test_sexp::char_literal(R"('\\t')");

// Escaped carriage return
constexpr auto k_escaped_cr_should_succeed = true;
constexpr auto k_escaped_cr_input = R"('\r')";
inline auto const k_escaped_cr_expected = test_sexp::char_literal(R"('\\r')");

// Escaped backslash
constexpr auto k_escaped_backslash_should_succeed = true;
constexpr auto k_escaped_backslash_input = R"('\\')";
inline auto const k_escaped_backslash_expected = test_sexp::char_literal(R"('\\\\')");

// Escaped single quote
constexpr auto k_escaped_quote_should_succeed = true;
constexpr auto k_escaped_quote_input = R"('\'')";
inline auto const k_escaped_quote_expected = test_sexp::char_literal(R"('\\'')");

// Escaped double quote (should also work)
constexpr auto k_escaped_double_quote_should_succeed = true;
constexpr auto k_escaped_double_quote_input = R"('\"')";
inline auto const k_escaped_double_quote_expected = test_sexp::char_literal(R"('\\\"')");

// Hex escape
constexpr auto k_hex_escape_should_succeed = true;
constexpr auto k_hex_escape_input = R"('\x41')";
inline auto const k_hex_escape_expected = test_sexp::char_literal(R"('\\x41')");

constexpr auto k_hex_escape_zero_should_succeed = true;
constexpr auto k_hex_escape_zero_input = R"('\x00')";
inline auto const k_hex_escape_zero_expected = test_sexp::char_literal(R"('\\x00')");

// === UTF-8 Characters ===

// Chinese character
constexpr auto k_utf8_chinese_should_succeed = true;
constexpr auto k_utf8_chinese_input = "'世'";
inline auto const k_utf8_chinese_expected = test_sexp::char_literal("'世'");

// Emoji
constexpr auto k_utf8_emoji_should_succeed = true;
constexpr auto k_utf8_emoji_input = "'🎉'";
inline auto const k_utf8_emoji_expected = test_sexp::char_literal("'🎉'");

// === With Trailing Content ===

constexpr auto k_with_trailing_text_should_succeed = false;  // New parser requires full consumption
constexpr auto k_with_trailing_text_input = "'a' + 'b'";
inline auto const k_with_trailing_text_expected = "{}";

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

TEST_CASE("Parse Char") {
  std::vector<Char_Params> const params_list = {
      {.name = "simple char",
       .input = k_simple_char_input,
       .expected = k_simple_char_expected,
       .should_succeed = k_simple_char_should_succeed},
      {.name = "uppercase char",
       .input = k_uppercase_char_input,
       .expected = k_uppercase_char_expected,
       .should_succeed = k_uppercase_char_should_succeed},
      {.name = "digit char",
       .input = k_digit_char_input,
       .expected = k_digit_char_expected,
       .should_succeed = k_digit_char_should_succeed},
      {.name = "space char",
       .input = k_space_char_input,
       .expected = k_space_char_expected,
       .should_succeed = k_space_char_should_succeed},
      {.name = "symbol exclamation",
       .input = k_symbol_exclamation_input,
       .expected = k_symbol_exclamation_expected,
       .should_succeed = k_symbol_exclamation_should_succeed},
      {.name = "symbol at",
       .input = k_symbol_at_input,
       .expected = k_symbol_at_expected,
       .should_succeed = k_symbol_at_should_succeed},
      {.name = "escaped newline",
       .input = k_escaped_newline_input,
       .expected = k_escaped_newline_expected,
       .should_succeed = k_escaped_newline_should_succeed},
      {.name = "escaped tab",
       .input = k_escaped_tab_input,
       .expected = k_escaped_tab_expected,
       .should_succeed = k_escaped_tab_should_succeed},
      {.name = "escaped cr",
       .input = k_escaped_cr_input,
       .expected = k_escaped_cr_expected,
       .should_succeed = k_escaped_cr_should_succeed},
      {.name = "escaped backslash",
       .input = k_escaped_backslash_input,
       .expected = k_escaped_backslash_expected,
       .should_succeed = k_escaped_backslash_should_succeed},
      {.name = "escaped quote",
       .input = k_escaped_quote_input,
       .expected = k_escaped_quote_expected,
       .should_succeed = k_escaped_quote_should_succeed},
      {.name = "escaped double quote",
       .input = k_escaped_double_quote_input,
       .expected = k_escaped_double_quote_expected,
       .should_succeed = k_escaped_double_quote_should_succeed},
      {.name = "hex escape",
       .input = k_hex_escape_input,
       .expected = k_hex_escape_expected,
       .should_succeed = k_hex_escape_should_succeed},
      {.name = "hex escape zero",
       .input = k_hex_escape_zero_input,
       .expected = k_hex_escape_zero_expected,
       .should_succeed = k_hex_escape_zero_should_succeed},
      {.name = "utf8 chinese",
       .input = k_utf8_chinese_input,
       .expected = k_utf8_chinese_expected,
       .should_succeed = k_utf8_chinese_should_succeed},
      {.name = "utf8 emoji",
       .input = k_utf8_emoji_input,
       .expected = k_utf8_emoji_expected,
       .should_succeed = k_utf8_emoji_should_succeed},
      {.name = "with trailing text",
       .input = k_with_trailing_text_input,
       .expected = k_with_trailing_text_expected,
       .should_succeed = k_with_trailing_text_should_succeed},
      {.name = "invalid - unclosed",
       .input = k_invalid_unclosed_input,
       .expected = k_invalid_unclosed_expected,
       .should_succeed = k_invalid_unclosed_should_succeed},
      {.name = "invalid - empty",
       .input = k_invalid_empty_input,
       .expected = k_invalid_empty_expected,
       .should_succeed = k_invalid_empty_should_succeed},
      {.name = "invalid - multiple chars",
       .input = k_invalid_multiple_chars_input,
       .expected = k_invalid_multiple_chars_expected,
       .should_succeed = k_invalid_multiple_chars_should_succeed},
      {.name = "invalid - double quotes",
       .input = k_invalid_double_quotes_input,
       .expected = k_invalid_double_quotes_expected,
       .should_succeed = k_invalid_double_quotes_should_succeed},
      {.name = "invalid - no quotes",
       .input = k_invalid_no_quotes_input,
       .expected = k_invalid_no_quotes_expected,
       .should_succeed = k_invalid_no_quotes_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
