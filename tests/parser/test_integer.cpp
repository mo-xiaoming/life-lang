#include "utils.hpp"

using life_lang::ast::Integer;
using life_lang::ast::make_integer;

PARSE_TEST(Integer, integer)

TEST_CASE("Parse Integer", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Integer_Params>({
          {"zero", "0", make_integer("0"), true, ""},
          {"simple number", "123", make_integer("123"), true, ""},
          {"large number", "987654321", make_integer("987654321"), true, ""},
          {"with underscores", "12_34_5", make_integer("12345"), true, ""},
          {"multiple underscores", "1_2_3_4", make_integer("1234"), true, ""},
          {"with trailing text", "42 abc", make_integer("42"), true, "abc"},
          // Invalid cases
          {"invalid - starts with zero", "0123", make_integer(""), false, "0123"},
          {"invalid - starts with underscore", "_12", make_integer(""), false, "_12"},
          {"invalid - ends with underscore", "12_", make_integer(""), false, "12_"},
          {"invalid - zero with underscore", "0_", make_integer(""), false, "0_"},
          {"invalid - underscore before zero", "_0", make_integer(""), false, "_0"},
          {"invalid - empty", "", make_integer(""), false, ""},
          {"invalid - letter", "abc", make_integer(""), false, "abc"},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}