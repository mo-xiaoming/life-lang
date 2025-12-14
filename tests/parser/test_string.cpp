#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::String;

PARSE_TEST(String, string)

namespace {

constexpr auto k_empty_string_should_succeed = true;
constexpr auto k_empty_string_input = R"("")";
inline auto const k_empty_string_expected = test_sexp::string(R"("")");

constexpr auto k_simple_string_should_succeed = true;
constexpr auto k_simple_string_input = R"("hello")";
inline auto const k_simple_string_expected = test_sexp::string(R"("hello")");

constexpr auto k_with_escaped_quote_should_succeed = true;
constexpr auto k_with_escaped_quote_input = R"("abc\"def")";
inline auto const k_with_escaped_quote_expected = test_sexp::string(R"("abc\"def")");

constexpr auto k_with_newline_escape_should_succeed = true;
constexpr auto k_with_newline_escape_input = R"("line1\nline2")";
inline auto const k_with_newline_escape_expected = test_sexp::string(R"("line1\nline2")");

constexpr auto k_with_hex_escape_should_succeed = true;
constexpr auto k_with_hex_escape_input = R"("hex\x00value")";
inline auto const k_with_hex_escape_expected = test_sexp::string(R"("hex\x00value")");

constexpr auto k_all_escapes_should_succeed = true;
constexpr auto k_all_escapes_input = R"("abc\"d\n\x00yz")";
inline auto const k_all_escapes_expected = test_sexp::string(R"("abc\"d\n\x00yz")");

constexpr auto k_with_trailing_text_should_succeed = false;
constexpr auto k_with_trailing_text_input = R"("hello" world)";
constexpr auto k_with_trailing_text_expected = R"({
  "String": {
    "value": "\"hello\""
  }
})";

constexpr auto k_invalid_unclosed_should_succeed = false;
constexpr auto k_invalid_unclosed_input = R"("hello)";
constexpr auto k_invalid_unclosed_expected = "{}";

constexpr auto k_invalid_no_quotes_should_succeed = false;
constexpr auto k_invalid_no_quotes_input = "hello";
constexpr auto k_invalid_no_quotes_expected = "{}";

}  // namespace

TEST_CASE("Parse String") {
  std::vector<String_Params> const params_list = {
      {.name = "empty string",
       .input = k_empty_string_input,
       .expected = k_empty_string_expected,
       .should_succeed = k_empty_string_should_succeed},
      {.name = "simple string",
       .input = k_simple_string_input,
       .expected = k_simple_string_expected,
       .should_succeed = k_simple_string_should_succeed},
      {.name = "with escaped quote",
       .input = k_with_escaped_quote_input,
       .expected = k_with_escaped_quote_expected,
       .should_succeed = k_with_escaped_quote_should_succeed},
      {.name = "with newline escape",
       .input = k_with_newline_escape_input,
       .expected = k_with_newline_escape_expected,
       .should_succeed = k_with_newline_escape_should_succeed},
      {.name = "with hex escape",
       .input = k_with_hex_escape_input,
       .expected = k_with_hex_escape_expected,
       .should_succeed = k_with_hex_escape_should_succeed},
      {.name = "all escapes",
       .input = k_all_escapes_input,
       .expected = k_all_escapes_expected,
       .should_succeed = k_all_escapes_should_succeed},
      {.name = "with trailing text",
       .input = k_with_trailing_text_input,
       .expected = k_with_trailing_text_expected,
       .should_succeed = k_with_trailing_text_should_succeed},
      {.name = "invalid - unclosed",
       .input = k_invalid_unclosed_input,
       .expected = k_invalid_unclosed_expected,
       .should_succeed = k_invalid_unclosed_should_succeed},
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