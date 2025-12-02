#include "utils.hpp"

// Note: Module tests use the PUBLIC API (parser::parse_module), not internal parsers
// This is because modules must consume ALL input - partial parses should fail

TEST_CASE("Parse Module - Complete Input Validation", "[parser]") {
  struct Test_Case {
    std::string name;
    std::string input;
    bool should_succeed;
  };

  auto const test = GENERATE(
      Catch::Generators::values<Test_Case>({
          // Valid cases - input fully consumed
          {"empty module", "", true},
          {"single function", "fn main(): I32 { return 0; }", true},
          {"whitespace only", "   \n\t  ", true},

          // Invalid cases - input not fully consumed (partial parse)
          {"incomplete function", "fn bad syntax", false},
          {"starts with number", "123 invalid", false},
          {"incomplete declaration", "fn foo(", false},
          {"extra text after valid function", "fn main(): I32 { return 0; } garbage", false},
      })
  );

  DYNAMIC_SECTION(test.name) {
    auto const result = life_lang::parser::parse_module(test.input, "test.life");

    CHECK(test.should_succeed == bool(result));

    if (!test.should_succeed && !result) {
      // Verify we got diagnostics with errors
      CHECK(result.error().has_errors());
    }
  }
}
