#include <doctest/doctest.h>

#include "parser.hpp"
#include "sexp.hpp"
#include "utils.hpp"

using namespace life_lang::parser;
using namespace life_lang::ast;
using namespace test_sexp;

TEST_CASE("Raw string - basic") {
  Parser parser(R"(r"hello world")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    CHECK(to_sexp_string(*expr, 0) == R"((string "r\"hello world\""))");
  }
}

TEST_CASE("Raw string - with backslashes") {
  Parser parser(R"(r"C:\path\to\file.txt")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    CHECK(to_sexp_string(*expr, 0) == R"((string "r\"C:\\path\\to\\file.txt\""))");
  }
}

TEST_CASE("Raw string - with double quotes using delimiter") {
  Parser parser(R"(r#"He said "hello" to me"#)");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    CHECK(to_sexp_string(*expr, 0) == R"((string "r#\"He said \"hello\" to me\"#"))");
  }
}

TEST_CASE("Raw string - multi-line") {
  Parser parser(R"(r"line 1
line 2
line 3")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    std::string const expected = R"((string "r\"line 1\nline 2\nline 3\""))";
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Raw string - regex pattern") {
  Parser parser(R"(r"\d+\.\d+")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    CHECK(to_sexp_string(*expr, 0) == R"((string "r\"\\d+\\.\\d+\""))");
  }
}

TEST_CASE("Raw string - JSON content") {
  Parser parser(R"(r#"{"key": "value", "number": 42}"#)");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    CHECK(to_sexp_string(*expr, 0) == R"((string "r#\"{\"key\": \"value\", \"number\": 42}\"#"))");
  }
}

TEST_CASE("Raw string - empty") {
  Parser parser(R"(r"")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    CHECK(to_sexp_string(*expr, 0) == R"((string "r\"\""))");
  }
}

TEST_CASE("Raw string - only newlines") {
  Parser parser("r\"\n\n\n\"");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    CHECK(to_sexp_string(*expr, 0) == "(string \"r\\\"\\n\\n\\n\\\"\")");
  }
}

TEST_CASE("Raw string - multiple delimiters") {
  Parser parser(R"(r##"Contains "# and "#" patterns"##)");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    CHECK(to_sexp_string(*expr, 0) == R"((string "r##\"Contains \"# and \"#\" patterns\"##"))");
  }
}

TEST_CASE("Raw string - no escape processing for \\n") {
  Parser parser(R"(r"Line 1\nLine 2")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    CHECK(to_sexp_string(*expr, 0) == R"((string "r\"Line 1\\nLine 2\""))");
  }
}

TEST_CASE("Raw string - no escape processing for \\t") {
  Parser parser(R"(r"Column1\tColumn2")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    CHECK(to_sexp_string(*expr, 0) == R"((string "r\"Column1\\tColumn2\""))");
  }
}

TEST_CASE("Raw string - literal backslash-quote") {
  Parser parser(R"(r#"Path: \"C:\Users\""#)");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    CHECK(to_sexp_string(*expr, 0) == R"((string "r#\"Path: \\\"C:\\Users\\\"\"#"))");
  }
}

TEST_CASE("Raw string - Windows path") {
  Parser parser(R"(r"C:\Users\Documents\file.txt")");
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    CHECK(to_sexp_string(*expr, 0) == R"((string "r\"C:\\Users\\Documents\\file.txt\""))");
  }
}

TEST_CASE("Raw string - unterminated error") {
  Parser parser(R"(r"unterminated)");
  auto const expr = parser.parse_expr();
  CHECK_FALSE(expr.has_value());
}

TEST_CASE("Raw string - unterminated with delimiter") {
  Parser parser(R"(r#"unterminated)");
  auto const expr = parser.parse_expr();
  CHECK_FALSE(expr.has_value());
}

TEST_CASE("Raw string - wrong delimiter count") {
  Parser parser(R"(r##"wrong delimiter"#)");
  auto const expr = parser.parse_expr();
  CHECK_FALSE(expr.has_value());
}
