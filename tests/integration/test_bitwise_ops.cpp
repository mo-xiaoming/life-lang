#include <doctest/doctest.h>
#include "parser.hpp"
#include "sexp.hpp"

TEST_CASE("Bitwise AND in expressions") {
  SUBCASE("simple bitwise AND") {
    life_lang::parser::Parser parser("flags & mask");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(
        life_lang::ast::to_sexp_string(*expr, 0) ==
        "(binary & (var ((var_segment \"flags\"))) (var ((var_segment \"mask\"))))"
    );
  }

  SUBCASE("bitwise AND with hex literals") {
    life_lang::parser::Parser parser("0xFF & 0x0F");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(binary & (integer \"0xFF\") (integer \"0x0F\"))");
  }

  SUBCASE("bitwise AND in let statement") {
    life_lang::parser::Parser parser("let result = value & 0xFF;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    CHECK(
        life_lang::ast::to_sexp_string(*stmt, 0) ==
        "(let false (pattern \"result\") nil (binary & (var ((var_segment \"value\"))) (integer \"0xFF\")))"
    );
  }
}

TEST_CASE("Bitwise OR in expressions") {
  SUBCASE("simple bitwise OR") {
    life_lang::parser::Parser parser("a | b");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(
        life_lang::ast::to_sexp_string(*expr, 0) == "(binary | (var ((var_segment \"a\"))) (var ((var_segment \"b\"))))"
    );
  }

  SUBCASE("bitwise OR with binary literals") {
    life_lang::parser::Parser parser("0b0001 | 0b0010 | 0b0100");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(
        life_lang::ast::to_sexp_string(*expr, 0) ==
        "(binary | (binary | (integer \"0b0001\") (integer \"0b0010\")) (integer \"0b0100\"))"
    );
  }

  SUBCASE("permission flags combination") {
    life_lang::parser::Parser parser("let perms = READ | WRITE | EXECUTE;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    auto const sexp = life_lang::ast::to_sexp_string(*stmt, 0);
    CHECK(
        sexp ==
        "(let false (pattern \"perms\") nil (binary | (binary | (var ((var_segment \"READ\"))) (var ((var_segment "
        "\"WRITE\")))) (var ((var_segment \"EXECUTE\")))))"
    );
  }
}

TEST_CASE("Bitwise XOR in expressions") {
  SUBCASE("simple bitwise XOR") {
    life_lang::parser::Parser parser("a ^ b");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(
        life_lang::ast::to_sexp_string(*expr, 0) == "(binary ^ (var ((var_segment \"a\"))) (var ((var_segment \"b\"))))"
    );
  }

  SUBCASE("XOR with hex literals") {
    life_lang::parser::Parser parser("0xFF ^ 0xAA");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(binary ^ (integer \"0xFF\") (integer \"0xAA\"))");
  }
}

TEST_CASE("Shift left operations") {
  SUBCASE("simple shift left") {
    life_lang::parser::Parser parser("value << 2");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(binary << (var ((var_segment \"value\"))) (integer \"2\"))");
  }

  SUBCASE("shift left with literal") {
    life_lang::parser::Parser parser("1 << 8");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(binary << (integer \"1\") (integer \"8\"))");
  }

  SUBCASE("shift left in let statement") {
    life_lang::parser::Parser parser("let shifted = bits << count;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    CHECK(
        life_lang::ast::to_sexp_string(*stmt, 0) ==
        "(let false (pattern \"shifted\") nil (binary << (var ((var_segment \"bits\"))) (var ((var_segment "
        "\"count\")))))"
    );
  }
}

TEST_CASE("Shift right operations") {
  SUBCASE("simple shift right") {
    life_lang::parser::Parser parser("value >> 4");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(binary >> (var ((var_segment \"value\"))) (integer \"4\"))");
  }

  SUBCASE("shift right with hex") {
    life_lang::parser::Parser parser("0xFF00 >> 8");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(binary >> (integer \"0xFF00\") (integer \"8\"))");
  }
}

TEST_CASE("Complex bitwise expressions") {
  SUBCASE("mask and shift") {
    life_lang::parser::Parser parser("(value & 0xFF) << 8");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(
        life_lang::ast::to_sexp_string(*expr, 0) ==
        "(binary << (binary & (var ((var_segment \"value\"))) (integer \"0xFF\")) (integer \"8\"))"
    );
  }

  SUBCASE("combine bytes") {
    life_lang::parser::Parser parser("(high << 8) | low");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(
        life_lang::ast::to_sexp_string(*expr, 0) ==
        "(binary | (binary << (var ((var_segment \"high\"))) (integer \"8\")) (var ((var_segment \"low\"))))"
    );
  }

  SUBCASE("RGB color composition") {
    life_lang::parser::Parser parser("(r << 16) | (g << 8) | b");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(
        life_lang::ast::to_sexp_string(*expr, 0) ==
        "(binary | (binary | (binary << (var ((var_segment \"r\"))) (integer \"16\")) (binary << (var ((var_segment "
        "\"g\"))) (integer \"8\"))) (var ((var_segment \"b\"))))"
    );
  }

  SUBCASE("extract byte from word") {
    life_lang::parser::Parser parser("(word >> 8) & 0xFF");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(
        life_lang::ast::to_sexp_string(*expr, 0) ==
        "(binary & (binary >> (var ((var_segment \"word\"))) (integer \"8\")) (integer \"0xFF\"))"
    );
  }
}

TEST_CASE("Bitwise operators in match expressions") {
  SUBCASE("match with bitwise pattern") {
    life_lang::parser::Parser parser(R"(match flags & 0x07 {
      0 => none,
      1 => read,
      2 => write,
    })");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
    CHECK(sexp.find("(binary & (var ((var_segment \"flags\"))) (integer \"0x07\"))") != std::string::npos);
  }
}

TEST_CASE("Bitwise operators with binary literals") {
  SUBCASE("bit flags using binary literals") {
    life_lang::parser::Parser parser("let flags = 0b0001 | 0b0100 | 0b1000;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    CHECK(
        life_lang::ast::to_sexp_string(*stmt, 0) ==
        "(let false (pattern \"flags\") nil (binary | (binary | (integer \"0b0001\") (integer \"0b0100\")) (integer "
        "\"0b1000\")))"
    );
  }

  SUBCASE("masking with binary literals") {
    life_lang::parser::Parser parser("value & 0b11110000");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    CHECK(
        life_lang::ast::to_sexp_string(*expr, 0) ==
        "(binary & (var ((var_segment \"value\"))) (integer \"0b11110000\"))"
    );
  }
}
