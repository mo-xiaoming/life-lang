#include <doctest/doctest.h>

#include "../parser/utils.hpp"
#include "diagnostics.hpp"
#include "parser/parser.hpp"
#include "parser/sexp.hpp"

using namespace life_lang::parser;
using namespace life_lang::ast;
using namespace test_sexp;

TEST_CASE("String interpolation - simple variable") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", R"("Hello, {name}!")"};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = string_interp({string_part("Hello, "), var_name("name"), string_part("!")});
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("String interpolation - multiple variables") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", R"xxx("Point: ({x}, {y})")xxx"};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected =
        string_interp({string_part("Point: ("), var_name("x"), string_part(", "), var_name("y"), string_part(")")});
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("String interpolation - expression") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", R"("Result: {x + y}")"};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = string_interp({string_part("Result: "), binary_expr("+", var_name("x"), var_name("y"))});
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("String interpolation - field access") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", R"("Name: {user.name}")"};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = string_interp({string_part("Name: "), field_access(var_name("user"), "name")});
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("String interpolation - function call") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", R"("Value: {get_value()}")"};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = string_interp({string_part("Value: "), function_call(var_name("get_value"), {})});
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("String interpolation - method chain") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", R"("Upper: {name.to_upper()}")"};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected =
        string_interp({string_part("Upper: "), function_call(var_name_path({"name", "to_upper"}), {})});
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Empty braces - not interpolation") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", R"("Format: {}")"};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = string(R"("Format: {}")");
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Escaped braces - literal") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", R"("Literal: \{value\}")"};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = string(R"("Literal: \{value\}")");
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Mixed escaped and interpolated") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", R"("Literal \{x\}, interpolated {y}")"};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = string_interp({string_part("Literal \\{x\\}, interpolated "), var_name("y")});
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("String interpolation in function") {
  constexpr auto k_input = R"(
fn greet(name: String): String {
  return "Hello, {name}!";
}
)";

  life_lang::Diagnostic_Engine diagnostics{"<test>", k_input};

  life_lang::parser::Parser parser{diagnostics};
  auto const module = parser.parse_module();
  REQUIRE(module);
  if (module) {
    CHECK(module->items.size() == 1);
  }
}
