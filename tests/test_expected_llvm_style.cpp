// Tests for LLVM-style Expected error handling
// Demonstrates mandatory error checking behavior

#include <string>

#include <doctest/doctest.h>

#include "expected.hpp"

using life_lang::Expected;
using life_lang::Unexpected;

namespace {
// Helper to create success/error results
Expected<int, std::string> divide(int a_, int b_) {
  if (b_ == 0) {
    return Unexpected{std::string("Division by zero")};
  }
  return a_ / b_;
}
}  // namespace

TEST_CASE("Expected - LLVM-style mandatory checking") {
  SUBCASE("Success path - check before use") {
    auto result = divide(10, 2);
    REQUIRE(result);  // Must check first
    CHECK(*result == 5);
  }

  SUBCASE("Error path - check before accessing error") {
    auto result = divide(10, 0);
    REQUIRE(!result);  // Must check first
    CHECK(result.error() == "Division by zero");
  }

  SUBCASE("Move-only semantics - cannot copy") {
    auto result = divide(10, 2);
    // auto copy = result;  // Would not compile - deleted copy constructor
    auto moved = std::move(result);
    REQUIRE(moved);
    CHECK(*moved == 5);
    // result is now in moved-from state (checked automatically during move)
  }

  SUBCASE("Error propagation - take_error()") {
    auto result = divide(10, 0);
    if (!result) {
      auto err = std::move(result).take_error();  // Must move to take error
      CHECK(err == "Division by zero");
    }
  }

  SUBCASE("Explicit error consumption") {
    auto result = divide(10, 0);
    result.consume_error();  // Explicitly discard without handling
    // No assertion on destruction
  }

  SUBCASE("has_value() marks as checked") {
    auto result = divide(10, 2);
    CHECK(result.has_value() == true);
    // Now safe to access
    CHECK(*result == 5);
  }

  SUBCASE("Chaining with early return pattern") {
    auto check_division = [](int a_, int b_) -> Expected<int, std::string> {
      auto result = divide(a_, b_);
      if (!result) {
        return Unexpected{std::move(result).take_error()};
      }
      return *result * 2;  // Use the value
    };

    auto success = check_division(10, 2);
    REQUIRE(success);
    CHECK(*success == 10);

    auto failure = check_division(10, 0);
    REQUIRE(!failure);
    CHECK(failure.error() == "Division by zero");
  }
}

TEST_CASE("Expected - operator-> and operator*") {
  struct Point {
    int x;
    int y;
    int distance() const { return x + y; }
  };

  auto make_point = [](int x_, int y_) -> Expected<Point, std::string> {
    if (x_ < 0 || y_ < 0) {
      return Unexpected{std::string("Negative coordinates")};
    }
    return Point{.x = x_, .y = y_};
  };

  SUBCASE("operator-> on success") {
    auto result = make_point(3, 4);
    REQUIRE(result);
    CHECK(result->x == 3);
    CHECK(result->y == 4);
    CHECK(result->distance() == 7);
  }

  SUBCASE("operator* for value access") {
    auto result = make_point(5, 6);
    REQUIRE(result);
    Point p = *result;
    CHECK(p.x == 5);
    CHECK(p.y == 6);
  }
}

// Note: The following would trigger assertions in debug builds:
//
// TEST_CASE("Expected - unchecked error assertion") {
//   // This would assert on destruction:
//   auto result = divide(10, 0);
//   // Forgot to check! Assertion: "Expected error must be explicitly checked or consumed"
// }
//
// TEST_CASE("Expected - accessing value without checking") {
//   auto result = divide(10, 2);
//   // *result without checking first would assert:
//   // Assertion: "Must check Expected before dereferencing"
// }
