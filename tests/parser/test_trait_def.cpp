#include "internal_rules.hpp"

#include <doctest/doctest.h>

namespace {

auto parse_trait(std::string const& input_) {
  return life_lang::internal::parse_trait_def(input_);
}

}  // namespace

TEST_CASE("Parse Trait_Definition - success cases") {
  SUBCASE("empty trait") {
    auto const* input = R"(trait Marker {})";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Marker");
    CHECK(result->methods.empty());
  }

  SUBCASE("trait with single method") {
    auto const* input = R"(trait Display { fn to_string(self): String; })";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Display");
    CHECK(result->methods.size() == 1);
    CHECK(result->methods[0].name == "to_string");
  }

  SUBCASE("generic trait") {
    auto const* input = R"(trait Iterator<T> { fn next(mut self): Option<T>; })";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Iterator");
    CHECK(result->type_params.size() == 1);
    CHECK(result->methods.size() == 1);
  }

  SUBCASE("trait with multiple methods") {
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

  SUBCASE("trait with multiple type parameters") {
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

  SUBCASE("trait with single associated type") {
    auto const* input = R"(
      trait Iterator {
        type Item;
        fn next(mut self): Option<Item>;
      }
    )";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Iterator");
    CHECK(result->assoc_types.size() == 1);
    CHECK(result->assoc_types[0].name == "Item");
    CHECK(result->assoc_types[0].bounds.empty());
    CHECK(result->methods.size() == 1);
  }

  SUBCASE("trait with associated type with single bound") {
    auto const* input = R"(
      trait Container {
        type Item: Display;
        fn len(self): I32;
      }
    )";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Container");
    CHECK(result->assoc_types.size() == 1);
    CHECK(result->assoc_types[0].name == "Item");
    CHECK(result->assoc_types[0].bounds.size() == 1);
    CHECK(result->methods.size() == 1);
  }

  SUBCASE("trait with associated type with multiple bounds") {
    auto const* input = R"(
      trait Collection {
        type Item: Clone + Display + Debug;
        fn get(self, index: I32): Option<Item>;
      }
    )";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Collection");
    CHECK(result->assoc_types.size() == 1);
    CHECK(result->assoc_types[0].name == "Item");
    CHECK(result->assoc_types[0].bounds.size() == 3);
    CHECK(result->methods.size() == 1);
  }

  SUBCASE("trait with multiple associated types") {
    auto const* input = R"(
      trait Graph {
        type Node;
        type Edge: Display;
        fn add_node(mut self, node: Node): Unit;
        fn add_edge(mut self, edge: Edge): Unit;
      }
    )";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Graph");
    CHECK(result->assoc_types.size() == 2);
    CHECK(result->assoc_types[0].name == "Node");
    CHECK(result->assoc_types[0].bounds.empty());
    CHECK(result->assoc_types[1].name == "Edge");
    CHECK(result->assoc_types[1].bounds.size() == 1);
    CHECK(result->methods.size() == 2);
  }

  SUBCASE("trait with only associated types, no methods") {
    auto const* input = R"(
      trait Types {
        type Input;
        type Output;
      }
    )";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Types");
    CHECK(result->assoc_types.size() == 2);
    CHECK(result->methods.empty());
  }

  SUBCASE("generic trait with associated type") {
    auto const* input = R"(
      trait Transformer<T> {
        type Output;
        fn transform(self, input: T): Output;
      }
    )";
    auto const result = parse_trait(input);
    REQUIRE(result);
    CHECK(result->name == "Transformer");
    CHECK(result->type_params.size() == 1);
    CHECK(result->assoc_types.size() == 1);
    CHECK(result->assoc_types[0].name == "Output");
    CHECK(result->methods.size() == 1);
  }
}

TEST_CASE("Parse Trait_Definition - failure cases") {
  SUBCASE("missing semicolon after method") {
    auto const* input = R"(trait Display { fn to_string(self): String })";
    auto const result = parse_trait(input);
    CHECK_FALSE(result);
  }

  // NOTE: Naming convention tests removed - parser accepts any identifier
  // Naming convention enforcement is deferred to semantic analysis phase
  // Tests for snake_case/lowercase trait names and associated type names were here

  SUBCASE("missing semicolon after associated type") {
    auto const* input = R"(trait Iterator { type Item fn next(mut self): Option<Item>; })";
    auto const result = parse_trait(input);
    CHECK_FALSE(result);
  }
}
