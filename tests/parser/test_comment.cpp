#include "internal_rules.hpp"
#include "utils.hpp"

// Test comments using Integer parser (simplest rule)
using life_lang::ast::Integer;

PARSE_TEST(Integer, integer)

namespace {

// Line comment tests
constexpr auto k_line_comment_after_should_succeed = true;
constexpr auto k_line_comment_after_input = "42 // this is a comment";
constexpr auto k_line_comment_after_expected = R"({
  "Integer": {
    "value": "42"
  }
})";

constexpr auto k_line_comment_before_should_succeed = true;
constexpr auto k_line_comment_before_input = "// comment\n123";
constexpr auto k_line_comment_before_expected = R"({
  "Integer": {
    "value": "123"
  }
})";

constexpr auto k_line_comment_multiple_should_succeed = true;
constexpr auto k_line_comment_multiple_input = "// first comment\n// second comment\n456";
constexpr auto k_line_comment_multiple_expected = R"({
  "Integer": {
    "value": "456"
  }
})";

constexpr auto k_line_comment_empty_should_succeed = true;
constexpr auto k_line_comment_empty_input = "789 //";
constexpr auto k_line_comment_empty_expected = R"({
  "Integer": {
    "value": "789"
  }
})";

constexpr auto k_line_comment_special_chars_should_succeed = true;
constexpr auto k_line_comment_special_chars_input = "99 // comment with /* */ and other chars!@#$%";
constexpr auto k_line_comment_special_chars_expected = R"({
  "Integer": {
    "value": "99"
  }
})";

// Block comment tests
constexpr auto k_block_comment_after_should_succeed = true;
constexpr auto k_block_comment_after_input = "42 /* block comment */";
constexpr auto k_block_comment_after_expected = R"({
  "Integer": {
    "value": "42"
  }
})";

constexpr auto k_block_comment_before_should_succeed = true;
constexpr auto k_block_comment_before_input = "/* comment */ 123";
constexpr auto k_block_comment_before_expected = R"({
  "Integer": {
    "value": "123"
  }
})";

constexpr auto k_block_comment_multiline_should_succeed = true;
constexpr auto k_block_comment_multiline_input = R"(/* multi
line
comment */ 456)";
constexpr auto k_block_comment_multiline_expected = R"({
  "Integer": {
    "value": "456"
  }
})";

constexpr auto k_block_comment_empty_should_succeed = true;
constexpr auto k_block_comment_empty_input = "789 /**/";
constexpr auto k_block_comment_empty_expected = R"({
  "Integer": {
    "value": "789"
  }
})";

constexpr auto k_block_comment_with_newlines_should_succeed = true;
constexpr auto k_block_comment_with_newlines_input = R"(/*
This is a block comment
with multiple lines
*/ 999)";
constexpr auto k_block_comment_with_newlines_expected = R"({
  "Integer": {
    "value": "999"
  }
})";

// Mixed comment tests
constexpr auto k_mixed_comments_should_succeed = true;
constexpr auto k_mixed_comments_input = "// line\n/* block */ 111 /* another */ // end";
constexpr auto k_mixed_comments_expected = R"({
  "Integer": {
    "value": "111"
  }
})";

constexpr auto k_comment_with_slashes_should_succeed = true;
constexpr auto k_comment_with_slashes_input = "222 /* comment with // inside */";
constexpr auto k_comment_with_slashes_expected = R"({
  "Integer": {
    "value": "222"
  }
})";

// Note: Unclosed block comments consume to end of input (similar to C/C++ behavior)
// The integer before the comment still parses successfully
constexpr auto k_unclosed_block_comment_should_succeed = true;
constexpr auto k_unclosed_block_comment_input = "42 /* unclosed";
constexpr auto k_unclosed_block_comment_expected = R"({
  "Integer": {
    "value": "42"
  }
})";

// Invalid cases
constexpr auto k_only_comment_should_succeed = false;
constexpr auto k_only_comment_input = "// just a comment";
constexpr auto k_only_comment_expected = "{}";

constexpr auto k_only_block_comment_should_succeed = false;
constexpr auto k_only_block_comment_input = "/* just a block comment */";
constexpr auto k_only_block_comment_expected = "{}";

}  // namespace

TEST_CASE("Parse Integer with comments", "[parser][comment]") {
  auto const params = GENERATE(
      Catch::Generators::values<Integer_Params>({
          {"line comment after",
           k_line_comment_after_input,
           k_line_comment_after_expected,
           k_line_comment_after_should_succeed},
          {"line comment before",
           k_line_comment_before_input,
           k_line_comment_before_expected,
           k_line_comment_before_should_succeed},
          {"multiple line comments",
           k_line_comment_multiple_input,
           k_line_comment_multiple_expected,
           k_line_comment_multiple_should_succeed},
          {"empty line comment",
           k_line_comment_empty_input,
           k_line_comment_empty_expected,
           k_line_comment_empty_should_succeed},
          {"line comment with special chars",
           k_line_comment_special_chars_input,
           k_line_comment_special_chars_expected,
           k_line_comment_special_chars_should_succeed},
          {"block comment after",
           k_block_comment_after_input,
           k_block_comment_after_expected,
           k_block_comment_after_should_succeed},
          {"block comment before",
           k_block_comment_before_input,
           k_block_comment_before_expected,
           k_block_comment_before_should_succeed},
          {"multiline block comment",
           k_block_comment_multiline_input,
           k_block_comment_multiline_expected,
           k_block_comment_multiline_should_succeed},
          {"empty block comment",
           k_block_comment_empty_input,
           k_block_comment_empty_expected,
           k_block_comment_empty_should_succeed},
          {"block comment with newlines",
           k_block_comment_with_newlines_input,
           k_block_comment_with_newlines_expected,
           k_block_comment_with_newlines_should_succeed},
          {"mixed comments", k_mixed_comments_input, k_mixed_comments_expected, k_mixed_comments_should_succeed},
          {"comment with slashes inside",
           k_comment_with_slashes_input,
           k_comment_with_slashes_expected,
           k_comment_with_slashes_should_succeed},
          {"unclosed block comment",
           k_unclosed_block_comment_input,
           k_unclosed_block_comment_expected,
           k_unclosed_block_comment_should_succeed},
          {"only line comment", k_only_comment_input, k_only_comment_expected, k_only_comment_should_succeed},
          {"only block comment",
           k_only_block_comment_input,
           k_only_block_comment_expected,
           k_only_block_comment_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
