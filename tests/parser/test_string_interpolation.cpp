// ReSharper disable CppDFAUnreachableCode
#include <doctest/doctest.h>

#include "parser.hpp"
#include "sexp.hpp"
#include "utils.hpp"

using namespace life_lang::parser;
using namespace life_lang::ast;
using namespace test_sexp;

TEST_CASE("String interpolation - single variable") {
  Parser parser(R"("value: {x}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const expected = string_interp({string_part("value: "), var_name("x")});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - multiple variables") {
  Parser parser(R"xxx("({x}, {y})")xxx");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const expected =
      string_interp({string_part("("), var_name("x"), string_part(", "), var_name("y"), string_part(")")});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - expression") {
  Parser parser(R"("result: {1 + 2}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const expected = string_interp({string_part("result: "), binary_expr("+", integer("1"), integer("2"))});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - field access") {
  Parser parser(R"("name: {user.name}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const expected = string_interp({string_part("name: "), field_access(var_name("user"), "name")});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - function call") {
  Parser parser(R"("result: {calculate(x, y)}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const expected =
      string_interp({string_part("result: "), function_call(var_name("calculate"), {var_name("x"), var_name("y")})});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - method call") {
  Parser parser(R"("upper: {name.to_upper()}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  // Method call parses as function call with path
  auto const expected = string_interp({string_part("upper: "), function_call(var_name_path({"name", "to_upper"}), {})});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - cast expression") {
  Parser parser(R"("value: {x as I64}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const expected = string_interp({string_part("value: "), cast_expr(var_name("x"), type_name("I64"))});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - nested expression") {
  Parser parser(R"("total: {(a + b) * c}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  // Parentheses affect precedence but aren't preserved in AST
  auto const expected = string_interp(
      {string_part("total: "), binary_expr("*", binary_expr("+", var_name("a"), var_name("b")), var_name("c"))}
  );
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - with escape sequences") {
  Parser parser(R"("path: {path}\n")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const expected = string_interp({string_part("path: "), var_name("path"), string_part("\\n")});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - starting with expression") {
  Parser parser(R"("{x} is the value")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const expected = string_interp({var_name("x"), string_part(" is the value")});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - ending with expression") {
  Parser parser(R"("value is {x}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const expected = string_interp({string_part("value is "), var_name("x")});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - only expression") {
  Parser parser(R"("{x}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const expected = string_interp({var_name("x")});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - adjacent expressions") {
  Parser parser(R"("{x}{y}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const expected = string_interp({var_name("x"), var_name("y")});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - empty string with expression") {
  Parser parser(R"("{}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  // Empty braces {} are treated as literal characters (for format function placeholders)
  CHECK(to_sexp_string(*expr, 0) == R"((string "\"{}\""))");
}

TEST_CASE("Regular string - format placeholders") {
  Parser parser(R"xxx("({}, {})")xxx");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  // Empty braces should be treated as literal string
  CHECK(to_sexp_string(*expr, 0) == R"xxx((string "\"({}, {})\""))xxx");
}

TEST_CASE("Regular string - no interpolation") {
  Parser parser(R"("plain string")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  CHECK(to_sexp_string(*expr, 0) == R"((string "\"plain string\""))");
}

TEST_CASE("String with escaped braces - literal braces") {
  Parser parser(R"("Literal: \{not interpolated\}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  // Escaped braces should result in regular string, not interpolation
  CHECK(to_sexp_string(*expr, 0) == R"((string "\"Literal: \\{not interpolated\\}\""))");
}

TEST_CASE("String with mixed escaped and interpolated braces") {
  Parser parser(R"("Escaped: \{literal\}, Interpolated: {x}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  // Should be interpolation with escaped braces in literal parts
  auto const expected = string_interp({string_part("Escaped: \\{literal\\}, Interpolated: "), var_name("x")});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String with only opening escaped brace") {
  Parser parser(R"("JSON: \{\"key\": \"value\"\}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  // All escaped - should be regular string
  CHECK(to_sexp_string(*expr, 0) == R"((string "\"JSON: \\{\\\"key\\\": \\\"value\\\"\\}\""))");
}

TEST_CASE("String interpolation - comparison expression") {
  Parser parser(R"("check: {x == y}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const expected = string_interp({string_part("check: "), binary_expr("==", var_name("x"), var_name("y"))});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

TEST_CASE("String interpolation - array index") {
  Parser parser(R"("item: {items[0]}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const expected = string_interp({string_part("item: "), index_expr(var_name("items"), integer("0"))});
  CHECK(to_sexp_string(*expr, 0) == expected);
}

// NOTE: Tuple field access syntax (pair.0, triple.1, etc.) is not yet implemented in the parser.
// Once it's added, uncomment this test to verify it works inside string interpolation.
// TEST_CASE("String interpolation - tuple access") {
//   Parser parser(R"("first: {pair.0}")");
//   auto const expr = parser.parse_expr();
//   REQUIRE(expr.has_value());
//   auto const expected = string_interp(
//       {string_part("first: "), field_access(var_name("pair"), "0")});
//   CHECK(to_sexp_string(*expr, 0) == expected);
// }

TEST_CASE("String interpolation - complex expression") {
  Parser parser(R"("value: {data.items[index].name.to_upper()}")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  auto const output = to_sexp_string(*expr, 0);
  // Verify it's a string_interp with the complex chain
  CHECK(output.starts_with("(string_interp "));
  CHECK(output.find("data") != std::string::npos);
  CHECK(output.find("items") != std::string::npos);
  CHECK(output.find("index") != std::string::npos);
  CHECK(output.find("name") != std::string::npos);
  CHECK(output.find("to_upper") != std::string::npos);
}
