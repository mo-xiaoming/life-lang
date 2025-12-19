// tests/integration/test_raw_string.cpp
#include <doctest/doctest.h>

#include "parser.hpp"
#include "sexp.hpp"

using namespace life_lang::parser;
using namespace life_lang::ast;

TEST_CASE("Integration - raw string in function") {
  Parser parser(R"(
    fn get_path(): String {
      return r"C:\Users\Documents\file.txt";
    }
  )");

  auto const module = parser.parse_module();
  REQUIRE(module);
  if (module) {
    auto const sexp = to_sexp_string(*module, 0);
    // Check function structure
    CHECK(sexp.find("(func_def") != std::string::npos);
    CHECK(sexp.find("\"get_path\"") != std::string::npos);
    // Check raw string is present (with escaped backslashes in S-expression)
    CHECK(sexp.find(R"((string "r\"C:\\Users\\Documents\\file.txt\""))") != std::string::npos);
  }
}

TEST_CASE("Integration - raw string with delimiter in function") {
  Parser parser(R"(
    fn get_json(): String {
      return r#"{"key": "value", "number": 42}"#;
    }
  )");

  auto const module = parser.parse_module();
  REQUIRE(module);
  if (module) {
    auto const sexp = to_sexp_string(*module, 0);
    CHECK(sexp.find("(func_def") != std::string::npos);
    CHECK(sexp.find("\"get_json\"") != std::string::npos);
    // Check raw string with delimiter
    CHECK(sexp.find(R"((string "r#\"{\"key\": \"value\", \"number\": 42}\"#"))") != std::string::npos);
  }
}

TEST_CASE("Integration - raw string multi-line in function") {
  Parser parser(R"(
    fn get_multiline(): String {
      return r"Line 1
Line 2
Line 3";
    }
  )");

  auto const module = parser.parse_module();
  REQUIRE(module);
  if (module) {
    auto const sexp = to_sexp_string(*module, 0);
    CHECK(sexp.find("(func_def") != std::string::npos);
    CHECK(sexp.find("\"get_multiline\"") != std::string::npos);
    // Check that newlines are preserved (escaped in S-expression as \n)
    CHECK(sexp.find(R"((string "r\"Line 1\nLine 2\nLine 3\""))") != std::string::npos);
  }
}

TEST_CASE("Integration - raw string for regex pattern") {
  Parser parser(R"(
    fn email_pattern(): String {
      return r"[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}";
    }
  )");

  auto const module = parser.parse_module();
  REQUIRE(module);
  if (module) {
    auto const sexp = to_sexp_string(*module, 0);
    CHECK(sexp.find("(func_def") != std::string::npos);
    CHECK(sexp.find("\"email_pattern\"") != std::string::npos);
    // Check regex pattern with unescaped backslashes
    CHECK(sexp.find(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,})") != std::string::npos);
  }
}

TEST_CASE("Integration - raw string vs regular string") {
  Parser parser(R"(
    fn compare_strings(): () {
      let raw_path = r"C:\path\to\file";
      let escaped_path = "C:\\path\\to\\file";
      return ();
    }
  )");

  auto const module = parser.parse_module();
  REQUIRE(module);
  if (module) {
    auto const sexp = to_sexp_string(*module, 0);
    CHECK(sexp.find("(func_def") != std::string::npos);
    // Both should have same output (escaped backslashes in S-expression)
    CHECK(sexp.find(R"((string "r\"C:\\path\\to\\file\""))") != std::string::npos);
    CHECK(sexp.find(R"((string \"r\\\"C:\\\\path\\\\to\\\\file\\\"\"))") != std::string::npos);
  }
}
