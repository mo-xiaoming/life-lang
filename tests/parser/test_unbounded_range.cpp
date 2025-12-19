#include <doctest/doctest.h>
#include "parser.hpp"
#include "sexp.hpp"
#include "utils.hpp"

using namespace life_lang;
using namespace life_lang::ast;
using namespace life_lang::parser;
using namespace test_sexp;

TEST_CASE("Unbounded range - unbounded end (a..)") {
  Parser parser("10..");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = range_expr(integer("10"), "nil", false);
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Unbounded range - unbounded end inclusive (a..=) - not valid syntax") {
  // Note: a..= doesn't make sense semantically, but parser may accept it
  // Semantic analysis should reject it
  Parser parser("10..=");
  auto const expr = parser.parse_expr();
  // Parser should handle it, semantic analysis will validate
  REQUIRE(expr.has_value());
}

TEST_CASE("Unbounded range - unbounded start (..b)") {
  Parser parser("..100");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = range_expr("nil", integer("100"), false);
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Unbounded range - unbounded start inclusive (..=b)") {
  Parser parser("..=100");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = range_expr("nil", integer("100"), true);
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Unbounded range - fully unbounded (..)") {
  Parser parser("..");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = range_expr("nil", "nil", false);
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Unbounded range - with variable start") {
  Parser parser("x..");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = range_expr(var_name("x"), "nil", false);
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Unbounded range - with variable end") {
  Parser parser("..y");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = range_expr("nil", var_name("y"), false);
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Unbounded range - with expression start") {
  Parser parser("x + 1..");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = range_expr(binary_expr("+", var_name("x"), integer("1")), "nil", false);
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Unbounded range - with expression end") {
  Parser parser("..y - 1");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = range_expr("nil", binary_expr("-", var_name("y"), integer("1")), false);
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Bounded range - still works (a..b)") {
  Parser parser("1..10");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = range_expr(integer("1"), integer("10"), false);
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Bounded range - inclusive still works (a..=b)") {
  Parser parser("1..=10");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = range_expr(integer("1"), integer("10"), true);
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Unbounded range - in for loop") {
  Parser parser("for i in 0.. { }");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = for_expr(simple_pattern("i"), range_expr(integer("0"), "nil", false), block({}));
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Unbounded range - in let statement") {
  Parser parser("let range = ..100;");
  auto const stmt = parser.parse_statement();
  REQUIRE(stmt.has_value());
  if (stmt.has_value()) {
    auto const expected = let_statement(simple_pattern("range"), range_expr("nil", integer("100"), false));
    CHECK(to_sexp_string(*stmt, 0) == expected);
  }
}
