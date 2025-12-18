#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Bool_Literal;

PARSE_TEST(Bool_Literal, bool_literal)

namespace {

// === Valid Boolean Literals ===

// Simple true
constexpr auto k_true_literal_should_succeed = true;
constexpr auto k_true_literal_input = "true";
inline auto const k_true_literal_expected = "(bool true)";

// Simple false
constexpr auto k_false_literal_should_succeed = true;
constexpr auto k_false_literal_input = "false";
inline auto const k_false_literal_expected = "(bool false)";

// True with whitespace
constexpr auto k_true_with_whitespace_should_succeed = true;
constexpr auto k_true_with_whitespace_input = "  true  ";
inline auto const k_true_with_whitespace_expected = "(bool true)";

// False with whitespace
constexpr auto k_false_with_whitespace_should_succeed = true;
constexpr auto k_false_with_whitespace_input = "  false  ";
inline auto const k_false_with_whitespace_expected = "(bool false)";

// True with line comments
constexpr auto k_true_with_comment_should_succeed = true;
constexpr auto k_true_with_comment_input = "// comment\ntrue";
inline auto const k_true_with_comment_expected = "(bool true)";

// False with block comments
constexpr auto k_false_with_block_comment_should_succeed = true;
constexpr auto k_false_with_block_comment_input = "/* comment */ false";
inline auto const k_false_with_block_comment_expected = "(bool false)";

// === Invalid Cases - Not Boolean Literals (should fail) ===

// Boolean followed by identifier continuation (not a literal, should be parsed as identifier)
// These are invalid for parse_bool_literal but valid as identifiers in parse_primary_expr
constexpr auto k_true_suffix_should_succeed = false;
constexpr auto k_true_suffix_input = "true_value";

constexpr auto k_false_suffix_should_succeed = false;
constexpr auto k_false_suffix_input = "false_value";

constexpr auto k_true_underscore_should_succeed = false;
constexpr auto k_true_underscore_input = "true_";

constexpr auto k_false_underscore_should_succeed = false;
constexpr auto k_false_underscore_input = "false_";

// Case variations (not supported - case-sensitive)
constexpr auto k_uppercase_true_should_succeed = false;
constexpr auto k_uppercase_true_input = "True";

constexpr auto k_uppercase_false_should_succeed = false;
constexpr auto k_uppercase_false_input = "False";

constexpr auto k_all_caps_true_should_succeed = false;
constexpr auto k_all_caps_true_input = "TRUE";

constexpr auto k_all_caps_false_should_succeed = false;
constexpr auto k_all_caps_false_input = "FALSE";

// Empty input
constexpr auto k_empty_input_should_succeed = false;
constexpr auto k_empty_input_input = "";

// Wrong keyword
constexpr auto k_wrong_keyword_should_succeed = false;
constexpr auto k_wrong_keyword_input = "yes";

}  // namespace

TEST_CASE("Parse Bool_Literal") {
  std::vector<Bool_Literal_Params> const params_list = {
      // Valid cases
      {.name = "true literal",
       .input = k_true_literal_input,
       .expected = k_true_literal_expected,
       .should_succeed = k_true_literal_should_succeed},
      {.name = "false literal",
       .input = k_false_literal_input,
       .expected = k_false_literal_expected,
       .should_succeed = k_false_literal_should_succeed},
      {.name = "true with whitespace",
       .input = k_true_with_whitespace_input,
       .expected = k_true_with_whitespace_expected,
       .should_succeed = k_true_with_whitespace_should_succeed},
      {.name = "false with whitespace",
       .input = k_false_with_whitespace_input,
       .expected = k_false_with_whitespace_expected,
       .should_succeed = k_false_with_whitespace_should_succeed},
      {.name = "true with comment",
       .input = k_true_with_comment_input,
       .expected = k_true_with_comment_expected,
       .should_succeed = k_true_with_comment_should_succeed},
      {.name = "false with block comment",
       .input = k_false_with_block_comment_input,
       .expected = k_false_with_block_comment_expected,
       .should_succeed = k_false_with_block_comment_should_succeed},

      // Invalid cases - should fail
      {.name = "true with suffix",
       .input = k_true_suffix_input,
       .expected = std::nullopt,
       .should_succeed = k_true_suffix_should_succeed},
      {.name = "false with suffix",
       .input = k_false_suffix_input,
       .expected = std::nullopt,
       .should_succeed = k_false_suffix_should_succeed},
      {.name = "true with underscore",
       .input = k_true_underscore_input,
       .expected = std::nullopt,
       .should_succeed = k_true_underscore_should_succeed},
      {.name = "false with underscore",
       .input = k_false_underscore_input,
       .expected = std::nullopt,
       .should_succeed = k_false_underscore_should_succeed},
      {.name = "uppercase True",
       .input = k_uppercase_true_input,
       .expected = std::nullopt,
       .should_succeed = k_uppercase_true_should_succeed},
      {.name = "uppercase False",
       .input = k_uppercase_false_input,
       .expected = std::nullopt,
       .should_succeed = k_uppercase_false_should_succeed},
      {.name = "all caps TRUE",
       .input = k_all_caps_true_input,
       .expected = std::nullopt,
       .should_succeed = k_all_caps_true_should_succeed},
      {.name = "all caps FALSE",
       .input = k_all_caps_false_input,
       .expected = std::nullopt,
       .should_succeed = k_all_caps_false_should_succeed},
      {.name = "empty input",
       .input = k_empty_input_input,
       .expected = std::nullopt,
       .should_succeed = k_empty_input_should_succeed},
      {.name = "wrong keyword",
       .input = k_wrong_keyword_input,
       .expected = std::nullopt,
       .should_succeed = k_wrong_keyword_should_succeed},
  };

  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
