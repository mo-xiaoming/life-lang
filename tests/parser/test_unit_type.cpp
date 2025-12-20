#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Statement;

PARSE_TEST(Statement, statement)

namespace {
// === Unit Type Tests ===

// Function with unit return type
constexpr auto k_unit_return_should_succeed = true;
constexpr auto k_unit_return_input = "fn test(): () { return (); }";
inline auto const k_unit_return_expected = test_sexp::func_def(
    test_sexp::func_decl("test", {}, {}, test_sexp::type_name("()")),
    test_sexp::block({test_sexp::return_statement(R"(unit)")})
);

}  // namespace

TEST_CASE("Parse Unit Type") {
  std::vector<Statement_Params> const params_list = {
      {.name = "unit return",
       .input = k_unit_return_input,
       .expected = k_unit_return_expected,
       .should_succeed = k_unit_return_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
