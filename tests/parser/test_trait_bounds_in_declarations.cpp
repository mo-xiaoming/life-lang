#include <doctest/doctest.h>
#include "internal_rules.hpp"

namespace {

auto parse_struct(std::string const& input_) {
  return life_lang::internal::parse_struct_def(input_);
}

auto parse_enum(std::string const& input_) {
  return life_lang::internal::parse_enum_def(input_);
}

auto parse_trait(std::string const& input_) {
  return life_lang::internal::parse_trait_def(input_);
}

auto parse_impl(std::string const& input_) {
  return life_lang::internal::parse_impl_block(input_);
}

}  // namespace

TEST_CASE("Trait bounds in struct definitions") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "struct with single bound", .input = "struct Box<T: Display> { value: T }"},
      {.name = "struct with multiple bounds", .input = "struct Container<T: Display + Clone> { value: T }"},
      {.name = "struct with multiple params with bounds",
       .input = "struct Pair<T: Display, U: Clone> { first: T, second: U }"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_struct(test.input);
      CHECK(result);
    }
  }
}

TEST_CASE("Trait bounds in enum definitions") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "enum with single bound", .input = "enum Option<T: Clone> { Some(T), None }"},
      {.name = "enum with multiple bounds", .input = "enum Result<T: Display + Clone, E: Display> { Ok(T), Err(E) }"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_enum(test.input);
      CHECK(result);
    }
  }
}

TEST_CASE("Trait bounds in trait definitions") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "trait with bounded type parameter",
       .input = "trait Iterator<T: Clone> { fn next(mut self): Option<T>; }"},
      {.name = "trait with multiple bounds",
       .input = "trait Comparable<T: Eq + Ord> { fn compare(self, other: T): Ordering; }"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_trait(test.input);
      CHECK(result);
    }
  }
}

TEST_CASE("Trait bounds in impl blocks") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "impl with single bound", .input = "impl<T: Display> Container<T> { fn show(self): Unit { } }"},
      {.name = "impl with multiple bounds",
       .input = "impl<T: Display + Clone + Eq> Array<T> { fn process(self): Unit { } }"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_impl(test.input);
      CHECK(result);
    }
  }
}
