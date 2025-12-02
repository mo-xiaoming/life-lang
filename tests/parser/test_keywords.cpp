#include "utils.hpp"

// Specialized test for keyword boundary validation
// Note: This test validates lexical analysis (keyword boundaries), not AST structure,
// so it doesn't follow the standard PARSE_TEST pattern which compares expected AST.
TEST_CASE("Keyword Boundary Validation", "[parser][keywords]") {
  struct Test_Case {
    std::string name;
    std::string input;
    bool should_succeed;
    std::string rest;
    bool is_return_stmt;  // true = Return_Statement, false = Function_Definition
  };

  auto const test = GENERATE(
      Catch::Generators::values<Test_Case>({
          // Valid function declarations - keywords followed by whitespace
          {"fn with space", "fn hello() : Int {}", true, "", false},
          {"fn with newline", "fn\nhello() : Int {}", true, "", false},
          {"fn with tab", "fn\thello() : Int {}", true, "", false},

          // Invalid - keywords followed by identifier characters
          {"fn with underscore", "fn_hello() : Int {}", false, "fn_hello() : Int {}", false},
          {"fn with digit", "fn2() : Int {}", false, "fn2() : Int {}", false},
          {"fn with letter", "fnord() : Int {}", false, "fnord() : Int {}", false},

          // Return statement tests
          {"return with space", "return 42;", true, "", true},
          {"return with newline", "return\n42;", true, "", true},
          {"return invalid continuation", "returnx 42;", false, "returnx 42;", true},
          {"return with underscore", "return_value;", false, "return_value;", true},
      })
  );

  DYNAMIC_SECTION(test.name) {
    auto begin = test.input.cbegin();
    auto const end = test.input.cend();

    if (test.is_return_stmt) {
      auto const got = life_lang::internal::parse_return_statement(begin, end);
      CHECK(test.should_succeed == bool(got));
      CHECK(test.rest == std::string{begin, end});
    } else {
      auto const got = life_lang::internal::parse_function_definition(begin, end);
      CHECK(test.should_succeed == bool(got));
      CHECK(test.rest == std::string{begin, end});
    }
  }
}

// Test that identifiers cannot be keywords
// Note: This test has custom validation logic (checking segment name),
// so it doesn't use PARSE_TEST which compares full AST structure.
TEST_CASE("Identifier vs Keyword Distinction", "[parser][keywords]") {
  struct Test_Case {
    std::string name;
    std::string input;
    std::string expected_name;
  };

  auto const test = GENERATE(
      Catch::Generators::values<Test_Case>({
          {"snake_case identifier", "hello_world", "hello_world"},
          {"identifier starting with 'f'", "function", "function"},
          {"identifier starting with 'r'", "ret", "ret"},
          {"identifier with 'fn' inside", "confn", "confn"},
          {"identifier ending with 'fn'", "defn", "defn"},
          // These should parse as identifiers, not keywords
          {"fn plus text", "fnord", "fnord"},
          {"return plus text", "returnvalue", "returnvalue"},
          {"let plus text", "letter", "letter"},
      })
  );

  DYNAMIC_SECTION(test.name) {
    auto begin = test.input.cbegin();
    auto const end = test.input.cend();

    auto const got = life_lang::internal::parse_path(begin, end);
    REQUIRE(bool(got));
    CHECK((*got).segments.size() == 1);
    CHECK((*got).segments[0].value == test.expected_name);
  }
}
