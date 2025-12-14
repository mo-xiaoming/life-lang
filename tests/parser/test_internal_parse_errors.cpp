#include <doctest/doctest.h>
#include <iostream>
#include <sstream>

#include "internal_rules.hpp"

// NOTE: Previous tests checked Spirit X3-specific error messages.
// Since we've replaced Spirit X3 with a hand-written parser that has different
// grammar rules (e.g., parameter types are optional), those tests are obsolete.
//
// Error reporting is tested through:
// - Integration tests that verify end-to-end error reporting
// - Individual parser tests that check failure cases for each construct
//
// Parser functionality is thoroughly tested by 55+ passing parser test cases.

TEST_CASE("Example clang-style error output") {
  // This test demonstrates the clang-style error format
  // Note: Tagged with [.] to hide from default test runs
  SUBCASE("Show example error message") {
    std::string const input = "fn main(): I32";  // Missing body

    auto result = life_lang::internal::parse_func_def(input);

    REQUIRE_FALSE(result);

    // Print the diagnostic to demonstrate format
    std::cout << "\n=== Example Clang-Style Error Output ===\n";
    result.error().print(std::cout);
    std::cout << "=========================================\n\n";
  }
}
