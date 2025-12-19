// Integration test for string interpolation feature
#include <doctest/doctest.h>

#include "../parser/utils.hpp"
#include "parser.hpp"
#include "sexp.hpp"

using namespace life_lang::parser;
using namespace life_lang::ast;
using namespace test_sexp;

TEST_CASE("String interpolation - simple variable") {
  Parser parser(R"("Hello, {name}!")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());

  auto const expected = string_interp({string_part("Hello, "), var_name("name"), string_part("!")});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - multiple variables") {
  Parser parser(R"xxx("Point: ({x}, {y})")xxx");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());

  auto const expected =
      string_interp({string_part("Point: ("), var_name("x"), string_part(", "), var_name("y"), string_part(")")});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - expression") {
  Parser parser(R"("Result: {x + y}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());

  auto const expected = string_interp({string_part("Result: "), binary_expr("+", var_name("x"), var_name("y"))});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - field access") {
  Parser parser(R"("Name: {user.name}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());

  auto const expected = string_interp({string_part("Name: "), field_access(var_name("user"), "name")});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - function call") {
  Parser parser(R"("Value: {get_value()}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());

  auto const expected = string_interp({string_part("Value: "), function_call(var_name("get_value"), {})});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - method chain") {
  Parser parser(R"("Upper: {name.to_upper()}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());

  auto const expected = string_interp({string_part("Upper: "), function_call(var_name_path({"name", "to_upper"}), {})});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("Empty braces - not interpolation") {
  Parser parser(R"("Format: {}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());

  CHECK(to_sexp_string(*expr, 0) == R"((string "\"Format: {}\""))");
}

TEST_CASE("Escaped braces - literal") {
  Parser parser(R"("Literal: \{value\}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());

  CHECK(to_sexp_string(*expr, 0) == R"((string "\"Literal: \\{value\\}\""))");
}

TEST_CASE("Mixed escaped and interpolated") {
  Parser parser(R"("Literal \{x\}, interpolated {y}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());

  auto const expected = string_interp({string_part("Literal \\{x\\}, interpolated "), var_name("y")});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation in function") {
  constexpr auto k_input = R"(
fn greet(name: String): String {
  return "Hello, {name}!";
}
)";

  Parser parser(k_input);
  auto const module = parser.parse_module();
  REQUIRE(module);
  CHECK(module->items.size() == 1);
}
