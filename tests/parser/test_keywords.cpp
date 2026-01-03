#include "internal_rules.hpp"
#include "utils.hpp"

TEST_CASE("Keyword Boundary Validation") {
  struct Test_Case {
    std::string name;
    std::string input;
    bool should_succeed;
    bool is_return_stmt;  // true = Return_Statement, false = Func_Def
  };

  std::vector<Test_Case> const test_list = {
      // Valid function declarations - keywords followed by whitespace
      {.name = "fn with space", .input = "fn hello() : Int {}", .should_succeed = true, .is_return_stmt = false},
      {.name = "fn with newline", .input = "fn\nhello() : Int {}", .should_succeed = true, .is_return_stmt = false},
      {.name = "fn with tab", .input = "fn\thello() : Int {}", .should_succeed = true, .is_return_stmt = false},

      // Invalid - keywords followed by variable_name characters
      {.name = "fn with underscore", .input = "fn_hello() : Int {}", .should_succeed = false, .is_return_stmt = false},
      {.name = "fn with digit", .input = "fn2() : Int {}", .should_succeed = false, .is_return_stmt = false},
      {.name = "fn with letter", .input = "fnord() : Int {}", .should_succeed = false, .is_return_stmt = false},

      // Return statement tests
      {.name = "return with space", .input = "return 42;", .should_succeed = true, .is_return_stmt = true},
      {.name = "return with newline", .input = "return\n42;", .should_succeed = true, .is_return_stmt = true},
      {.name = "return invalid continuation", .input = "returnx 42;", .should_succeed = false, .is_return_stmt = true},
      {.name = "return with underscore", .input = "return_value;", .should_succeed = true, .is_return_stmt = true},
  };

  for (auto const& test: test_list) {
    SUBCASE(std::string(test.name).c_str()) {
      if (test.is_return_stmt) {
        auto const got = life_lang::internal::parse_statement(test.input);
        CHECK(test.should_succeed == bool(got));
      } else {
        auto const got = life_lang::internal::parse_func_def(test.input);
        CHECK(test.should_succeed == bool(got));
      }
    }
  }
}

// Test that variable_names cannot be keywords
// Note: This test has custom validation logic (checking segment name),
// so it doesn't use PARSE_TEST which compares full AST structure.
TEST_CASE("Var_Name vs Keyword Distinction") {
  struct Test_Case {
    std::string name;
    std::string input;
    std::string expected_name;
  };

  std::vector<Test_Case> const test_list = {
      {.name = "snake_case variable_name", .input = "hello_world", .expected_name = "hello_world"},
      {.name = "variable_name starting with 'f'", .input = "function", .expected_name = "function"},
      {.name = "variable_name starting with 'r'", .input = "ret", .expected_name = "ret"},
      {.name = "variable_name with 'fn' inside", .input = "confn", .expected_name = "confn"},
      {.name = "variable_name ending with 'fn'", .input = "defn", .expected_name = "defn"},
      // These should parse as variable_names, not keywords
      {.name = "fn plus text", .input = "fnord", .expected_name = "fnord"},
      {.name = "return plus text", .input = "returnvalue", .expected_name = "returnvalue"},
      {.name = "let plus text", .input = "letter", .expected_name = "letter"},
  };
  for (auto const& test: test_list) {
    SUBCASE(std::string(test.name).c_str()) {
      auto const got = life_lang::internal::parse_type_name(test.input);
      REQUIRE(bool(got));
      auto const& segments = std::get<life_lang::ast::Path_Type>(*got).segments;
      CHECK(segments.size() == 1);
      CHECK(segments[0].value == test.expected_name);
    }
  }
}
