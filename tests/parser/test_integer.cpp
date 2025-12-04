#include "utils.hpp"

using life_lang::ast::Integer;

PARSE_TEST(Integer, integer)

TEST_CASE("Parse Integer", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Integer_Params>({
          {"zero", "0", R"({"Integer": {"value": "0"}})", true, ""},
          {"simple number", "123", R"({"Integer": {"value": "123"}})", true, ""},
          {"large number", "987654321", R"({"Integer": {"value": "987654321"}})", true, ""},
          {"with underscores", "12_34_5", R"({"Integer": {"value": "12345"}})", true, ""},
          {"multiple underscores", "1_2_3_4", R"({"Integer": {"value": "1234"}})", true, ""},
          {"with trailing text", "42 abc", R"({"Integer": {"value": "42"}})", true, "abc"},
          // Invalid cases
          {"invalid - starts with zero", "0123", "{}", false, "0123"},
          {"invalid - starts with underscore", "_12", "{}", false, "_12"},
          {"invalid - ends with underscore", "12_", "{}", false, "12_"},
          {"invalid - zero with underscore", "0_", "{}", false, "0_"},
          {"invalid - underscore before zero", "_0", "{}", false, "_0"},
          {"invalid - empty", "", "{}", false, ""},
          {"invalid - letter", "abc", "{}", false, "abc"},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}