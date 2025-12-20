#include "internal_rules.hpp"

#include <doctest/doctest.h>

namespace {

auto parse_func(std::string const& input_) {
  return life_lang::internal::parse_func_def(input_);
}

auto parse_struct(std::string const& input_) {
  return life_lang::internal::parse_struct_def(input_);
}

auto parse_enum(std::string const& input_) {
  return life_lang::internal::parse_enum_def(input_);
}

auto parse_impl(std::string const& input_) {
  return life_lang::internal::parse_impl_block(input_);
}

auto parse_trait(std::string const& input_) {
  return life_lang::internal::parse_trait_def(input_);
}

auto parse_trait_impl(std::string const& input_) {
  return life_lang::internal::parse_trait_impl(input_);
}

}  // namespace

// ============================================================================
// Function Declarations
// ============================================================================

TEST_CASE("Where clauses in function declarations") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "simple where clause",
       .input = "fn process<T>(items: Vec<T>): Result where T: Display { return Result {}; }"},
      {.name = "multiple bounds on one type",
       .input = "fn compare<T>(a: T, b: T): Bool where T: Eq + Ord { return true; }"},
      {.name = "multiple predicates",
       .input = "fn transform<T, U>(input: T): U where T: Display, U: Clone { return input; }"},
      {.name = "inline bounds and where clause",
       .input = "fn process<T: Display, U>(a: T, b: U): Unit where U: Clone + Eq { return Unit {}; }"},
      {.name = "no where clause", .input = "fn process<T: Display>(item: T): Unit { return Unit {}; }"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_func(test.input);
      CHECK(result);
    }
  }
}

// ============================================================================
// Struct Definitions
// ============================================================================

TEST_CASE("Where clauses in struct definitions") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "simple where clause", .input = "struct Container<T> where T: Clone { value: T }"},
      {.name = "multiple predicates", .input = "struct Pair<T, U> where T: Display, U: Clone { first: T, second: U }"},
      {.name = "empty struct with where", .input = "struct Marker<T> where T: Send {}"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_struct(test.input);
      CHECK(result);
    }
  }
}

// ============================================================================
// Enum Definitions
// ============================================================================

TEST_CASE("Where clauses in enum definitions") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "simple where clause", .input = "enum Option<T> where T: Clone { Some(T), None }"},
      {.name = "multiple bounds", .input = "enum Result<T, E> where T: Display + Clone, E: Debug { Ok(T), Err(E) }"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_enum(test.input);
      CHECK(result);
    }
  }
}

// ============================================================================
// Impl Blocks
// ============================================================================

TEST_CASE("Where clauses in impl blocks") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "simple where clause",
       .input = "impl<T> Container<T> where T: Clone { fn new(): Container<T> { return Container {}; } }"},
      {.name = "multiple predicates",
       .input = "impl<T, U> Pair<T, U> where T: Display, U: Clone { fn first(self): T { return self.first; } }"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_impl(test.input);
      CHECK(result);
    }
  }
}

// ============================================================================
// Trait Definitions
// ============================================================================

TEST_CASE("Where clauses in trait definitions") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "simple where clause", .input = "trait Processor<T> where T: Clone { fn process(item: T): Result; }"},
      {.name = "complex where clause",
       .input = "trait Converter<T, U> where T: Display + Clone, U: Debug { fn convert(input: T): U; }"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_trait(test.input);
      CHECK(result);
    }
  }
}

// ============================================================================
// Trait Implementations
// ============================================================================

TEST_CASE("Where clauses in trait implementations") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "simple where clause",
       .input = "impl<T> Display for Container<T> where T: Display { fn fmt(self): String { return \"\"; } }"},
      {.name = "multiple predicates",
       .input = "impl<T, U> Convert<U> for Wrapper<T> where T: Display, U: Clone { fn convert(self): U { return "
                "self.value; } }"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_trait_impl(test.input);
      CHECK(result);
    }
  }
}
