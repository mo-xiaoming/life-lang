#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::String;

PARSE_TEST(String, string)

namespace {

constexpr auto k_empty_string_should_succeed = true;
constexpr auto k_empty_string_input = R"("")";
constexpr auto k_empty_string_expected = R"({"String": {"value": "\"\""}})";

constexpr auto k_simple_string_should_succeed = true;
constexpr auto k_simple_string_input = R"("hello")";
constexpr auto k_simple_string_expected = R"({"String": {"value": "\"hello\""}})";

constexpr auto k_with_escaped_quote_should_succeed = true;
constexpr auto k_with_escaped_quote_input = R"("abc\"def")";
constexpr auto k_with_escaped_quote_expected = R"({"String": {"value": "\"abc\\\"def\""}})";

constexpr auto k_with_newline_escape_should_succeed = true;
constexpr auto k_with_newline_escape_input = R"("line1\nline2")";
constexpr auto k_with_newline_escape_expected = R"({"String": {"value": "\"line1\\nline2\""}})";

constexpr auto k_with_hex_escape_should_succeed = true;
constexpr auto k_with_hex_escape_input = R"("hex\x00value")";
constexpr auto k_with_hex_escape_expected = R"({"String": {"value": "\"hex\\x00value\""}})";

constexpr auto k_all_escapes_should_succeed = true;
constexpr auto k_all_escapes_input = R"("abc\"d\n\x00yz")";
constexpr auto k_all_escapes_expected = R"({"String": {"value": "\"abc\\\"d\\n\\x00yz\""}})";

constexpr auto k_with_trailing_text_should_succeed = true;
constexpr auto k_with_trailing_text_input = R"("hello" world)";
constexpr auto k_with_trailing_text_expected = R"({"String": {"value": "\"hello\""}})";

constexpr auto k_invalid_unclosed_should_succeed = false;
constexpr auto k_invalid_unclosed_input = R"("hello)";
constexpr auto k_invalid_unclosed_expected = "{}";

constexpr auto k_invalid_no_quotes_should_succeed = false;
constexpr auto k_invalid_no_quotes_input = "hello";
constexpr auto k_invalid_no_quotes_expected = "{}";

}  // namespace

TEST_CASE("Parse String", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<String_Params>({
          {"empty string", k_empty_string_input, k_empty_string_expected, k_empty_string_should_succeed},
          {"simple string", k_simple_string_input, k_simple_string_expected, k_simple_string_should_succeed},
          {"with escaped quote", k_with_escaped_quote_input, k_with_escaped_quote_expected,
           k_with_escaped_quote_should_succeed},
          {"with newline escape", k_with_newline_escape_input, k_with_newline_escape_expected,
           k_with_newline_escape_should_succeed},
          {"with hex escape", k_with_hex_escape_input, k_with_hex_escape_expected, k_with_hex_escape_should_succeed},
          {"all escapes", k_all_escapes_input, k_all_escapes_expected, k_all_escapes_should_succeed},
          {"with trailing text", k_with_trailing_text_input, k_with_trailing_text_expected,
           k_with_trailing_text_should_succeed},
          {"invalid - unclosed", k_invalid_unclosed_input, k_invalid_unclosed_expected,
           k_invalid_unclosed_should_succeed},
          {"invalid - no quotes", k_invalid_no_quotes_input, k_invalid_no_quotes_expected,
           k_invalid_no_quotes_should_succeed},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}