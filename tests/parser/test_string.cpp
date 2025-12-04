#include "utils.hpp"

using life_lang::ast::String;

PARSE_TEST(String, string)

TEST_CASE("Parse String", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<String_Params>({
          {"empty string", R"("")", R"({"String": {"value": "\"\""}})", true, ""},
          {"simple string", R"("hello")", R"({"String": {"value": "\"hello\""}})", true, ""},
          {"with escaped quote", R"("abc\"def")", R"({"String": {"value": "\"abc\\\"def\""}})", true, ""},
          {"with newline escape", R"("line1\nline2")", R"({"String": {"value": "\"line1\\nline2\""}})", true, ""},
          {"with hex escape", R"("hex\x00value")", R"({"String": {"value": "\"hex\\x00value\""}})", true, ""},
          {"all escapes", R"("abc\"d\n\x00yz")", R"({"String": {"value": "\"abc\\\"d\\n\\x00yz\""}})", true, ""},
          {"with trailing text", R"("hello" world)", R"({"String": {"value": "\"hello\""}})", true, "world"},
          {"invalid - unclosed", R"("hello)", "{}", false, R"("hello)"},
          {"invalid - no quotes", "hello", "{}", false, "hello"},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}