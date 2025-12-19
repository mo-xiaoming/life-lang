#include <doctest/doctest.h>
#include "parser.hpp"
#include "sexp.hpp"

TEST_CASE("Boolean literals in expressions") {
  SUBCASE("simple true") {
    life_lang::parser::Parser parser("true");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(bool true)");
    }
  }

  SUBCASE("simple false") {
    life_lang::parser::Parser parser("false");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(bool false)");
    }
  }

  SUBCASE("boolean in binary expression") {
    life_lang::parser::Parser parser("true && false");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(binary && (bool true) (bool false))");
    }
  }

  SUBCASE("boolean with logical or") {
    life_lang::parser::Parser parser("false || true");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(binary || (bool false) (bool true))");
    }
  }

  SUBCASE("boolean with negation") {
    life_lang::parser::Parser parser("!true");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(unary ! (bool true))");
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
          "(binary || (binary && (unary ! (bool false)) (bool true)) (bool false))"
      );
    }
  }
}

TEST_CASE("Boolean literals in let statements") {
  SUBCASE("let with true") {
    life_lang::parser::Parser parser("let flag = true;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    if (stmt.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*stmt, 0);
      CHECK(sexp == "(let false (pattern \"flag\") nil (bool true))");
    }
  }

  SUBCASE("let with false") {
    life_lang::parser::Parser parser("let enabled = false;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    if (stmt.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*stmt, 0);
      CHECK(sexp == "(let false (pattern \"enabled\") nil (bool false))");
    }
  }

  SUBCASE("let with type annotation") {
    life_lang::parser::Parser parser("let ready: Bool = true;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    if (stmt.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*stmt, 0);
      CHECK(sexp == "(let false (pattern \"ready\") (path ((type_segment \"Bool\"))) (bool true))");
    }
  }
}

TEST_CASE("Boolean literals in function calls") {
  SUBCASE("function call with boolean argument") {
    life_lang::parser::Parser parser("set_flag(true)");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(call (var ((var_segment \"set_flag\"))) ((bool true)))");
    }
  }

  SUBCASE("function call with multiple boolean arguments") {
    life_lang::parser::Parser parser("compare(true, false)");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(call (var ((var_segment \"compare\"))) ((bool true) (bool false)))");
    }
  }
}

TEST_CASE("Boolean literals in if expressions") {
  SUBCASE("if with boolean condition") {
    life_lang::parser::Parser parser("if true { return 1; }");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(if (bool true) (block ((return (integer \"1\")))))");
    }
  }

  SUBCASE("if with boolean expression") {
    life_lang::parser::Parser parser("if true && false { return 0; }");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(if (binary && (bool true) (bool false)) (block ((return (integer \"0\")))))");
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
          sexp ==
          "(match (var ((var_segment \"value\"))) ((arm (lit_pattern (bool true)) nil (integer \"1\")) (arm "
          "(lit_pattern "
          "(bool false)) nil (integer \"0\"))))"
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
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(array_lit ((bool true) (bool false) (bool true)))");
    }
  }
}

TEST_CASE("Boolean literals in tuples") {
  SUBCASE("tuple with booleans") {
    life_lang::parser::Parser parser("(true, false)");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(tuple_lit ((bool true) (bool false)))");
    }
  }

  SUBCASE("tuple with mixed types including booleans") {
    life_lang::parser::Parser parser("(42, true, \"hello\", false)");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(tuple_lit ((integer \"42\") (bool true) (string \"\\\"hello\\\"\") (bool false)))");
    }
  }
}

TEST_CASE("Boolean literals in struct literals") {
  SUBCASE("struct with boolean field") {
    life_lang::parser::Parser parser("Config { enabled: true, debug: false }");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(
          sexp == "(struct_lit \"Config\" ((field_init \"enabled\" (bool true)) (field_init \"debug\" (bool false))))"
      );
    }
  }
}

TEST_CASE("Boolean literals vs identifiers") {
  SUBCASE("true is not an identifier") {
    // When parsing as expression, true should be boolean literal
    life_lang::parser::Parser parser("true");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(bool true)");
    }
  }

  SUBCASE("true_value is an identifier, not a boolean") {
    // true_value should parse as identifier
    life_lang::parser::Parser parser("true_value");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(var ((var_segment \"true_value\")))");
    }
  }

  SUBCASE("false_flag is an identifier") {
    life_lang::parser::Parser parser("false_flag");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(var ((var_segment \"false_flag\")))");
    }
  }
}

TEST_CASE("Boolean literals in return statements") {
  SUBCASE("return true") {
    life_lang::parser::Parser parser("return true;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    if (stmt.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*stmt, 0);
      CHECK(sexp == "(return (bool true))");
    }
  }

  SUBCASE("return false") {
    life_lang::parser::Parser parser("return false;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    if (stmt.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*stmt, 0);
      CHECK(sexp == "(return (bool false))");
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
    // Verify structure: function with Bool parameters, Bool return type, and boolean literal returns
    CHECK(func->declaration.name == "is_valid");
    CHECK(func->declaration.func_params.size() == 2);

    auto const sexp = life_lang::ast::to_sexp_string(*func, 0);
    // Full exact match would be too brittle for this complex case, but verify it contains proper boolean literals
    // in the return statements (not variables)
    CHECK(sexp.find("(return (bool true))") != std::string::npos);
    CHECK(sexp.find("(return (bool false))") != std::string::npos);
  }
}
