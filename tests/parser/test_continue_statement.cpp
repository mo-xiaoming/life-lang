#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Statement;

PARSE_TEST(Statement, statement)

namespace {
// === Valid Continue Statements ===

// Simple continue
constexpr auto k_simple_continue_should_succeed = true;
constexpr auto k_simple_continue_input = "continue;";
inline auto const k_simple_continue_expected = test_sexp::continue_statement();

// Continue with spaces
constexpr auto k_continue_with_spaces_should_succeed = true;
constexpr auto k_continue_with_spaces_input = "continue  ;";
inline auto const k_continue_with_spaces_expected = test_sexp::continue_statement();

// === Invalid Continue Statements ===

// Missing semicolon
constexpr auto k_missing_semicolon_should_succeed = false;
constexpr auto k_missing_semicolon_input = "continue";

// Continue with value (not allowed - continue doesn't take values unlike break)
constexpr auto k_continue_with_value_should_succeed = false;
constexpr auto k_continue_with_value_input = "continue x;";

}  // namespace

TEST_CASE("Parse Continue_Statement") {
  std::vector<Statement_Params> const params_list = {
      // Valid cases
      {.name = "simple continue",
       .input = k_simple_continue_input,
       .expected = k_simple_continue_expected,
       .should_succeed = k_simple_continue_should_succeed},
      {.name = "continue with spaces",
       .input = k_continue_with_spaces_input,
       .expected = k_continue_with_spaces_expected,
       .should_succeed = k_continue_with_spaces_should_succeed},

      // Invalid cases
      {.name = "invalid - missing semicolon",
       .input = k_missing_semicolon_input,
       .expected = "",
       .should_succeed = k_missing_semicolon_should_succeed},
      {.name = "invalid - continue with value",
       .input = k_continue_with_value_input,
       .expected = "",
       .should_succeed = k_continue_with_value_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
