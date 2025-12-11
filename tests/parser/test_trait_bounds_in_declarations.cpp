//! Test trait bounds in struct, enum, trait, and impl declarations
//! Verifies that bounds work consistently across all generic declarations

#include "internal_rules.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

auto parse_struct(std::string const& a_input) {
  auto begin = a_input.cbegin();
  return life_lang::internal::parse_struct_def(begin, a_input.cend());
}

auto parse_enum(std::string const& a_input) {
  auto begin = a_input.cbegin();
  return life_lang::internal::parse_enum_def(begin, a_input.cend());
}

auto parse_trait(std::string const& a_input) {
  auto begin = a_input.cbegin();
  return life_lang::internal::parse_trait_def(begin, a_input.cend());
}

auto parse_impl(std::string const& a_input) {
  auto begin = a_input.cbegin();
  return life_lang::internal::parse_impl_block(begin, a_input.cend());
}

}  // namespace

TEST_CASE("Trait bounds in struct definitions", "[parser][trait_bounds]") {
  SECTION("struct with single bound") {
    auto const* input = "struct Box<T: Display> { value: T }";
    auto const result = parse_struct(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 1);
    CHECK(result->type_params[0].bounds[0].trait_name.segments[0].value == "Display");
  }

  SECTION("struct with multiple bounds") {
    auto const* input = "struct Container<T: Display + Clone> { value: T }";
    auto const result = parse_struct(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 2);
    CHECK(result->type_params[0].bounds[0].trait_name.segments[0].value == "Display");
    CHECK(result->type_params[0].bounds[1].trait_name.segments[0].value == "Clone");
  }

  SECTION("struct with multiple params with bounds") {
    auto const* input = "struct Pair<T: Display, U: Clone> { first: T, second: U }";
    auto const result = parse_struct(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 2);
    CHECK(result->type_params[0].bounds.size() == 1);
    CHECK(result->type_params[1].bounds.size() == 1);
  }
}

TEST_CASE("Trait bounds in enum definitions", "[parser][trait_bounds]") {
  SECTION("enum with single bound") {
    auto const* input = "enum Option<T: Clone> { Some(T), None }";
    auto const result = parse_enum(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 1);
    CHECK(result->type_params[0].bounds[0].trait_name.segments[0].value == "Clone");
  }

  SECTION("enum with multiple bounds") {
    auto const* input = "enum Result<T: Display + Clone, E: Display> { Ok(T), Err(E) }";
    auto const result = parse_enum(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 2);
    CHECK(result->type_params[0].bounds.size() == 2);
    CHECK(result->type_params[1].bounds.size() == 1);
  }
}

TEST_CASE("Trait bounds in trait definitions", "[parser][trait_bounds]") {
  SECTION("trait with bounded type parameter") {
    auto const* input = "trait Iterator<T: Clone> { fn next(mut self): Option<T>; }";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 1);
    CHECK(result->type_params[0].bounds[0].trait_name.segments[0].value == "Clone");
  }

  SECTION("trait with multiple bounds") {
    auto const* input = "trait Comparable<T: Eq + Ord> { fn compare(self, other: T): Ordering; }";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 2);
  }
}

TEST_CASE("Trait bounds in impl blocks", "[parser][trait_bounds]") {
  SECTION("impl with single bound") {
    auto const* input = "impl<T: Display> Container<T> { fn show(self): Unit { } }";
    auto const result = parse_impl(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 1);
    CHECK(result->type_params[0].bounds[0].trait_name.segments[0].value == "Display");
  }

  SECTION("impl with multiple bounds") {
    auto const* input = "impl<T: Display + Clone + Eq> Array<T> { fn process(self): Unit { } }";
    auto const result = parse_impl(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 3);
    CHECK(result->type_params[0].bounds[0].trait_name.segments[0].value == "Display");
    CHECK(result->type_params[0].bounds[1].trait_name.segments[0].value == "Clone");
    CHECK(result->type_params[0].bounds[2].trait_name.segments[0].value == "Eq");
  }
}
