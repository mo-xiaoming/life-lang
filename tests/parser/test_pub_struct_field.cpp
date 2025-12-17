#include <doctest/doctest.h>

#include "../../src/parser.hpp"

TEST_CASE("parse_struct_def - pub fields") {
  life_lang::parser::Parser parser(R"(
struct Point {
  pub x: I32,
  y: I32,
  pub z: I32
}
)");

  auto const result = parser.parse_struct_def();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    CHECK(result->name == "Point");
    REQUIRE(result->fields.size() == 3);

    CHECK(result->fields[0].name == "x");
    CHECK(result->fields[0].is_pub == true);

    CHECK(result->fields[1].name == "y");
    CHECK(result->fields[1].is_pub == false);

    CHECK(result->fields[2].name == "z");
    CHECK(result->fields[2].is_pub == true);
  }
}

TEST_CASE("parse_struct_def - all pub fields") {
  life_lang::parser::Parser parser(R"(
struct User {
  pub name: String,
  pub age: I32
}
)");

  auto const result = parser.parse_struct_def();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    REQUIRE(result->fields.size() == 2);
    CHECK(result->fields[0].is_pub == true);
    CHECK(result->fields[1].is_pub == true);
  }
}

TEST_CASE("parse_struct_def - no pub fields") {
  life_lang::parser::Parser parser(R"(
struct Internal {
  data: I32,
  flag: Bool
}
)");

  auto const result = parser.parse_struct_def();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    REQUIRE(result->fields.size() == 2);
    CHECK(result->fields[0].is_pub == false);
    CHECK(result->fields[1].is_pub == false);
  }
}
