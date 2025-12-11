#include "internal_rules.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

auto parse_trait(std::string const& a_input) {
  auto begin = a_input.cbegin();
  return life_lang::internal::parse_trait_def(begin, a_input.cend());
}

}  // namespace

TEST_CASE("Parse Trait_Definition - success cases", "[parser]") {
  SECTION("empty trait") {
    auto const* input = R"(trait Marker {})";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Marker");
    CHECK(result->methods.empty());
  }

  SECTION("trait with single method") {
    auto const* input = R"(trait Display { fn to_string(self): String; })";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Display");
    CHECK(result->methods.size() == 1);
    CHECK(result->methods[0].name == "to_string");
  }

  SECTION("generic trait") {
    auto const* input = R"(trait Iterator<T> { fn next(mut self): Option<T>; })";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Iterator");
    CHECK(result->type_params.size() == 1);
    CHECK(result->methods.size() == 1);
  }

  SECTION("trait with multiple methods") {
    auto const* input = R"(
      trait Comparable {
        fn compare(self, other: Self): Ordering;
        fn less_than(self, other: Self): Bool;
      }
    )";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Comparable");
    CHECK(result->methods.size() == 2);
  }

  SECTION("trait with multiple type parameters") {
    auto const* input = R"(
      trait Map<K, V> {
        fn get(self, key: K): Option<V>;
      }
    )";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Map");
    CHECK(result->type_params.size() == 2);
  }
}

TEST_CASE("Parse Trait_Definition - failure cases", "[parser]") {
  SECTION("missing semicolon after method") {
    auto const* input = R"(trait Display { fn to_string(self): String })";
    auto const result = parse_trait(input);
    CHECK_FALSE(result);
  }

  SECTION("snake_case trait name") {
    auto const* input = R"(trait display_trait { fn show(self): Unit; })";
    auto const result = parse_trait(input);
    CHECK_FALSE(result);
  }

  SECTION("lowercase trait name") {
    auto const* input = R"(trait display { fn show(self): Unit; })";
    auto const result = parse_trait(input);
    CHECK_FALSE(result);
  }
}
