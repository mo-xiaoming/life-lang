#include "internal_rules.hpp"
#include "utils.hpp"
using life_lang::ast::Statement;

PARSE_TEST(Statement, statement)

namespace {
// === Unit Type Tests ===

// Function with unit return type
constexpr auto k_unit_return_should_succeed = true;
constexpr auto k_unit_return_input = "fn test(): () { return (); }";
inline auto const k_unit_return_expected = R"({
  "Func_Def": {
    "decl": {
      "Func_Decl": {
        "name": "test",
        "type_params": [],
        "params": [],
        "return_type": {
          "Tuple_Type": {
            "element_types": []
          }
        }
      }
    },
    "body": {
      "Block": {
        "statements": [
          {
            "Return_Statement": {
              "expr": {
                "Unit_Literal": {}
              }
            }
          }
        ]
      }
    }
  }
})";
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
