#include <doctest/doctest.h>
#include "../parser/utils.hpp"
#include "parser.hpp"
#include "sexp.hpp"

using life_lang::ast::to_sexp_string;
using life_lang::parser::Parser;
using namespace test_sexp;
using life_lang::parser::Parser;

TEST_CASE("Octal literals in expressions") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "simple octal literal", .input = "0o755", .expected = integer("0o755")},
      {.name = "octal in binary expression",
       .input = "0o10 + 0o20",
       .expected = binary_expr("+", integer("0o10"), integer("0o20"))},
      {.name = "octal in comparison",
       .input = "perms == 0o644",
       .expected = binary_expr("==", var_name("perms"), integer("0o644"))},
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

TEST_CASE("Octal literals with underscores") {
  SUBCASE("octal with underscores") {
    Parser parser("0o7_7_7");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(to_sexp_string(*expr, 0) == integer("0o777"));
    }
  }

  SUBCASE("let with octal underscores") {
    Parser parser("let perms = 0o7_5_5;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    if (stmt.has_value()) {
      CHECK(to_sexp_string(*stmt, 0) == let_statement(simple_pattern("perms"), integer("0o755")));
    }
  }
}

TEST_CASE("Octal literals with type suffixes") {
  SUBCASE("octal with U16 suffix") {
    Parser parser("0o644U16");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(to_sexp_string(*expr, 0) == integer("0o644", "U16"));
    }
  }

  SUBCASE("octal with I32 suffix") {
    Parser parser("0o755I32");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(to_sexp_string(*expr, 0) == integer("0o755", "I32"));
    }
  }
}

TEST_CASE("Octal literals in let statements") {
  SUBCASE("let with octal value") {
    Parser parser("let mode = 0o755;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    if (stmt.has_value()) {
      CHECK(to_sexp_string(*stmt, 0) == let_statement(simple_pattern("mode"), integer("0o755")));
    }
  }

  SUBCASE("multiple let statements with octal (Unix permissions)") {
    Parser parser("let rwx = 0o755; let rw = 0o644;");
    auto const stmt1 = parser.parse_statement();
    REQUIRE(stmt1.has_value());
    if (stmt1.has_value()) {
      CHECK(to_sexp_string(*stmt1, 0) == let_statement(simple_pattern("rwx"), integer("0o755")));
    }

    auto const stmt2 = parser.parse_statement();
    REQUIRE(stmt2.has_value());
    if (stmt2.has_value()) {
      CHECK(to_sexp_string(*stmt2, 0) == let_statement(simple_pattern("rw"), integer("0o644")));
    }
  }

  SUBCASE("let with octal and type annotation") {
    Parser parser("let perms: U16 = 0o644;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    if (stmt.has_value()) {
      auto const expected = let_statement(simple_pattern("perms"), integer("0o644"), false, type_name("U16"));
      CHECK(to_sexp_string(*stmt, 0) == expected);
    }
  }
}

TEST_CASE("Octal literals in arrays") {
  SUBCASE("array of octal values (Unix permissions)") {
    Parser parser("[0o755, 0o644, 0o444]");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      if (expr.has_value()) {
        CHECK(to_sexp_string(*expr, 0) == array_literal({integer("0o755"), integer("0o644"), integer("0o444")}));
      }
    }
  }

  SUBCASE("octal permissions in let array") {
    Parser parser("let modes = [0o777, 0o666, 0o555];");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    if (stmt.has_value()) {
      auto const expected =
          let_statement(simple_pattern("modes"), array_literal({integer("0o777"), integer("0o666"), integer("0o555")}));
      CHECK(to_sexp_string(*stmt, 0) == expected);
    }
  }
}

TEST_CASE("Octal uppercase O prefix") {
  SUBCASE("uppercase O in octal literal") {
    Parser parser("0O777");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      if (expr.has_value()) {
        CHECK(to_sexp_string(*expr, 0) == integer("0o777"));
      }
    }
  }
}

TEST_CASE("Mixed number bases") {
  SUBCASE("decimal, hex, octal, and binary in one expression") {
    Parser parser("100 + 0xFF + 0o77 + 0b11");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      if (expr.has_value()) {
        auto const expected = binary_expr(
            "+",
            binary_expr("+", binary_expr("+", integer("100"), integer("0xFF")), integer("0o77")),
            integer("0b11")
        );
        CHECK(to_sexp_string(*expr, 0) == expected);
      }
    }
  }

  SUBCASE("let statements with all number bases") {
    Parser parser("let dec = 100; let hex = 0xFF; let oct = 0o77; let bin = 0b11;");
    auto const stmt1 = parser.parse_statement();
    REQUIRE(stmt1.has_value());
    if (stmt1.has_value()) {
      CHECK(to_sexp_string(*stmt1, 0) == let_statement(simple_pattern("dec"), integer("100")));
    }

    auto const stmt2 = parser.parse_statement();
    REQUIRE(stmt2.has_value());
    if (stmt2.has_value()) {
      CHECK(to_sexp_string(*stmt2, 0) == let_statement(simple_pattern("hex"), integer("0xFF")));
    }

    auto const stmt3 = parser.parse_statement();
    REQUIRE(stmt3.has_value());
    if (stmt3.has_value()) {
      CHECK(to_sexp_string(*stmt3, 0) == let_statement(simple_pattern("oct"), integer("0o77")));
    }

    auto const stmt4 = parser.parse_statement();
    REQUIRE(stmt4.has_value());
    if (stmt4.has_value()) {
      CHECK(to_sexp_string(*stmt4, 0) == let_statement(simple_pattern("bin"), integer("0b11")));
    }
  }
}
