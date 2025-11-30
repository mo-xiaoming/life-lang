#include "utils.hpp"

using life_lang::ast::make_string;
using life_lang::ast::String;

PARSE_TEST(String, string)

TEST_CASE("Parse String", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<String_Params>({
          {"empty string", R"("")", make_string(R"("")"), true, ""},
          {"simple string", R"("hello")", make_string(R"("hello")"), true, ""},
          {"with escaped quote", R"("abc\"def")", make_string(R"("abc\"def")"), true, ""},
          {"with newline escape", R"("line1\nline2")", make_string(R"("line1\nline2")"), true, ""},
          {"with hex escape", R"("hex\x00value")", make_string(R"("hex\x00value")"), true, ""},
          {"all escapes", R"("abc\"d\n\x00yz")", make_string(R"("abc\"d\n\x00yz")"), true, ""},
          {"with trailing text", R"("hello" world)", make_string(R"("hello")"), true, "world"},
          {"invalid - unclosed", R"("hello)", make_string(""), false, R"("hello)"},
          {"invalid - no quotes", "hello", make_string(""), false, "hello"},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}