#include <doctest/doctest.h>
#include "../parser/utils.hpp"
#include "parser.hpp"
#include "sexp.hpp"

using namespace life_lang::parser;
using namespace life_lang::ast;
using namespace test_sexp;

TEST_CASE("Tuple types in function signatures") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected_return_type;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "simple tuple return type",
       .input = "fn create_point(x: I32, y: I32): (I32, I32) { return (x, y); }",
       .expected_return_type = tuple_type({type_name("I32"), type_name("I32")})},
      {.name = "tuple parameter type",
       .input = "fn swap(pair: (I32, I32)): (I32, I32) { let (a, b) = pair; return (b, a); }",
       .expected_return_type = tuple_type({type_name("I32"), type_name("I32")})},
      {.name = "nested tuples",
       .input = "fn nested_tuples(): ((I32, I32), (String, Bool)) { return ((1, 2), (\"hello\", true)); }",
       .expected_return_type = tuple_type(
           {tuple_type({type_name("I32"), type_name("I32")}), tuple_type({type_name("String"), type_name("Bool")})}
       )},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      Parser parser(tc.input);
      auto const func = parser.parse_func_def();
      REQUIRE(func.has_value());
      if (func.has_value()) {
        CHECK(to_sexp_string(func->declaration.return_type, 0) == tc.expected_return_type);
      }
    }
  }
}

TEST_CASE("Tuple literals in expressions") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "simple tuple literal", .input = "(x, y)", .expected = tuple_literal({var_name("x"), var_name("y")})},
      {.name = "nested tuple literal",
       .input = "((1, 2), (\"hello\", true))",
       .expected = tuple_literal(
           {tuple_literal({integer("1"), integer("2")}), tuple_literal({string("\"hello\""), bool_literal(true)})}
       )},
      {.name = "tuple with array literal",
       .input = "(true, [1, 2, 3, 4])",
       .expected = tuple_literal(
           {bool_literal(true), array_literal({integer("1"), integer("2"), integer("3"), integer("4")})}
       )},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      Parser parser(tc.input);
      auto const expr = parser.parse_expr();
      REQUIRE(expr.has_value());
      if (expr.has_value()) {
        CHECK(to_sexp_string(*expr, 0) == tc.expected);
      }
    }
  }
}
