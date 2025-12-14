#include "internal_rules.hpp"

#include <doctest/doctest.h>

namespace {

auto parse_trait_impl(std::string const& input_) {
  return life_lang::internal::parse_trait_impl(input_);
}

}  // namespace

TEST_CASE("Parse Trait_Impl - success cases") {
  SUBCASE("basic impl") {
    auto const* input = R"(
      impl Display for Point {
        fn to_string(self): String {
          return "Point";
        }
      }
    )";
    auto const result = parse_trait_impl(input);
    REQUIRE(result);
    CHECK(result->trait_name.segments().front().value == "Display");
    CHECK(result->type_name.segments().front().value == "Point");
    CHECK(result->methods.size() == 1);
  }

  SUBCASE("generic impl") {
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

  SUBCASE("impl with multiple methods") {
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

  SUBCASE("empty impl") {
    auto const* input = R"(impl Marker for Unit {})";
    auto const result = parse_trait_impl(input);
    REQUIRE(result);
    CHECK(result->methods.empty());
  }

  SUBCASE("impl with single associated type") {
    auto const* input = R"(
      impl Iterator for Vec {
        type Item = I32;
        fn next(mut self): Option<Item> {
          return None;
        }
      }
    )";
    auto const result = parse_trait_impl(input);
    REQUIRE(result);
    CHECK(result->trait_name.segments().front().value == "Iterator");
    CHECK(result->type_name.segments().front().value == "Vec");
    CHECK(result->assoc_type_impls.size() == 1);
    CHECK(result->assoc_type_impls[0].name == "Item");
    CHECK(result->assoc_type_impls[0].type_value.segments().front().value == "I32");
    CHECK(result->methods.size() == 1);
  }

  SUBCASE("impl with multiple associated types") {
    auto const* input = R"(
      impl Graph for Network {
        type Node = Vertex;
        type Edge = Connection;
        fn add_node(mut self, node: Node): Unit { }
        fn add_edge(mut self, edge: Edge): Unit { }
      }
    )";
    auto const result = parse_trait_impl(input);
    REQUIRE(result);
    CHECK(result->assoc_type_impls.size() == 2);
    CHECK(result->assoc_type_impls[0].name == "Node");
    CHECK(result->assoc_type_impls[0].type_value.segments().front().value == "Vertex");
    CHECK(result->assoc_type_impls[1].name == "Edge");
    CHECK(result->assoc_type_impls[1].type_value.segments().front().value == "Connection");
    CHECK(result->methods.size() == 2);
  }

  SUBCASE("generic impl with associated type using type parameter") {
    auto const* input = R"(
      impl<T> Iterator for Array<T> {
        type Item = T;
        fn next(mut self): Option<T> {
          return None;
        }
      }
    )";
    auto const result = parse_trait_impl(input);
    REQUIRE(result);
    CHECK(result->type_params.size() == 1);
    CHECK(result->assoc_type_impls.size() == 1);
    CHECK(result->assoc_type_impls[0].name == "Item");
    CHECK(result->assoc_type_impls[0].type_value.segments().front().value == "T");
    CHECK(result->methods.size() == 1);
  }

  SUBCASE("impl with complex associated type") {
    auto const* input = R"(
      impl<T> Transformer for Converter<T> {
        type Output = Vec<T>;
        fn transform(self, input: T): Output {
          return vec;
        }
      }
    )";
    auto const result = parse_trait_impl(input);
    REQUIRE(result);
    CHECK(result->assoc_type_impls.size() == 1);
    CHECK(result->assoc_type_impls[0].name == "Output");
    CHECK(result->assoc_type_impls[0].type_value.segments().front().value == "Vec");
    CHECK(result->assoc_type_impls[0].type_value.segments().front().type_params.size() == 1);
  }

  SUBCASE("impl with only associated types, no methods") {
    auto const* input = R"(
      impl Types for Container {
        type Item = String;
        type Output = I32;
      }
    )";
    auto const result = parse_trait_impl(input);
    REQUIRE(result);
    CHECK(result->assoc_type_impls.size() == 2);
    CHECK(result->methods.empty());
  }
}

TEST_CASE("Parse Trait_Impl - failure cases") {
  SUBCASE("missing for keyword") {
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

  SUBCASE("impl without trait name") {
    auto const* input = R"(
      impl for Point {
        fn test(): Unit { }
      }
    )";
    auto const result = parse_trait_impl(input);
    CHECK_FALSE(result);
  }

  SUBCASE("missing semicolon after associated type") {
    auto const* input = R"(
      impl Iterator for Vec {
        type Item = I32
        fn next(mut self): Option<Item> { }
      }
    )";
    auto const result = parse_trait_impl(input);
    CHECK_FALSE(result);
  }

  // NOTE: Naming convention test removed - parser accepts any identifier
  // Naming convention enforcement is deferred to semantic analysis phase
  // Test for snake_case associated type name in impl was here

  SUBCASE("missing equals in associated type impl") {
    auto const* input = R"(
      impl Iterator for Vec {
        type Item I32;
        fn next(mut self): Option<Item> { }
      }
    )";
    auto const result = parse_trait_impl(input);
    CHECK_FALSE(result);
  }
}
