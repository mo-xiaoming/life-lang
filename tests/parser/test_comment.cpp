#include "internal_rules.hpp"
#include "utils.hpp"

// Test comments using Integer parser (simplest rule)
using life_lang::ast::Integer;

PARSE_TEST(Integer, integer)

namespace {

// Line comment tests
constexpr auto k_line_comment_after_should_succeed = true;
constexpr auto k_line_comment_after_input = "42 // this is a comment";
constexpr auto k_line_comment_after_expected = R"((integer "42"))";

constexpr auto k_line_comment_before_should_succeed = true;
constexpr auto k_line_comment_before_input = "// comment\n123";
constexpr auto k_line_comment_before_expected = R"((integer "123"))";

constexpr auto k_line_comment_multiple_should_succeed = true;
constexpr auto k_line_comment_multiple_input = "// first comment\n// second comment\n456";
constexpr auto k_line_comment_multiple_expected = R"((integer "456"))";

constexpr auto k_line_comment_empty_should_succeed = true;
constexpr auto k_line_comment_empty_input = "789 //";
constexpr auto k_line_comment_empty_expected = R"((integer "789"))";

constexpr auto k_line_comment_special_chars_should_succeed = true;
constexpr auto k_line_comment_special_chars_input = "99 // comment with /* */ and other chars!@#$%";
constexpr auto k_line_comment_special_chars_expected = R"((integer "99"))";

// Block comment tests
constexpr auto k_block_comment_after_should_succeed = true;
constexpr auto k_block_comment_after_input = "42 /* block comment */";
constexpr auto k_block_comment_after_expected = R"((integer "42"))";

constexpr auto k_block_comment_before_should_succeed = true;
constexpr auto k_block_comment_before_input = "/* comment */ 123";
constexpr auto k_block_comment_before_expected = R"((integer "123"))";

constexpr auto k_block_comment_multiline_should_succeed = true;
constexpr auto k_block_comment_multiline_input = R"(/* multi
line
comment */ 456)";
constexpr auto k_block_comment_multiline_expected = R"((integer "456"))";

constexpr auto k_block_comment_empty_should_succeed = true;
constexpr auto k_block_comment_empty_input = "789 /**/";
constexpr auto k_block_comment_empty_expected = R"((integer "789"))";

constexpr auto k_block_comment_with_newlines_should_succeed = true;
constexpr auto k_block_comment_with_newlines_input = R"(/*
This is a block comment
with multiple lines
*/ 999)";
constexpr auto k_block_comment_with_newlines_expected = R"((integer "999"))";

// Mixed comment tests
constexpr auto k_mixed_comments_should_succeed = true;
constexpr auto k_mixed_comments_input = "// line\n/* block */ 111 /* another */ // end";
constexpr auto k_mixed_comments_expected = R"((integer "111"))";

constexpr auto k_comment_with_slashes_should_succeed = true;
constexpr auto k_comment_with_slashes_input = "222 /* comment with // inside */";
constexpr auto k_comment_with_slashes_expected = R"((integer "222"))";

// Note: Unclosed block comments consume to end of input (similar to C/C++ behavior)
// The integer before the comment still parses successfully
constexpr auto k_unclosed_block_comment_should_succeed = true;
constexpr auto k_unclosed_block_comment_input = "42 /* unclosed";
constexpr auto k_unclosed_block_comment_expected = R"((integer "42"))";

// Invalid cases
constexpr auto k_only_comment_should_succeed = false;
constexpr auto k_only_comment_input = "// just a comment";
constexpr auto k_only_comment_expected = "{}";

constexpr auto k_only_block_comment_should_succeed = false;
constexpr auto k_only_block_comment_input = "/* just a block comment */";
constexpr auto k_only_block_comment_expected = "{}";

}  // namespace

TEST_CASE("Parse Integer with comments") {
  std::vector<Integer_Params> const params_list = {
      {.name = "line comment after",
       .input = k_line_comment_after_input,
       .expected = k_line_comment_after_expected,
       .should_succeed = k_line_comment_after_should_succeed},
      {.name = "line comment before",
       .input = k_line_comment_before_input,
       .expected = k_line_comment_before_expected,
       .should_succeed = k_line_comment_before_should_succeed},
      {.name = "multiple line comments",
       .input = k_line_comment_multiple_input,
       .expected = k_line_comment_multiple_expected,
       .should_succeed = k_line_comment_multiple_should_succeed},
      {.name = "empty line comment",
       .input = k_line_comment_empty_input,
       .expected = k_line_comment_empty_expected,
       .should_succeed = k_line_comment_empty_should_succeed},
      {.name = "line comment with special chars",
       .input = k_line_comment_special_chars_input,
       .expected = k_line_comment_special_chars_expected,
       .should_succeed = k_line_comment_special_chars_should_succeed},
      {.name = "block comment after",
       .input = k_block_comment_after_input,
       .expected = k_block_comment_after_expected,
       .should_succeed = k_block_comment_after_should_succeed},
      {.name = "block comment before",
       .input = k_block_comment_before_input,
       .expected = k_block_comment_before_expected,
       .should_succeed = k_block_comment_before_should_succeed},
      {.name = "multiline block comment",
       .input = k_block_comment_multiline_input,
       .expected = k_block_comment_multiline_expected,
       .should_succeed = k_block_comment_multiline_should_succeed},
      {.name = "empty block comment",
       .input = k_block_comment_empty_input,
       .expected = k_block_comment_empty_expected,
       .should_succeed = k_block_comment_empty_should_succeed},
      {.name = "block comment with newlines",
       .input = k_block_comment_with_newlines_input,
       .expected = k_block_comment_with_newlines_expected,
       .should_succeed = k_block_comment_with_newlines_should_succeed},
      {.name = "mixed comments",
       .input = k_mixed_comments_input,
       .expected = k_mixed_comments_expected,
       .should_succeed = k_mixed_comments_should_succeed},
      {.name = "comment with slashes inside",
       .input = k_comment_with_slashes_input,
       .expected = k_comment_with_slashes_expected,
       .should_succeed = k_comment_with_slashes_should_succeed},
      {.name = "unclosed block comment",
       .input = k_unclosed_block_comment_input,
       .expected = k_unclosed_block_comment_expected,
       .should_succeed = k_unclosed_block_comment_should_succeed},
      {.name = "only line comment",
       .input = k_only_comment_input,
       .expected = k_only_comment_expected,
       .should_succeed = k_only_comment_should_succeed},
      {.name = "only block comment",
       .input = k_only_block_comment_input,
       .expected = k_only_block_comment_expected,
       .should_succeed = k_only_block_comment_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
