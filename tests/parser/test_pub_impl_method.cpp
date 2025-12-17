#include <doctest/doctest.h>

#include "../../src/parser.hpp"

TEST_CASE("parse_impl_block - pub methods") {
  life_lang::parser::Parser parser(R"(
impl Point {
  pub fn new(x: I32, y: I32): Point {
    return Point { x: x, y: y };
  }
  
  fn internal_helper(self): I32 {
    return self.x;
  }
  
  pub fn distance(self): F64 {
    return 0.0;
  }
}
)");

  auto const result = parser.parse_impl_block();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    REQUIRE(result->methods.size() == 3);

    CHECK(result->methods[0].declaration.name == "new");
    CHECK(result->methods[0].is_pub == true);

    CHECK(result->methods[1].declaration.name == "internal_helper");
    CHECK(result->methods[1].is_pub == false);

    CHECK(result->methods[2].declaration.name == "distance");
    CHECK(result->methods[2].is_pub == true);
  }
}

TEST_CASE("parse_impl_block - all pub methods") {
  life_lang::parser::Parser parser(R"(
impl Calculator {
  pub fn add(a: I32, b: I32): I32 {
    return a + b;
  }
  
  pub fn subtract(a: I32, b: I32): I32 {
    return a - b;
  }
}
)");

  auto const result = parser.parse_impl_block();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    REQUIRE(result->methods.size() == 2);
    CHECK(result->methods[0].is_pub == true);
    CHECK(result->methods[1].is_pub == true);
  }
}

TEST_CASE("parse_impl_block - no pub methods") {
  life_lang::parser::Parser parser(R"(
impl Internal {
  fn helper(): I32 {
    return 42;
  }
}
)");

  auto const result = parser.parse_impl_block();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    REQUIRE(result->methods.size() == 1);
    CHECK(result->methods[0].is_pub == false);
  }
}
