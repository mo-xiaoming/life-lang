#include "internal_rules.hpp"
#include "utils.hpp"
using life_lang::ast::Statement;

PARSE_TEST(Statement, statement)

namespace {
// === Unit Type Tests ===

// Function with unit return type
constexpr auto k_unit_return_should_succeed = true;
constexpr auto k_unit_return_input = "fn test(): () { return (); }";
inline auto const k_unit_return_expected = test_json::func_def(
    test_json::func_decl("test", {}, {}, test_json::type_name("()")),
    test_json::block({test_json::return_statement(R"({"Unit_Literal": {}})")})
);

}  // namespace

TEST_CASE("Parse Unit Type", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Statement_Params>({
          {"unit return", k_unit_return_input, k_unit_return_expected, k_unit_return_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
