#include <doctest/doctest.h>
#include "../parser/utils.hpp"
#include "parser.hpp"
#include "sexp.hpp"

using namespace test_sexp;

TEST_CASE("Boolean literals in expressions") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "simple true", .input = "true", .expected = bool_literal(true)},
      {.name = "simple false", .input = "false", .expected = bool_literal(false)},
      {.name = "boolean in binary expression",
       .input = "true && false",
       .expected = binary_expr("&&", bool_literal(true), bool_literal(false))},
      {.name = "boolean with logical or",
       .input = "false || true",
       .expected = binary_expr("||", bool_literal(false), bool_literal(true))},
      {.name = "boolean with negation", .input = "!true", .expected = unary_expr("!", bool_literal(true))},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      life_lang::parser::Parser parser(tc.input);
      auto const expr = parser.parse_expr();
      REQUIRE(expr.has_value());
      if (expr.has_value()) {
        CHECK(life_lang::ast::to_sexp_string(*expr, 0) == tc.expected);
      }
    }
  }

  SUBCASE("complex boolean expression") {
    life_lang::parser::Parser parser("!false && true || false");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      // Should parse as: (!false && true) || false
      CHECK(
          life_lang::ast::to_sexp_string(*expr, 0) ==
          binary_expr(
              "||",
              binary_expr("&&", unary_expr("!", bool_literal(false)), bool_literal(true)),
              bool_literal(false)
          )
      );
    }
  }
}

TEST_CASE("Boolean literals in let statements") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "let with true",
       .input = "let flag = true;",
       .expected = let_statement(simple_pattern("flag"), bool_literal(true))},
      {.name = "let with false",
       .input = "let enabled = false;",
       .expected = let_statement(simple_pattern("enabled"), bool_literal(false))},
      {.name = "let with type annotation",
       .input = "let ready: Bool = true;",
       .expected = let_statement(simple_pattern("ready"), bool_literal(true), false, type_name("Bool"))},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      life_lang::parser::Parser parser(tc.input);
      auto const stmt = parser.parse_statement();
      REQUIRE(stmt.has_value());
      if (stmt.has_value()) {
        CHECK(life_lang::ast::to_sexp_string(*stmt, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Boolean literals in function calls") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "function call with boolean argument",
       .input = "set_flag(true)",
       .expected = function_call(var_name("set_flag"), {bool_literal(true)})},
      {.name = "function call with multiple boolean arguments",
       .input = "compare(true, false)",
       .expected = function_call(var_name("compare"), {bool_literal(true), bool_literal(false)})},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      life_lang::parser::Parser parser(tc.input);
      auto const expr = parser.parse_expr();
      REQUIRE(expr.has_value());
      if (expr.has_value()) {
        CHECK(life_lang::ast::to_sexp_string(*expr, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Boolean literals in if expressions") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "if with boolean condition",
       .input = "if true { return 1; }",
       .expected = if_expr(bool_literal(true), block({return_statement(integer("1"))}))},
      {.name = "if with boolean expression",
       .input = "if true && false { return 0; }",
       .expected = if_expr(
           binary_expr("&&", bool_literal(true), bool_literal(false)),
           block({return_statement(integer("0"))})
       )},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      life_lang::parser::Parser parser(tc.input);
      auto const expr = parser.parse_expr();
      REQUIRE(expr.has_value());
      if (expr.has_value()) {
        CHECK(life_lang::ast::to_sexp_string(*expr, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Boolean literals in match expressions") {
  SUBCASE("match with boolean patterns") {
    auto const* input = R"(
      match value {
        true => 1,
        false => 0,
      }
    )";
    life_lang::parser::Parser parser(input);
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(
          sexp == match_expr(
                      var_name("value"),
                      {match_arm(literal_pattern(bool_literal(true)), integer("1")),
                       match_arm(literal_pattern(bool_literal(false)), integer("0"))}
                  )
      );
    }
  }
}

TEST_CASE("Boolean literals in arrays") {
  SUBCASE("array of booleans") {
    life_lang::parser::Parser parser("[true, false, true]");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(
          life_lang::ast::to_sexp_string(*expr, 0) ==
          array_literal({bool_literal(true), bool_literal(false), bool_literal(true)})
      );
    }
  }
}

TEST_CASE("Boolean literals in tuples") {
  SUBCASE("tuple with booleans") {
    life_lang::parser::Parser parser("(true, false)");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == tuple_literal({bool_literal(true), bool_literal(false)}));
    }
  }

  SUBCASE("tuple with mixed types including booleans") {
    life_lang::parser::Parser parser("(42, true, \"hello\", false)");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(
          life_lang::ast::to_sexp_string(*expr, 0) ==
          tuple_literal({integer("42"), bool_literal(true), string("\"hello\""), bool_literal(false)})
      );
    }
  }
}

TEST_CASE("Boolean literals in struct literals") {
  SUBCASE("struct with boolean field") {
    life_lang::parser::Parser parser("Config { enabled: true, debug: false }");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(
          life_lang::ast::to_sexp_string(*expr, 0) ==
          struct_literal(
              "Config",
              {field_init("enabled", bool_literal(true)), field_init("debug", bool_literal(false))}
          )
      );
    }
  }
}

TEST_CASE("Boolean literals vs identifiers") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "true is not an identifier", .input = "true", .expected = bool_literal(true)},
      {.name = "true_value is an identifier, not a boolean", .input = "true_value", .expected = var_name("true_value")},
      {.name = "false_flag is an identifier", .input = "false_flag", .expected = var_name("false_flag")},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      life_lang::parser::Parser parser(tc.input);
      auto const expr = parser.parse_expr();
      REQUIRE(expr.has_value());
      if (expr.has_value()) {
        CHECK(life_lang::ast::to_sexp_string(*expr, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Boolean literals in return statements") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "return true", .input = "return true;", .expected = return_statement(bool_literal(true))},
      {.name = "return false", .input = "return false;", .expected = return_statement(bool_literal(false))},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      life_lang::parser::Parser parser(tc.input);
      auto const stmt = parser.parse_statement();
      REQUIRE(stmt.has_value());
      if (stmt.has_value()) {
        CHECK(life_lang::ast::to_sexp_string(*stmt, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Complete function with boolean literals") {
  auto const* input = R"(
    fn is_valid(enabled: Bool, ready: Bool): Bool {
      if enabled && ready {
        return true;
      } else {
        return false;
      }
    }
  )";

  life_lang::parser::Parser parser(input);
  auto const func = parser.parse_func_def();
  REQUIRE(func.has_value());
  if (func.has_value()) {
    // Verify structure
    CHECK(func->declaration.name == "is_valid");
    CHECK(func->declaration.func_params.size() == 2);

    // Verify return type is Bool
    auto const return_type_sexp = life_lang::ast::to_sexp_string(func->declaration.return_type, 0);
    CHECK(return_type_sexp == type_name("Bool"));

    // Verify function body contains if-expression with boolean literals
    auto const& body = func->body;
    CHECK(body.statements.size() == 1);
    if (body.statements.size() == 1) {
      auto const stmt_sexp = life_lang::ast::to_sexp_string(body.statements[0], 0);
      // Expected: if-statement wrapping if-expression with true/false returns
      auto const expected = if_statement(if_else_expr(
          binary_expr("&&", var_name("enabled"), var_name("ready")),
          block({return_statement(bool_literal(true))}),
          block({return_statement(bool_literal(false))})
      ));
      CHECK(stmt_sexp == expected);
    }
  }
}
