#include <doctest/doctest.h>
#include "diagnostics.hpp"
#include "parser.hpp"
#include "sexp.hpp"
#include "utils.hpp"

using namespace life_lang;
using namespace life_lang::ast;
using namespace life_lang::parser;
using namespace test_sexp;

TEST_CASE("Unbounded range expressions") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "unbounded end (a..)", .input = "10..", .expected = range_expr(integer("10"), "nil", false)},
      {.name = "unbounded start (..b)", .input = "..100", .expected = range_expr("nil", integer("100"), false)},
      {.name = "unbounded start inclusive (..=b)",
       .input = "..=100",
       .expected = range_expr("nil", integer("100"), true)},
      {.name = "fully unbounded (..)", .input = "..", .expected = range_expr("nil", "nil", false)},
      {.name = "with variable start", .input = "x..", .expected = range_expr(var_name("x"), "nil", false)},
      {.name = "with variable end", .input = "..y", .expected = range_expr("nil", var_name("y"), false)},
      {.name = "with expression start",
       .input = "x + 1..",
       .expected = range_expr(binary_expr("+", var_name("x"), integer("1")), "nil", false)},
      {.name = "with expression end",
       .input = "..y - 1",
       .expected = range_expr("nil", binary_expr("-", var_name("y"), integer("1")), false)},
      {.name = "bounded range (a..b)", .input = "1..10", .expected = range_expr(integer("1"), integer("10"), false)},
      {.name = "bounded range inclusive (a..=b)",
       .input = "1..=10",
       .expected = range_expr(integer("1"), integer("10"), true)},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      life_lang::Diagnostic_Engine diagnostics{"<test>", test.input};

      life_lang::parser::Parser parser{diagnostics};
      auto const expr = parser.parse_expr();
      REQUIRE(expr.has_value());
      if (expr.has_value()) {
        CHECK(to_sexp_string(*expr, 0) == test.expected);
      }
    }
  }
}

TEST_CASE("Unbounded range - unbounded end inclusive (a..=) - not valid syntax") {
  // Note: a..= doesn't make sense semantically, but parser may accept it
  // Semantic analysis should reject it
  life_lang::Diagnostic_Engine diagnostics{"<test>", "10..="};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  // Parser should handle it, semantic analysis will validate
  REQUIRE(expr.has_value());
}

TEST_CASE("Unbounded range in context") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "in for loop",
       .input = "for i in 0.. { }",
       .expected = for_expr(simple_pattern("i"), range_expr(integer("0"), "nil", false), block({}))},
  };

  for (auto const& test: k_test_cases) {
    SUBCASE(test.name) {
      life_lang::Diagnostic_Engine diagnostics{"<test>", test.input};

      life_lang::parser::Parser parser{diagnostics};
      auto const expr = parser.parse_expr();
      REQUIRE(expr.has_value());
      if (expr.has_value()) {
        CHECK(to_sexp_string(*expr, 0) == test.expected);
      }
    }
  }
}

TEST_CASE("Unbounded range in statements") {
  SUBCASE("in let statement") {
    constexpr auto* k_input = "let range = ..100;";
    life_lang::Diagnostic_Engine diagnostics{"<test>", k_input};

    life_lang::parser::Parser parser{diagnostics};
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    if (stmt.has_value()) {
      auto const expected = let_statement(simple_pattern("range"), range_expr("nil", integer("100"), false));
      CHECK(to_sexp_string(*stmt, 0) == expected);
    }
  }
}
