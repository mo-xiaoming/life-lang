//! Test trait bounds in struct, enum, trait, and impl declarations
//! Verifies that bounds work consistently across all generic declarations

#include "internal_rules.hpp"

#include <doctest/doctest.h>

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
  SUBCASE("struct with single bound") {
    auto const* input = "struct Box<T: Display> { value: T }";
    auto const result = parse_struct(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 1);
    CHECK(result->type_params[0].bounds[0].trait_name.segments()[0].value == "Display");
  }

  SUBCASE("struct with multiple bounds") {
    auto const* input = "struct Container<T: Display + Clone> { value: T }";
    auto const result = parse_struct(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 2);
    CHECK(result->type_params[0].bounds[0].trait_name.segments()[0].value == "Display");
    CHECK(result->type_params[0].bounds[1].trait_name.segments()[0].value == "Clone");
  }

  SUBCASE("struct with multiple params with bounds") {
    auto const* input = "struct Pair<T: Display, U: Clone> { first: T, second: U }";
    auto const result = parse_struct(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 2);
    CHECK(result->type_params[0].bounds.size() == 1);
    CHECK(result->type_params[1].bounds.size() == 1);
  }
}

TEST_CASE("Trait bounds in enum definitions") {
  SUBCASE("enum with single bound") {
    auto const* input = "enum Option<T: Clone> { Some(T), None }";
    auto const result = parse_enum(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 1);
    CHECK(result->type_params[0].bounds[0].trait_name.segments()[0].value == "Clone");
  }

  SUBCASE("enum with multiple bounds") {
    auto const* input = "enum Result<T: Display + Clone, E: Display> { Ok(T), Err(E) }";
    auto const result = parse_enum(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 2);
    CHECK(result->type_params[0].bounds.size() == 2);
    CHECK(result->type_params[1].bounds.size() == 1);
  }
}

TEST_CASE("Trait bounds in trait definitions") {
  SUBCASE("trait with bounded type parameter") {
    auto const* input = "trait Iterator<T: Clone> { fn next(mut self): Option<T>; }";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 1);
    CHECK(result->type_params[0].bounds[0].trait_name.segments()[0].value == "Clone");
  }

  SUBCASE("trait with multiple bounds") {
    auto const* input = "trait Comparable<T: Eq + Ord> { fn compare(self, other: T): Ordering; }";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 2);
  }
}

TEST_CASE("Trait bounds in impl blocks") {
  SUBCASE("impl with single bound") {
    auto const* input = "impl<T: Display> Container<T> { fn show(self): Unit { } }";
    auto const result = parse_impl(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 1);
    CHECK(result->type_params[0].bounds[0].trait_name.segments()[0].value == "Display");
  }

  SUBCASE("impl with multiple bounds") {
    auto const* input = "impl<T: Display + Clone + Eq> Array<T> { fn process(self): Unit { } }";
    auto const result = parse_impl(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->type_params[0].bounds.size() == 3);
    CHECK(result->type_params[0].bounds[0].trait_name.segments()[0].value == "Display");
    CHECK(result->type_params[0].bounds[1].trait_name.segments()[0].value == "Clone");
    CHECK(result->type_params[0].bounds[2].trait_name.segments()[0].value == "Eq");
  }
}
