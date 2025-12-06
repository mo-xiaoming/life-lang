#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>

#include "internal_rules.hpp"

// Tests to verify that internal parse_* functions produce clang-style diagnostics
// when parsing fails, including file:line:column format and source context.

TEST_CASE("Spirit X3 internal error messages included", "[parser][diagnostics]") {
  SECTION("Invalid function definition produces diagnostic") {
    std::string const input = "fn test(x) {}";  // Missing ': Type' in parameter
    auto begin = input.cbegin();
    auto const end = input.cend();

    auto result = life_lang::internal::parse_function_definition(begin, end);

    REQUIRE_FALSE(result);

    std::ostringstream output;
    result.error().print(output);
    std::string const actual = output.str();

    // Now shows only our clang-style diagnostic (Spirit X3's formatting suppressed)
    std::string const expected =
        "<input>:1:1: error: Failed to parse function definition: Expecting: ':' here:\n"
        "    fn test(x) {}\n"
        "    ^\n";

    CHECK(actual == expected);
  }

  SECTION("Invalid statement produces diagnostic") {
    std::string const input = "return";  // Missing expression and semicolon
    auto begin = input.cbegin();
    auto const end = input.cend();

    auto result = life_lang::internal::parse_statement(begin, end);

    REQUIRE_FALSE(result);

    std::ostringstream output;
    result.error().print(output);
    std::string const actual = output.str();

    // Now shows only our clang-style diagnostic (Spirit X3's formatting suppressed)
    std::string const expected =
        "<input>:1:7: error: Failed to parse statement: Expecting: expression here:\n"
        "    return\n"
        "          ^\n";

    CHECK(actual == expected);
  }

  SECTION("Missing colon in function definition parameter") {
    std::string const input = "fn test(param Type) {}";  // Missing ':'
    auto begin = input.cbegin();
    auto const end = input.cend();

    auto result = life_lang::internal::parse_function_definition(begin, end);

    REQUIRE_FALSE(result);

    std::ostringstream output;
    result.error().print(output);
    std::string const actual = output.str();

    // Spirit X3's message extracted, formatted by our diagnostic engine
    std::string const expected =
        "<input>:1:1: error: Failed to parse function definition: Expecting: ':' here:\n"
        "    fn test(param Type) {}\n"
        "    ^\n";

    CHECK(actual == expected);
  }

  SECTION("Shows line with error and caret") {
    std::string const input = "fn func(x Int) {}";  // Missing ':'
    auto begin = input.cbegin();
    auto const end = input.cend();

    auto result = life_lang::internal::parse_function_definition(begin, end);

    REQUIRE_FALSE(result);

    std::ostringstream output;
    result.error().print(output);
    std::string const actual = output.str();

    // Shows source line with caret (Spirit X3's formatting suppressed)
    std::string const expected =
        "<input>:1:1: error: Failed to parse function definition: Expecting: ':' here:\n"
        "    fn func(x Int) {}\n"
        "    ^\n";

    CHECK(actual == expected);
  }
}

TEST_CASE("Example clang-style error output", "[parser][diagnostics][.]") {
  // This test demonstrates the clang-style error format
  // Note: Tagged with [.] to hide from default test runs
  SECTION("Show example error message") {
    std::string const input = "fn main(): I32";  // Missing body
    auto begin = input.cbegin();
    auto const end = input.cend();

    auto result = life_lang::internal::parse_function_definition(begin, end);

    REQUIRE_FALSE(result);

    // Print the diagnostic to demonstrate format
    std::cout << "\n=== Example Clang-Style Error Output ===\n";
    result.error().print(std::cout);
    std::cout << "=========================================\n\n";
  }
}
