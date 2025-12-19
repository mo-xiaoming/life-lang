#include <doctest/doctest.h>

#include "parser.hpp"
#include "sexp.hpp"
#include "utils.hpp"

using namespace life_lang::parser;
using namespace life_lang::ast;
using namespace test_sexp;

TEST_CASE("Or pattern - simple literals") {
  Parser parser("1 | 2 | 3");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected =
        or_pattern({literal_pattern(integer("1")), literal_pattern(integer("2")), literal_pattern(integer("3"))});
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Or pattern - variable names") {
  Parser parser("x | y | z");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = or_pattern({simple_pattern("x"), simple_pattern("y"), simple_pattern("z")});
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Or pattern - two alternatives") {
  Parser parser("true | false");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = or_pattern({literal_pattern("(bool true)"), literal_pattern("(bool false)")});
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Or pattern - nested in tuple") {
  Parser parser("(1 | 2, 3 | 4)");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = tuple_pattern(
        {or_pattern({literal_pattern(integer("1")), literal_pattern(integer("2"))}),
         or_pattern({literal_pattern(integer("3")), literal_pattern(integer("4"))})}
    );
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Or pattern - wildcard alternatives") {
  Parser parser("_ | x");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = or_pattern({wildcard_pattern(), simple_pattern("x")});
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Or pattern - in let statement") {
  Parser parser("let x | y = value;");
  auto const stmt = parser.parse_statement();
  REQUIRE(stmt.has_value());
  if (stmt.has_value()) {
    auto const expected_pattern = or_pattern({simple_pattern("x"), simple_pattern("y")});
    auto const expected = let_statement(expected_pattern, var_name("value"));
    CHECK(to_sexp_string(*stmt, 0) == expected);
  }
}

TEST_CASE("Or pattern - strings") {
  Parser parser(R"("hello" | "world")");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = or_pattern({literal_pattern(string(R"("hello")")), literal_pattern(string(R"("world")"))});
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Or pattern - single pattern (no |)") {
  Parser parser("42");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    // Should NOT be wrapped in or_pattern
    auto const expected = literal_pattern(integer("42"));
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Or pattern - enum variants (future: requires type names)") {
  // This will parse as simple patterns now, semantic analysis will validate later
  Parser parser("Some | None");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = or_pattern({simple_pattern("Some"), simple_pattern("None")});
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Or pattern - four alternatives") {
  Parser parser("1 | 2 | 3 | 4");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = or_pattern(
        {literal_pattern(integer("1")),
         literal_pattern(integer("2")),
         literal_pattern(integer("3")),
         literal_pattern(integer("4"))}
    );
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Or pattern - mixed types (semantic analysis will reject)") {
  // Parser accepts, semantic analysis will reject type mismatch
  Parser parser("1 | \"hello\"");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = or_pattern({literal_pattern(integer("1")), literal_pattern(string(R"("hello")"))});
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}
