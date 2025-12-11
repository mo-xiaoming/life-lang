#include "internal_rules.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

auto parse_trait_impl(std::string const& a_input) {
  auto begin = a_input.cbegin();
  return life_lang::internal::parse_trait_impl(begin, a_input.cend());
}

}  // namespace

TEST_CASE("Parse Trait_Impl - success cases", "[parser]") {
  SECTION("basic impl") {
    auto const* input = R"(
      impl Display for Point {
        fn to_string(self): String {
          return "Point";
        }
      }
    )";
    auto const result = parse_trait_impl(input);
    REQUIRE(result);
    CHECK(result->trait_name.segments.front().value == "Display");
    CHECK(result->type_name.segments.front().value == "Point");
    CHECK(result->methods.size() == 1);
  }

  SECTION("generic impl") {
    auto const* input = R"(
      impl<T> Iterator<T> for Array<T> {
        fn next(mut self): Option<T> {
          return None;
        }
      }
    )";
    auto const result = parse_trait_impl(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->methods.size() == 1);
  }

  SECTION("impl with multiple methods") {
    auto const* input = R"(
      impl Comparable for I32 {
        fn compare(self, other: I32): Ordering {
          return Equal;
        }
        fn less_than(self, other: I32): Bool {
          return false;
        }
      }
    )";
    auto const result = parse_trait_impl(input);
    REQUIRE(result);
    CHECK(result->methods.size() == 2);
  }

  SECTION("empty impl") {
    auto const* input = R"(impl Marker for Unit {})";
    auto const result = parse_trait_impl(input);
    REQUIRE(result);
    CHECK(result->methods.empty());
  }
}

TEST_CASE("Parse Trait_Impl - failure cases", "[parser]") {
  SECTION("missing for keyword") {
    auto const* input = R"(
      impl Display Point {
        fn to_string(self): String {
          return "Point";
        }
      }
    )";
    auto const result = parse_trait_impl(input);
    CHECK_FALSE(result);
  }

  SECTION("impl without trait name") {
    auto const* input = R"(
      impl for Point {
        fn test(): Unit { }
      }
    )";
    auto const result = parse_trait_impl(input);
    CHECK_FALSE(result);
  }
}
