#include <doctest/doctest.h>
#include "parser.hpp"
#include "sexp.hpp"

TEST_CASE("Hexadecimal literals in expressions") {
  SUBCASE("simple hex literal") {
    life_lang::parser::Parser parser("0xFF");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(integer \"0xFF\")");
    }
  }

  SUBCASE("hex in binary expression") {
    life_lang::parser::Parser parser("0xFF + 0x10");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(binary + (integer \"0xFF\") (integer \"0x10\"))");
    }
  }

  SUBCASE("hex in comparison") {
    life_lang::parser::Parser parser("value == 0xDEAD");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(
          life_lang::ast::to_sexp_string(*expr, 0) == "(binary == (var ((var_segment \"value\"))) (integer \"0xDEAD\"))"
      );
    }
  }
}

TEST_CASE("Hexadecimal literals in let statements") {
  SUBCASE("let with hex value") {
    life_lang::parser::Parser parser("let flags = 0xFF;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    if (stmt.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*stmt, 0);
      CHECK(sexp == "(let false (pattern \"flags\") nil (integer \"0xFF\"))");
    }
  }

  SUBCASE("let with hex and type annotation") {
    life_lang::parser::Parser parser("let color: U32 = 0xDEAD_BEEF;");
    auto const stmt = parser.parse_statement();
    REQUIRE(stmt.has_value());
    if (stmt.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*stmt, 0);
      CHECK(sexp == "(let false (pattern \"color\") (path ((type_segment \"U32\"))) (integer \"0xDEADBEEF\"))");
    }
  }
}

TEST_CASE("Hexadecimal literals in arrays") {
  SUBCASE("array of hex values") {
    life_lang::parser::Parser parser("[0x00, 0xFF, 0x7F]");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(array_lit ((integer \"0x00\") (integer \"0xFF\") (integer \"0x7F\")))");
    }
  }

  SUBCASE("color palette array") {
    life_lang::parser::Parser parser("[0xFF0000, 0x00FF00, 0x0000FF]");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(array_lit ((integer \"0xFF0000\") (integer \"0x00FF00\") (integer \"0x0000FF\")))");
    }
  }
}

TEST_CASE("Hexadecimal literals with type suffixes") {
  SUBCASE("hex U8") {
    life_lang::parser::Parser parser("0xFFU8");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(integer \"0xFF\" \"U8\")");
    }
  }

  SUBCASE("hex U32") {
    life_lang::parser::Parser parser("0xDEADBEEFU32");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(integer \"0xDEADBEEF\" \"U32\")");
    }
  }

  SUBCASE("hex I64") {
    life_lang::parser::Parser parser("0x7FFF_FFFF_FFFF_FFFFI64");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == "(integer \"0x7FFFFFFFFFFFFFFF\" \"I64\")");
    }
  }
}

TEST_CASE("Hexadecimal in function calls") {
  SUBCASE("function call with hex argument") {
    life_lang::parser::Parser parser("set_color(0xFF00FF)");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(call (var ((var_segment \"set_color\"))) ((integer \"0xFF00FF\")))");
    }
  }

  SUBCASE("multiple hex arguments") {
    life_lang::parser::Parser parser("create_rgb(0xFF, 0x80, 0x00)");
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(
          sexp ==
          "(call (var ((var_segment \"create_rgb\"))) ((integer \"0xFF\") (integer \"0x80\") (integer \"0x00\")))"
      );
    }
  }
}

TEST_CASE("Hexadecimal in match expressions") {
  SUBCASE("match with hex patterns") {
    auto const* input = R"(
      match status {
        0x00 => 1,
        0xFF => 2,
      }
    )";
    life_lang::parser::Parser parser(input);
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(
          sexp ==
          "(match (var ((var_segment \"status\"))) ((arm (lit_pattern (integer \"0x00\")) nil (integer \"1\")) (arm "
          "(lit_pattern (integer \"0xFF\")) nil (integer \"2\"))))"
      );
    }
  }
}

TEST_CASE("Complete function with hexadecimal literals") {
  auto const* input = R"(
    fn check_flags(value: U32): Bool {
      let mask = 0xFF00;
      return true;
    }
  )";

  life_lang::parser::Parser parser(input);
  auto const func = parser.parse_func_def();
  REQUIRE(func.has_value());
  if (func.has_value()) {
    CHECK(func->declaration.name == "check_flags");
    CHECK(func->declaration.func_params.size() == 1);

    auto const sexp = life_lang::ast::to_sexp_string(*func, 0);
    // Verify complete function structure with hex literal in let statement
    CHECK(
        sexp ==
        "(func_def false (func_decl \"check_flags\" () ((param false \"value\" (path ((type_segment \"U32\"))))) (path "
        "((type_segment \"Bool\")))) (block ((let false (pattern \"mask\") nil (integer \"0xFF00\")) (return (bool "
        "true)))))"
    );
  }
}
