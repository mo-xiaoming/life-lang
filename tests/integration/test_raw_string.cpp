#include <doctest/doctest.h>

#include "../parser/utils.hpp"
#include "diagnostics.hpp"
#include "parser.hpp"
#include "sexp.hpp"

using namespace life_lang::parser;
using namespace life_lang::ast;
using namespace test_sexp;

TEST_CASE("Raw string literals - simple expressions") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "raw string with backslashes",
       .input = R"(r"C:\\Users\\Documents\\file.txt")",
       .expected = string(R"(r"C:\\Users\\Documents\\file.txt")")},
      {.name = "raw string with delimiter",
       .input = R"(r#"{"key": "value", "number": 42}"#)",
       .expected = string(R"(r#"{"key": "value", "number": 42}"#)")},
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

TEST_CASE("Raw strings in function return statements") {
  SUBCASE("function with raw string path") {
    constexpr std::string_view k_source = R"(
    fn get_path(): String {
      return r"C:\Users\Documents\file.txt";
    }
  )";
    life_lang::Diagnostic_Engine diagnostics{"<test>", k_source};
    life_lang::parser::Parser parser{diagnostics};

    auto const func = parser.parse_func_def();
    REQUIRE(func.has_value());
    if (func.has_value()) {
      CHECK(func->declaration.name == "get_path");
      CHECK(to_sexp_string(func->declaration.return_type, 0) == type_name("String"));
      CHECK(func->body.statements.size() == 1);
      if (func->body.statements.size() == 1) {
        auto const expected = return_statement(string(R"(r"C:\Users\Documents\file.txt")"));
        CHECK(to_sexp_string(func->body.statements[0], 0) == expected);
      }
    }
  }

  SUBCASE("function with regex pattern") {
    constexpr std::string_view k_source = R"(
    fn email_pattern(): String {
      return r"[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}";
    }
  )";
    life_lang::Diagnostic_Engine diagnostics{"<test>", k_source};
    life_lang::parser::Parser parser{diagnostics};

    auto const func = parser.parse_func_def();
    REQUIRE(func.has_value());
    if (func.has_value()) {
      CHECK(func->declaration.name == "email_pattern");
      CHECK(to_sexp_string(func->declaration.return_type, 0) == type_name("String"));
      CHECK(func->body.statements.size() == 1);
      if (func->body.statements.size() == 1) {
        auto const expected = return_statement(string(R"(r"[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}")"));
        CHECK(to_sexp_string(func->body.statements[0], 0) == expected);
      }
    }
  }
}
