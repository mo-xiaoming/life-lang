// Comprehensive where clause parsing tests across all declaration types
// Uses success-only validation since JSON comparison is complex

#include "internal_rules.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

auto parse_func(std::string const& a_input) {
  auto begin = a_input.cbegin();
  return life_lang::internal::parse_func_def(begin, a_input.cend());
}

auto parse_struct(std::string const& a_input) {
  auto begin = a_input.cbegin();
  return life_lang::internal::parse_struct_def(begin, a_input.cend());
}

auto parse_enum(std::string const& a_input) {
  auto begin = a_input.cbegin();
  return life_lang::internal::parse_enum_def(begin, a_input.cend());
}

auto parse_impl(std::string const& a_input) {
  auto begin = a_input.cbegin();
  return life_lang::internal::parse_impl_block(begin, a_input.cend());
}

auto parse_trait(std::string const& a_input) {
  auto begin = a_input.cbegin();
  return life_lang::internal::parse_trait_def(begin, a_input.cend());
}

auto parse_trait_impl(std::string const& a_input) {
  auto begin = a_input.cbegin();
  return life_lang::internal::parse_trait_impl(begin, a_input.cend());
}

}  // namespace

// ============================================================================
// Function Declarations
// ============================================================================

TEST_CASE("Where clauses in function declarations", "[parser][where_clause]") {
  SECTION("simple where clause") {
    auto const* input = "fn process<T>(items: Vec<T>): Result where T: Display { return Result {}; }";
    auto const result = parse_func(input);
    REQUIRE(result);
    REQUIRE(result->declaration.where_clause.has_value());
    auto const& where = *result->declaration.where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 1);
    REQUIRE(where.predicates[0].bounds.size() == 1);
  }

  SECTION("multiple bounds on one type") {
    auto const* input = "fn compare<T>(a: T, b: T): Bool where T: Eq + Ord { return true; }";
    auto const result = parse_func(input);
    REQUIRE(result);
    REQUIRE(result->declaration.where_clause.has_value());
    auto const& where = *result->declaration.where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 1);
    REQUIRE(where.predicates[0].bounds.size() == 2);
  }

  SECTION("multiple predicates") {
    auto const* input = "fn transform<T, U>(input: T): U where T: Display, U: Clone { return input; }";
    auto const result = parse_func(input);
    REQUIRE(result);
    REQUIRE(result->declaration.where_clause.has_value());
    auto const& where = *result->declaration.where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 2);
  }

  SECTION("inline bounds and where clause") {
    auto const* input = "fn process<T: Display, U>(a: T, b: U): Unit where U: Clone + Eq { return Unit {}; }";
    auto const result = parse_func(input);
    REQUIRE(result);
    CHECK(result->declaration.type_params[0].bounds.size() == 1);  // inline bound on T
    REQUIRE(result->declaration.where_clause.has_value());
    auto const& where = *result->declaration.where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 1);                  // where clause for U
    REQUIRE(where.predicates[0].bounds.size() == 2);        // Clone + Eq
  }

  SECTION("no where clause - regression test") {
    auto const* input = "fn process<T: Display>(item: T): Unit { return Unit {}; }";
    auto const result = parse_func(input);
    REQUIRE(result);
    CHECK_FALSE(result->declaration.where_clause.has_value());
  }
}

// ============================================================================
// Struct Definitions
// ============================================================================

TEST_CASE("Where clauses in struct definitions", "[parser][where_clause]") {
  SECTION("simple where clause") {
    auto const* input = "struct Container<T> where T: Clone { value: T }";
    auto const result = parse_struct(input);
    REQUIRE(result);
    REQUIRE(result->where_clause.has_value());
    auto const& where = *result->where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 1);
  }

  SECTION("multiple predicates") {
    auto const* input = "struct Pair<T, U> where T: Display, U: Clone { first: T, second: U }";
    auto const result = parse_struct(input);
    REQUIRE(result);
    REQUIRE(result->where_clause.has_value());
    auto const& where = *result->where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 2);
  }

  SECTION("empty struct with where") {
    auto const* input = "struct Marker<T> where T: Send {}";
    auto const result = parse_struct(input);
    REQUIRE(result);
    CHECK(result->where_clause.has_value());
  }
}

// ============================================================================
// Enum Definitions
// ============================================================================

TEST_CASE("Where clauses in enum definitions", "[parser][where_clause]") {
  SECTION("simple where clause") {
    auto const* input = "enum Option<T> where T: Clone { Some(T), None }";
    auto const result = parse_enum(input);
    REQUIRE(result);
    REQUIRE(result->where_clause.has_value());
    auto const& where = *result->where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 1);
  }

  SECTION("multiple bounds") {
    auto const* input = "enum Result<T, E> where T: Display + Clone, E: Debug { Ok(T), Err(E) }";
    auto const result = parse_enum(input);
    REQUIRE(result);
    REQUIRE(result->where_clause.has_value());
    auto const& where = *result->where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 2);
    REQUIRE(where.predicates[0].bounds.size() == 2);  // Display + Clone
  }
}

// ============================================================================
// Impl Blocks
// ============================================================================

TEST_CASE("Where clauses in impl blocks", "[parser][where_clause]") {
  SECTION("simple where clause") {
    auto const* input = "impl<T> Container<T> where T: Clone { fn new(): Container<T> { return Container {}; } }";
    auto const result = parse_impl(input);
    REQUIRE(result);
    REQUIRE(result->where_clause.has_value());
    auto const& where = *result->where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 1);
  }

  SECTION("multiple predicates") {
    auto const* input = "impl<T, U> Pair<T, U> where T: Display, U: Clone { fn first(self): T { return self.first; } }";
    auto const result = parse_impl(input);
    REQUIRE(result);
    REQUIRE(result->where_clause.has_value());
    auto const& where = *result->where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 2);
  }
}

// ============================================================================
// Trait Definitions
// ============================================================================

TEST_CASE("Where clauses in trait definitions", "[parser][where_clause]") {
  SECTION("simple where clause") {
    auto const* input = "trait Processor<T> where T: Clone { fn process(item: T): Result; }";
    auto const result = parse_trait(input);
    REQUIRE(result);
    REQUIRE(result->where_clause.has_value());
    auto const& where = *result->where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 1);
  }

  SECTION("complex where clause") {
    auto const* input = "trait Converter<T, U> where T: Display + Clone, U: Debug { fn convert(input: T): U; }";
    auto const result = parse_trait(input);
    REQUIRE(result);
    REQUIRE(result->where_clause.has_value());
    auto const& where = *result->where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 2);
    REQUIRE(where.predicates[0].bounds.size() == 2);
  }
}

// ============================================================================
// Trait Implementations
// ============================================================================

TEST_CASE("Where clauses in trait implementations", "[parser][where_clause]") {
  SECTION("simple where clause") {
    auto const* input = "impl<T> Display for Container<T> where T: Display { fn fmt(self): String { return \"\"; } }";
    auto const result = parse_trait_impl(input);
    REQUIRE(result);
    REQUIRE(result->where_clause.has_value());
    auto const& where = *result->where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 1);
  }

  SECTION("multiple predicates") {
    auto const* input =
        "impl<T, U> Convert<U> for Wrapper<T> where T: Display, U: Clone { fn convert(self): U { return "
        "self.value; } }";
    auto const result = parse_trait_impl(input);
    REQUIRE(result);
    REQUIRE(result->where_clause.has_value());
    auto const& where = *result->where_clause;  // NOLINT(bugprone-unchecked-optional-access)
    REQUIRE(where.predicates.size() == 2);
  }
}
