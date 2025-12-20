#include <doctest/doctest.h>
#include "internal_rules.hpp"

namespace {

auto parse_trait_impl(std::string const& input_) {
  return life_lang::internal::parse_trait_impl(input_);
}

}  // namespace

TEST_CASE("Parse Trait_Impl - success cases") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "basic impl", .input = R"(
      impl Display for Point {
        fn to_string(self): String {
          return "Point";
        }
      }
    )"},
      {.name = "generic impl", .input = R"(
      impl<T> Iterator<T> for Array<T> {
        fn next(mut self): Option<T> {
          return None;
        }
      }
    )"},
      {.name = "impl with multiple methods", .input = R"(
      impl Comparable for I32 {
        fn compare(self, other: I32): Ordering {
          return Equal;
        }
        fn less_than(self, other: I32): Bool {
          return false;
        }
      }
    )"},
      {.name = "empty impl", .input = R"(impl Marker for Unit {})"},
      {.name = "impl with single associated type", .input = R"(
      impl Iterator for Vec {
        type Item = I32;
        fn next(mut self): Option<Item> {
          return None;
        }
      }
    )"},
      {.name = "impl with multiple associated types", .input = R"(
      impl Graph for Network {
        type Node = Vertex;
        type Edge = Connection;
        fn add_node(mut self, node: Node): Unit { }
        fn add_edge(mut self, edge: Edge): Unit { }
      }
    )"},
      {.name = "generic impl with associated type using type parameter", .input = R"(
      impl<T> Iterator for Array<T> {
        type Item = T;
        fn next(mut self): Option<T> {
          return None;
        }
      }
    )"},
      {.name = "impl with complex associated type", .input = R"(
      impl<T> Transformer for Converter<T> {
        type Output = Vec<T>;
        fn transform(self, input: T): Output {
          return vec;
        }
      }
    )"},
      {.name = "impl with only associated types, no methods", .input = R"(
      impl Types for Container {
        type Item = String;
        type Output = I32;
      }
    )"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_trait_impl(test.input);
      CHECK(result);
    }
  }
}

TEST_CASE("Parse Trait_Impl - failure cases") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "missing for keyword", .input = R"(
      impl Display Point {
        fn to_string(self): String {
          return "Point";
        }
      }
    )"},
      {.name = "impl without trait name", .input = R"(
      impl for Point {
        fn test(): Unit { }
      }
    )"},
      {.name = "missing semicolon after associated type", .input = R"(
      impl Iterator for Vec {
        type Item = I32
        fn next(mut self): Option<Item> { }
      }
    )"},
      {.name = "missing equals in associated type impl", .input = R"(
      impl Iterator for Vec {
        type Item I32;
        fn next(mut self): Option<Item> { }
      }
    )"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_trait_impl(test.input);
      CHECK_FALSE(result);
    }
  }
}
