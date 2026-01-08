#include <doctest/doctest.h>
#include "../parser/utils.hpp"
#include "diagnostics.hpp"
#include "parser/parser.hpp"
#include "parser/sexp.hpp"

using namespace test_sexp;

TEST_CASE("Hexadecimal literals in expressions") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "simple hex literal", .input = "0xFF", .expected = integer("0xFF")},
      {.name = "hex in binary expression",
       .input = "0xFF + 0x10",
       .expected = binary_expr("+", integer("0xFF"), integer("0x10"))},
      {.name = "hex in comparison",
       .input = "value == 0xDEAD",
       .expected = binary_expr("==", var_name("value"), integer("0xDEAD"))},
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
        CHECK(life_lang::ast::to_sexp_string(*expr, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Hexadecimal literals in let statements") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "let with hex value",
       .input = "let flags = 0xFF;",
       .expected = let_statement(simple_pattern("flags"), integer("0xFF"))},
      {.name = "let with hex and type annotation",
       .input = "let color: U32 = 0xDEAD_BEEF;",
       .expected = let_statement(simple_pattern("color"), integer("0xDEADBEEF"), false, type_name("U32"))},
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
        CHECK(life_lang::ast::to_sexp_string(*stmt, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Hexadecimal literals in arrays") {
  SUBCASE("array of hex values") {
    life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{"[0x00, 0xFF, 0x7F]"});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};
    life_lang::parser::Parser parser{diagnostics};
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(array_lit ((integer \"0x00\") (integer \"0xFF\") (integer \"0x7F\")))");
    }
  }

  SUBCASE("color palette array") {
    life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{"[0xFF0000, 0x00FF00, 0x0000FF]"});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};
    life_lang::parser::Parser parser{diagnostics};
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
    life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{"0xFFU8"});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};
    life_lang::parser::Parser parser{diagnostics};
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == integer("0xFF", "U8"));
    }
  }

  SUBCASE("hex U32") {
    life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{"0xDEADBEEFU32"});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};
    life_lang::parser::Parser parser{diagnostics};
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == integer("0xDEADBEEF", "U32"));
    }
  }

  SUBCASE("hex I64") {
    life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{"0x7FFF_FFFF_FFFF_FFFFI64"});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};
    life_lang::parser::Parser parser{diagnostics};
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      CHECK(life_lang::ast::to_sexp_string(*expr, 0) == integer("0x7FFFFFFFFFFFFFFF", "I64"));
    }
  }
}

TEST_CASE("Hexadecimal in function calls") {
  SUBCASE("function call with hex argument") {
    life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{"set_color(0xFF00FF)"});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};
    life_lang::parser::Parser parser{diagnostics};
    auto const expr = parser.parse_expr();
    REQUIRE(expr.has_value());
    if (expr.has_value()) {
      auto const sexp = life_lang::ast::to_sexp_string(*expr, 0);
      CHECK(sexp == "(call (var ((var_segment \"set_color\"))) ((integer \"0xFF00FF\")))");
    }
  }

  SUBCASE("multiple hex arguments") {
    life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{"create_rgb(0xFF, 0x80, 0x00)"});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};
    life_lang::parser::Parser parser{diagnostics};
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
    life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{input});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};
    life_lang::parser::Parser parser{diagnostics};
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

  life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{input});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};
  life_lang::parser::Parser parser{diagnostics};
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
