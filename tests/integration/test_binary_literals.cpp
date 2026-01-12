#include <doctest/doctest.h>

#include "../parser/internal_rules.hpp"
#include "../parser/utils.hpp"
#include "diagnostics.hpp"
#include "parser/parser.hpp"
#include "parser/sexp.hpp"

using life_lang::ast::to_sexp_string;
using namespace test_sexp;

TEST_CASE("Binary literals in expressions") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "simple binary literal", .input = "0b1010", .expected = integer("0b1010")},
      {.name = "binary in addition",
       .input = "0b1010 + 0b0101",
       .expected = binary_expr("+", integer("0b1010"), integer("0b0101"))},
      {.name = "binary in comparison",
       .input = "value == 0b1111",
       .expected = binary_expr("==", var_name("value"), integer("0b1111"))},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{tc.input});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

      life_lang::parser::Parser parser{diagnostics};
      auto const expr = parser.parse_expr();
      REQUIRE(expr.has_value());
      if (expr.has_value()) {
        CHECK(to_sexp_string(*expr, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Binary literals with underscores") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "binary with underscores", .input = "0b1111_0000_1010_0101", .expected = integer("0b1111000010100101")},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{tc.input});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

      life_lang::parser::Parser parser{diagnostics};
      auto const expr = parser.parse_expr();
      REQUIRE(expr.has_value());
      if (expr.has_value()) {
        CHECK(to_sexp_string(*expr, 0) == tc.expected);
      }
    }
  }

  SUBCASE("let with binary underscores") {
    life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{"let flags = 0b1111_0000;"});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

    life_lang::parser::Parser parser{diagnostics};
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    if (stmt.has_value()) {
      CHECK(to_sexp_string(*stmt, 0) == let_statement(simple_pattern("flags"), integer("0b11110000")));
    }
  }
}

TEST_CASE("Binary literals with type suffixes") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "binary with U8 suffix", .input = "0b11111111U8", .expected = integer("0b11111111", "U8")},
      {.name = "binary with I32 suffix", .input = "0b1010_1010I32", .expected = integer("0b10101010", "I32")},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{tc.input});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

      life_lang::parser::Parser parser{diagnostics};
      auto const expr = parser.parse_expr();
      REQUIRE(expr.has_value());
      if (expr.has_value()) {
        CHECK(to_sexp_string(*expr, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Binary literals in let statements") {
  SUBCASE("multiple let statements with binary") {
    life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{"let mask = 0b1111_0000; let bits = 0b1010_0101;"});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

    life_lang::parser::Parser parser{diagnostics};
    auto const stmt1 = parser.parse_statement();
    REQUIRE(stmt1.has_value());
    if (stmt1.has_value()) {
      CHECK(to_sexp_string(*stmt1, 0) == let_statement(simple_pattern("mask"), integer("0b11110000")));
    }

    auto const stmt2 = parser.parse_statement();
    REQUIRE(stmt2.has_value());
    if (stmt2.has_value()) {
      CHECK(to_sexp_string(*stmt2, 0) == let_statement(simple_pattern("bits"), integer("0b10100101")));
    }
  }
}

TEST_CASE("Binary literals in arrays") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "array of binary values",
       .input = "[0b0001, 0b0010, 0b0100, 0b1000]",
       .expected = array_literal({integer("0b0001"), integer("0b0010"), integer("0b0100"), integer("0b1000")})},
      {.name = "array of binary bytes",
       .input = "[0b1111_1111, 0b0000_0000, 0b1010_1010]",
       .expected = array_literal({integer("0b11111111"), integer("0b00000000"), integer("0b10101010")})},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{tc.input});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

      life_lang::parser::Parser parser{diagnostics};
      auto const expr = parser.parse_expr();
      REQUIRE(expr.has_value());
      if (expr.has_value()) {
        CHECK(to_sexp_string(*expr, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Binary literals in match expressions") {
  SUBCASE("match with binary patterns") {
    constexpr std::string_view k_source = R"(match x {
      0b0000 => true,
      0b1111 => false,
    })";
    life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{k_source});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};
    life_lang::parser::Parser parser{diagnostics};
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const expected = match_expr(
          var_name("x"),
          {match_arm(literal_pattern(integer("0b0000")), bool_literal(true)),
           match_arm(literal_pattern(integer("0b1111")), bool_literal(false))}
      );
      CHECK(to_sexp_string(*expr, 0) == expected);
    }
  }
}

TEST_CASE("Binary literals - bit masks") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "permission bits",
       .input = "let read = 0b100;",
       .expected = let_statement(simple_pattern("read"), integer("0b100"))},
      {.name = "multiple permission bits",
       .input = "let write = 0b010;",
       .expected = let_statement(simple_pattern("write"), integer("0b010"))},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{tc.input});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

      life_lang::parser::Parser parser{diagnostics};
      auto const stmt = parser.parse_statement();
      REQUIRE(stmt.has_value());
      if (stmt.has_value()) {
        CHECK(to_sexp_string(*stmt, 0) == tc.expected);
      }
    }
  }
}
