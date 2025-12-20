#include <doctest/doctest.h>
#include "../parser/utils.hpp"
#include "parser.hpp"
#include "sexp.hpp"

using namespace test_sexp;

TEST_CASE("Bitwise operators in let statements") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "bitwise AND in let statement",
       .input = "let result = value & 0xFF;",
       .expected = let_statement(simple_pattern("result"), binary_expr("&", var_name("value"), integer("0xFF")))},
      {.name = "permission flags combination",
       .input = "let perms = READ | WRITE | EXECUTE;",
       .expected = let_statement(
           simple_pattern("perms"),
           binary_expr("|", binary_expr("|", var_name("READ"), var_name("WRITE")), var_name("EXECUTE"))
       )},
      {.name = "shift left in let statement",
       .input = "let shifted = bits << count;",
       .expected = let_statement(simple_pattern("shifted"), binary_expr("<<", var_name("bits"), var_name("count")))},
      {.name = "bit flags using binary literals",
       .input = "let flags = 0b0001 | 0b0100 | 0b1000;",
       .expected = let_statement(
           simple_pattern("flags"),
           binary_expr("|", binary_expr("|", integer("0b0001"), integer("0b0100")), integer("0b1000"))
       )},
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

TEST_CASE("Bitwise AND in expressions") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "simple bitwise AND",
       .input = "flags & mask",
       .expected = binary_expr("&", var_name("flags"), var_name("mask"))},
      {.name = "bitwise AND with hex literals",
       .input = "0xFF & 0x0F",
       .expected = binary_expr("&", integer("0xFF"), integer("0x0F"))},
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

TEST_CASE("Bitwise OR in expressions") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "simple bitwise OR", .input = "a | b", .expected = binary_expr("|", var_name("a"), var_name("b"))},
      {.name = "bitwise OR with binary literals",
       .input = "0b0001 | 0b0010 | 0b0100",
       .expected = binary_expr("|", binary_expr("|", integer("0b0001"), integer("0b0010")), integer("0b0100"))},
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

TEST_CASE("Bitwise XOR in expressions") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "simple bitwise XOR", .input = "a ^ b", .expected = binary_expr("^", var_name("a"), var_name("b"))},
      {.name = "XOR with hex literals",
       .input = "0xFF ^ 0xAA",
       .expected = binary_expr("^", integer("0xFF"), integer("0xAA"))},
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

TEST_CASE("Shift left operations") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "simple shift left",
       .input = "value << 2",
       .expected = binary_expr("<<", var_name("value"), integer("2"))},
      {.name = "shift left with literal", .input = "1 << 8", .expected = binary_expr("<<", integer("1"), integer("8"))},
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

TEST_CASE("Shift right operations") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static auto const k_test_cases = std::array{
      Test_Case{
          .name = "simple shift right",
          .input = "value >> 4",
          .expected = binary_expr(">>", var_name("value"), integer("4"))
      },
      Test_Case{
          .name = "shift right with hex",
          .input = "0xFF00 >> 8",
          .expected = binary_expr(">>", integer("0xFF00"), integer("8"))
      },
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

TEST_CASE("Complex bitwise expressions") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "mask and shift",
       .input = "(value & 0xFF) << 8",
       .expected = binary_expr("<<", binary_expr("&", var_name("value"), integer("0xFF")), integer("8"))},
      {.name = "combine bytes",
       .input = "(high << 8) | low",
       .expected = binary_expr("|", binary_expr("<<", var_name("high"), integer("8")), var_name("low"))},
      {.name = "RGB color composition",
       .input = "(r << 16) | (g << 8) | b",
       .expected = binary_expr(
           "|",
           binary_expr(
               "|",
               binary_expr("<<", var_name("r"), integer("16")),
               binary_expr("<<", var_name("g"), integer("8"))
           ),
           var_name("b")
       )},
      {.name = "extract byte from word",
       .input = "(word >> 8) & 0xFF",
       .expected = binary_expr("&", binary_expr(">>", var_name("word"), integer("8")), integer("0xFF"))},
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

TEST_CASE("Bitwise operators in match expressions") {
  SUBCASE("match with bitwise pattern") {
    life_lang::parser::Parser parser(R"(match flags & 0x07 {
      0 => none,
      1 => read,
      2 => write,
    })");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const expected = match_expr(
          binary_expr("&", var_name("flags"), integer("0x07")),
          {match_arm(literal_pattern(integer("0")), var_name("none")),
           match_arm(literal_pattern(integer("1")), var_name("read")),
           match_arm(literal_pattern(integer("2")), var_name("write"))}
      );
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == expected);
    }
  }
}

TEST_CASE("Bitwise operators with binary literals") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "masking with binary literals",
       .input = "value & 0b11110000",
       .expected = binary_expr("&", var_name("value"), integer("0b11110000"))},
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
