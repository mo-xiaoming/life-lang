#include <doctest/doctest.h>
#include "internal_rules.hpp"

namespace {

auto parse_trait(std::string const& input_) {
  return life_lang::internal::parse_trait_def(input_);
}

}  // namespace

TEST_CASE("Parse Trait_Definition - success cases") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "empty trait", .input = R"(trait Marker {})"},
      {.name = "trait with single method", .input = R"(trait Display { fn to_string(self): String; })"},
      {.name = "generic trait", .input = R"(trait Iterator<T> { fn next(mut self): Option<T>; })"},
      {.name = "trait with multiple methods", .input = R"(
      trait Comparable {
        fn compare(self, other: Self): Ordering;
        fn less_than(self, other: Self): Bool;
      }
    )"},
      {.name = "trait with multiple type parameters", .input = R"(
      trait Map<K, V> {
        fn get(self, key: K): Option<V>;
      }
    )"},
      {.name = "trait with single associated type", .input = R"(
      trait Iterator {
        type Item;
        fn next(mut self): Option<Item>;
      }
    )"},
      {.name = "trait with associated type with single bound", .input = R"(
      trait Container {
        type Item: Display;
        fn len(self): I32;
      }
    )"},
      {.name = "trait with associated type with multiple bounds", .input = R"(
      trait Collection {
        type Item: Clone + Display + Debug;
        fn get(self, index: I32): Option<Item>;
      }
    )"},
      {.name = "trait with multiple associated types", .input = R"(
      trait Graph {
        type Node;
        type Edge: Display;
        fn add_node(mut self, node: Node): Unit;
        fn add_edge(mut self, edge: Edge): Unit;
      }
    )"},
      {.name = "trait with only associated types, no methods", .input = R"(
      trait Types {
        type Input;
        type Output;
      }
    )"},
      {.name = "generic trait with associated type", .input = R"(
      trait Transformer<T> {
        type Output;
        fn transform(self, input: T): Output;
      }
    )"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_trait(test.input);
      CHECK(result);
    }
  }
}

TEST_CASE("Parse Trait_Definition - failure cases") {
  struct Test_Case {
    char const* name;
    char const* input;
  };

  static constexpr Test_Case k_test_cases[] = {
      {.name = "missing semicolon after method", .input = R"(trait Display { fn to_string(self): String })"},
      {.name = "missing semicolon after associated type",
       .input = R"(trait Iterator { type Item fn next(mut self): Option<Item>; })"},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      auto const result = parse_trait(test.input);
      CHECK_FALSE(result);
    }
  }
}
