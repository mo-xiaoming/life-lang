#include <doctest/doctest.h>
#include "parser.hpp"
#include "sexp.hpp"
#include "utils.hpp"

using namespace life_lang;
using namespace life_lang::ast;
using namespace life_lang::parser;
using namespace test_sexp;

TEST_CASE("Rest pattern - type-only matching (empty fields with ..)") {
  Parser parser("Point { .. }");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = struct_pattern_with_rest(type_name("Point"), {});
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Rest pattern - single field with rest") {
  Parser parser("User { name, .. }");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = struct_pattern_with_rest(type_name("User"), {field_pattern("name", simple_pattern("name"))});
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Rest pattern - multiple fields with rest") {
  Parser parser("Config { host, port, .. }");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = struct_pattern_with_rest(
        type_name("Config"),
        {field_pattern("host", simple_pattern("host")), field_pattern("port", simple_pattern("port"))}
    );
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Rest pattern - with explicit pattern binding") {
  Parser parser("Request { method: m, url: u, .. }");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = struct_pattern_with_rest(
        type_name("Request"),
        {field_pattern("method", simple_pattern("m")), field_pattern("url", simple_pattern("u"))}
    );
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Rest pattern - trailing comma before ..") {
  Parser parser("User { name, age, .. }");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = struct_pattern_with_rest(
        type_name("User"),
        {field_pattern("name", simple_pattern("name")), field_pattern("age", simple_pattern("age"))}
    );
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Rest pattern - no rest (all fields explicit)") {
  Parser parser("Point { x, y }");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = struct_pattern(
        type_name("Point"),
        {field_pattern("x", simple_pattern("x")), field_pattern("y", simple_pattern("y"))}
    );
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Rest pattern - error: .. not at end") {
  Parser parser("Config { .., debug }");
  auto const pattern = parser.parse_pattern();
  // Parser should accept .. anywhere for now, semantic analysis will enforce position
  // Actually, the parser enforces that .. must be last
  CHECK_FALSE(pattern.has_value());
}

TEST_CASE("Rest pattern - error: comma after ..") {
  Parser parser("User { name, .., }");
  auto const pattern = parser.parse_pattern();
  CHECK_FALSE(pattern.has_value());
}

TEST_CASE("Rest pattern - in match expression") {
  Parser parser(R"(
    match value {
      Point { .. } => "point",
      Circle { radius, .. } => "circle",
    }
  )");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = match_expr(
        var_name("value"),
        {match_arm(struct_pattern_with_rest(type_name("Point"), {}), string(R"("point")")),
         match_arm(
             struct_pattern_with_rest(type_name("Circle"), {field_pattern("radius", simple_pattern("radius"))}),
             string(R"("circle")")
         )}
    );
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Rest pattern - nested in tuple pattern") {
  Parser parser("(Point { x, .. }, Circle { .. })");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = tuple_pattern(
        {struct_pattern_with_rest(type_name("Point"), {field_pattern("x", simple_pattern("x"))}),
         struct_pattern_with_rest(type_name("Circle"), {})}
    );
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Rest pattern - with nested patterns") {
  Parser parser("Request { method, body: Some(data), .. }");
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = struct_pattern_with_rest(
        type_name("Request"),
        {field_pattern("method", simple_pattern("method")),
         field_pattern("body", enum_pattern(type_name("Some"), {simple_pattern("data")}))}
    );
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}
