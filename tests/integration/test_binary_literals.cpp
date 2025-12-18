#include <doctest/doctest.h>

#include "../parser/internal_rules.hpp"
#include "parser.hpp"
#include "sexp.hpp"

using life_lang::ast::to_sexp_string;
using life_lang::parser::Parser;

TEST_CASE("Binary literals in expressions") {
  SUBCASE("simple binary literal") {
    Parser parser("0b1010");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(to_sexp_string(*expr, 0) == "(integer \"0b1010\")");
  }

  SUBCASE("binary in addition") {
    Parser parser("0b1010 + 0b0101");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(to_sexp_string(*expr, 0) == "(binary + (integer \"0b1010\") (integer \"0b0101\"))");
  }

  SUBCASE("binary in comparison") {
    Parser parser("value == 0b1111");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(to_sexp_string(*expr, 0) == "(binary == (var ((var_segment \"value\"))) (integer \"0b1111\"))");
  }
}

TEST_CASE("Binary literals with underscores") {
  SUBCASE("binary with underscores") {
    Parser parser("0b1111_0000_1010_0101");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(to_sexp_string(*expr, 0) == "(integer \"0b1111000010100101\")");
  }

  SUBCASE("let with binary underscores") {
    Parser parser("let flags = 0b1111_0000;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    CHECK(to_sexp_string(*stmt, 0) == "(let false (pattern \"flags\") nil (integer \"0b11110000\"))");
  }
}

TEST_CASE("Binary literals with type suffixes") {
  SUBCASE("binary with U8 suffix") {
    Parser parser("0b11111111U8");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(to_sexp_string(*expr, 0) == "(integer \"0b11111111\" \"U8\")");
  }

  SUBCASE("binary with I32 suffix") {
    Parser parser("0b1010_1010I32");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(to_sexp_string(*expr, 0) == "(integer \"0b10101010\" \"I32\")");
  }
}

TEST_CASE("Binary literals in let statements") {
  SUBCASE("multiple let statements with binary") {
    Parser parser("let mask = 0b1111_0000; let bits = 0b1010_0101;");
    auto const stmt1 = parser.parse_statement();
    REQUIRE(stmt1.has_value());
    CHECK(to_sexp_string(*stmt1, 0) == "(let false (pattern \"mask\") nil (integer \"0b11110000\"))");

    auto const stmt2 = parser.parse_statement();
    REQUIRE(stmt2.has_value());
    CHECK(to_sexp_string(*stmt2, 0) == "(let false (pattern \"bits\") nil (integer \"0b10100101\"))");
  }
}

TEST_CASE("Binary literals in arrays") {
  SUBCASE("array of binary values") {
    Parser parser("[0b0001, 0b0010, 0b0100, 0b1000]");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(
        to_sexp_string(*expr, 0) ==
        "(array_lit ((integer \"0b0001\") (integer \"0b0010\") (integer \"0b0100\") (integer \"0b1000\")))"
    );
  }

  SUBCASE("array of binary bytes") {
    Parser parser("[0b1111_1111, 0b0000_0000, 0b1010_1010]");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(
        to_sexp_string(*expr, 0) ==
        "(array_lit ((integer \"0b11111111\") (integer \"0b00000000\") (integer \"0b10101010\")))"
    );
  }
}

TEST_CASE("Binary literals in match expressions") {
  SUBCASE("match with binary patterns") {
    Parser parser(R"(match x {
      0b0000 => true,
      0b1111 => false,
    })");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(
        to_sexp_string(*expr, 0) ==
        "(match (var ((var_segment \"x\"))) ((arm (lit_pattern (integer \"0b0000\")) nil (bool true)) (arm "
        "(lit_pattern (integer \"0b1111\")) nil (bool false))))"
    );
  }
}

TEST_CASE("Binary literals - bit masks") {
  SUBCASE("permission bits") {
    Parser parser("let read = 0b100;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    CHECK(to_sexp_string(*stmt, 0) == "(let false (pattern \"read\") nil (integer \"0b100\"))");
  }

  SUBCASE("multiple permission bits") {
    Parser parser("let write = 0b010;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    CHECK(to_sexp_string(*stmt, 0) == "(let false (pattern \"write\") nil (integer \"0b010\"))");
  }
}
