#include "utils.hpp"

using life_lang::ast::make_block;
using life_lang::ast::make_function_declaration;
using life_lang::ast::make_function_definition;
using life_lang::ast::make_module;
using life_lang::ast::make_path;
using life_lang::ast::make_return_statement;
using life_lang::ast::make_statement;
using life_lang::ast::Module;

// Note: Module tests use the PUBLIC API (parser::parse), not internal parsers
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
    auto begin = test.input.cbegin();
    auto const end = test.input.cend();
    std::ostringstream error_stream;

    auto const result = life_lang::parser::parse(begin, end, error_stream);

    CHECK(test.should_succeed == bool(result));

    if (test.should_succeed && result) {
      // Verify all input was consumed
      CHECK(begin == end);
    } else if (!test.should_succeed && !result) {
      // Verify we got an error message
      CHECK(!result.error().empty());
    }
  }
}
