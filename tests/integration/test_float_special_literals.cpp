#include <doctest/doctest.h>
#include <array>
#include "../parser/utils.hpp"
#include "diagnostics.hpp"
#include "parser.hpp"
#include "sexp.hpp"

using life_lang::ast::to_sexp_string;
using namespace test_sexp;

TEST_CASE("Float special literals - individual expressions") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static auto const k_test_cases = std::array{
      Test_Case{.name = "nan literal", .input = "nan", .expected = float_literal("nan")},
      Test_Case{.name = "inf literal", .input = "inf", .expected = float_literal("inf")},
      Test_Case{.name = "negative inf", .input = "-inf", .expected = unary_expr("-", float_literal("inf"))},
      Test_Case{.name = "nan with F32 suffix", .input = "nanF32", .expected = float_literal("nan", "F32")},
      Test_Case{.name = "inf with F64 suffix", .input = "infF64", .expected = float_literal("inf", "F64")},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      life_lang::Diagnostic_Engine diagnostics{"<test>", tc.input};

      life_lang::parser::Parser parser{diagnostics};
      auto const expr = parser.parse_expr();
      REQUIRE(expr.has_value());
      if (expr.has_value()) {
        CHECK(to_sexp_string(*expr, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Float special literals in function bodies") {
  SUBCASE("function returning nan") {
    life_lang::Diagnostic_Engine diagnostics{"<test>", "fn get_nan(): F32 { return nanF32; }"};

    life_lang::parser::Parser parser{diagnostics};
    auto const func = parser.parse_func_def();
    REQUIRE(func.has_value());
    if (func.has_value()) {
      CHECK(func->declaration.name == "get_nan");
      CHECK(to_sexp_string(func->declaration.return_type, 0) == type_name("F32"));
      CHECK(func->body.statements.size() == 1);
      if (func->body.statements.size() == 1) {
        auto const expected = return_statement(float_literal("nan", "F32"));
        CHECK(to_sexp_string(func->body.statements[0], 0) == expected);
      }
    }
  }

  SUBCASE("function returning inf") {
    life_lang::Diagnostic_Engine diagnostics{"<test>", "fn get_infinity(): F64 { return infF64; }"};

    life_lang::parser::Parser parser{diagnostics};
    auto const func = parser.parse_func_def();
    REQUIRE(func.has_value());
    if (func.has_value()) {
      CHECK(func->declaration.name == "get_infinity");
      CHECK(to_sexp_string(func->declaration.return_type, 0) == type_name("F64"));
      CHECK(func->body.statements.size() == 1);
      if (func->body.statements.size() == 1) {
        auto const expected = return_statement(float_literal("inf", "F64"));
        CHECK(to_sexp_string(func->body.statements[0], 0) == expected);
      }
    }
  }
}
