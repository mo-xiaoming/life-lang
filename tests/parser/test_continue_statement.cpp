#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Statement;

PARSE_TEST(Statement, statement)

namespace {
// === Valid Continue Statements ===

// Simple continue
constexpr auto k_simple_continue_should_succeed = true;
constexpr auto k_simple_continue_input = "continue;";
inline auto const k_simple_continue_expected = test_json::continue_statement();

// Continue with spaces
constexpr auto k_continue_with_spaces_should_succeed = true;
constexpr auto k_continue_with_spaces_input = "continue  ;";
inline auto const k_continue_with_spaces_expected = test_json::continue_statement();

// === Invalid Continue Statements ===

// Missing semicolon
constexpr auto k_missing_semicolon_should_succeed = false;
constexpr auto k_missing_semicolon_input = "continue";

// Continue with value (not allowed - continue doesn't take values unlike break)
constexpr auto k_continue_with_value_should_succeed = false;
constexpr auto k_continue_with_value_input = "continue x;";

}  // namespace

TEST_CASE("Parse Continue_Statement", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Statement_Params>({
          // Valid cases
          {"simple continue", k_simple_continue_input, k_simple_continue_expected, k_simple_continue_should_succeed},
          {"continue with spaces", k_continue_with_spaces_input, k_continue_with_spaces_expected,
           k_continue_with_spaces_should_succeed},

          // Invalid cases
          {"invalid - missing semicolon", k_missing_semicolon_input, "", k_missing_semicolon_should_succeed},
          {"invalid - continue with value", k_continue_with_value_input, "", k_continue_with_value_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) { check_parse(params); }
}
