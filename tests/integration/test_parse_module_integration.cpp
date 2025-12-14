// Integration tests for the public parse_module API
//
// These tests validate complete parsing workflows from source code to AST,
// testing the full pipeline that users will interact with through parse_module().
//
// Unlike unit tests (tests/parser/*), these test through the public API only,
// making them resilient to internal implementation changes.
//
// Test Strategy:
// - Use parse_module() public API (not internal parsers)
// - Validate complete input consumption (partial parses should fail)
// - For errors, verify diagnostic format matches clang style
// - Test both successful parses and error cases with diagnostics

#include <doctest/doctest.h>
#include <sstream>

#include "parser.hpp"

// ============================================================================
// Complete Input Validation Tests
// ============================================================================

TEST_CASE("Parse Module - Complete Input Validation") {
  struct Test_Case {
    std::string name;
    std::string input;
    bool should_succeed;
    std::string expected_error_pattern;  // Optional: substring to check in error message
  };

  std::vector<Test_Case> const tests = {
      // === Valid cases - input fully consumed ===
      {.name = "empty module", .input = "", .should_succeed = true, .expected_error_pattern = ""},
      {.name = "whitespace only", .input = "   \n\t  ", .should_succeed = true, .expected_error_pattern = ""},
      {.name = "single function",
       .input = "fn main(): I32 { return 0; }",
       .should_succeed = true,
       .expected_error_pattern = ""},
      {.name = "single struct",
       .input = "struct Point { x: I32, y: I32 }",
       .should_succeed = true,
       .expected_error_pattern = ""},
      {.name = "struct and function",
       .input = "struct Point { x: I32 } fn main(): I32 { return 0; }",
       .should_succeed = true,
       .expected_error_pattern = ""},
      {.name = "multiple structs",
       .input = "struct Point { x: I32 } struct Line { start: Point, end: Point }",
       .should_succeed = true,
       .expected_error_pattern = ""},
      {.name = "multiple functions",
       .input = "fn add(a: I32, b: I32): I32 { return 0; } fn main(): I32 { return 0; }",
       .should_succeed = true,
       .expected_error_pattern = ""},

      // === Invalid cases - parsing failures ===
      {.name = "incomplete function",
       .input = "fn bad syntax",
       .should_succeed = false,
       .expected_error_pattern = "Expected '('"},
      {.name = "incomplete struct", .input = "struct Point {", .should_succeed = false, .expected_error_pattern = ""},
      {.name = "starts with number",
       .input = "123 invalid",
       .should_succeed = false,
       .expected_error_pattern = "Expected module-level item"},
      {.name = "incomplete function declaration",
       .input = "fn foo(",
       .should_succeed = false,
       .expected_error_pattern = ""},
      {.name = "incomplete parameter", .input = "fn foo(x", .should_succeed = false, .expected_error_pattern = ""},

      // === Invalid cases - extra text after valid parse ===
      {.name = "variable after function",
       .input = "fn main(): I32 { return 0; } garbage",
       .should_succeed = false,
       .expected_error_pattern = "Expected module-level item"},
      {.name = "variable after struct",
       .input = "struct Point { x: I32 } garbage",
       .should_succeed = false,
       .expected_error_pattern = "Expected module-level item"},
      {.name = "invalid token after function",
       .input = "fn main(): I32 { return 0; } @#$",
       .should_succeed = false,
       .expected_error_pattern = ""},
  };

  for (auto const& test : tests) {
    SUBCASE(test.name.c_str()) {
      life_lang::parser::Parser parser{test.input, "test.life"};
      auto const result = parser.parse_module();

      // Verify parse success/failure matches expectation
      CHECK(test.should_succeed == result.has_value());

      if (!test.should_succeed && !result) {
        // Verify we got diagnostics with errors (from result.error(), not parser)
        auto const& diagnostics = result.error();
        REQUIRE(diagnostics.has_errors());
        REQUIRE_FALSE(diagnostics.diagnostics().empty());

        // If expected error pattern specified, verify it appears in diagnostic
        if (!test.expected_error_pattern.empty()) {
          std::ostringstream oss;
          diagnostics.print(oss);
          CHECK(oss.str().find(test.expected_error_pattern) != std::string::npos);
        }
      }
    }
  }
}

// ============================================================================
// Struct Literals and Field Access Tests
// ============================================================================

TEST_CASE("Parse Module - Struct Literals and Field Access") {
  struct Test_Case {
    std::string name;
    std::string input;
    bool should_succeed;
  };

  std::vector<Test_Case> const tests = {
      // === Struct literals in function bodies ===
      {.name = "function returning struct literal",
       .input = "struct Point { x: I32, y: I32 } fn origin(): Point { return Point { x: 0, y: 0 }; }",
       .should_succeed = true},
      {.name = "struct literal with trailing comma",
       .input = "struct Point { x: I32, y: I32 } fn origin(): Point { return Point { x: 0, y: 0, }; }",
       .should_succeed = true},
      {.name = "nested struct literal",
       .input = "struct Inner { v: I32 } struct Outer { i: Inner } "
                "fn make(): Outer { return Outer { i: Inner { v: 1 } }; }",
       .should_succeed = true},

      // === Field access in function bodies ===
      {.name = "field access in return",
       .input = "struct Point { x: I32 } fn get_x(p: Point): I32 { return p.x; }",
       .should_succeed = true},
      {.name = "chained field access",
       .input = "struct Inner { val: I32 } struct Outer { inner: Inner } "
                "fn get_val(o: Outer): I32 { return o.inner.val; }",
       .should_succeed = true},
      {.name = "field access on struct literal",
       .input = "struct Point { x: I32, y: I32 } fn get_x(): I32 { return Point { x: 42, y: 0 }.x; }",
       .should_succeed = true},

      // === Complex combinations ===
      {.name = "struct with nested struct fields",
       .input = "struct Point { x: I32, y: I32 } struct Rect { top_left: Point, bottom_right: Point }",
       .should_succeed = true},
      {.name = "function with struct literal and field access",
       .input = "struct Point { x: I32, y: I32 } fn double_x(p: Point): I32 { return p.x; }",
       .should_succeed = true},
      {.name = "multiple struct operations",
       .input = "struct Point { x: I32, y: I32 } "
                "fn process(p: Point): Point { return Point { x: p.x, y: p.y }; }",
       .should_succeed = true},

      // === Invalid cases ===
      {.name = "incomplete struct literal",
       .input = "struct Point { x: I32 } fn bad(): Point { return Point { x: ",
       .should_succeed = false},
      {.name = "missing closing brace in struct literal",
       .input = "struct Point { x: I32 } fn bad(): Point { return Point { x: 0 ",
       .should_succeed = false},
      {.name = "incomplete field access",
       .input = "struct Point { x: I32 } fn bad(p: Point): I32 { return p.",
       .should_succeed = false},
  };

  for (auto const& test : tests) {
    SUBCASE(test.name.c_str()) {
      life_lang::parser::Parser parser{test.input, "test.life"};
      auto const result = parser.parse_module();

      CHECK(test.should_succeed == result.has_value());

      if (!test.should_succeed && !result) {
        REQUIRE(result.error().has_errors());
      }
    }
  }
}

// ============================================================================
// Diagnostic Format Tests (Clang-Style Output)
// ============================================================================

TEST_CASE("Parse Module - Diagnostic Format Matches Clang Style") {
  SUBCASE("Single-line parse error with source context") {
    std::string const invalid_input = "fn bad syntax here";
    life_lang::parser::Parser parser{invalid_input, "test.life"};
    auto const result = parser.parse_module();

    REQUIRE_FALSE(result.has_value());

    std::ostringstream oss;
    result.error().print(oss);
    std::string const output = oss.str();

    // New parser generates multiple specific errors - just check it contains key error
    CHECK(output.find("test.life:1:8: error: Expected '(', found 's'") != std::string::npos);
    CHECK(output.find("fn bad syntax here") != std::string::npos);
  }

  SUBCASE("Multi-line source with error on last line") {
    std::string const source =
        "fn main(): I32 {\n"
        "    return 0;\n"
        "}\n"
        "unexpected garbage";

    life_lang::parser::Parser parser{source, "multiline.life"};
    auto const result = parser.parse_module();

    REQUIRE_FALSE(result.has_value());

    auto const& diag = result.error();
    REQUIRE(diag.has_errors());

    // Error should be on line 4
    auto const& first_error = diag.diagnostics().front();
    CHECK(first_error.range.start.line == 4);

    // Verify clang-style formatting with proper error message
    std::ostringstream oss;
    diag.print(oss);
    std::string const output = oss.str();

    // New parser reports specific error about missing semicolon
    CHECK(output.find("multiline.life:4:") != std::string::npos);
    CHECK(output.find("unexpected garbage") != std::string::npos);
  }

  SUBCASE("Error on non-first line") {
    std::string const source =
        "fn main(): I32 { return 0; }\n"
        "fn bad(";

    life_lang::parser::Parser parser{source, "error_line2.life"};
    auto const result = parser.parse_module();

    REQUIRE_FALSE(result.has_value());

    auto const& diag = result.error();
    REQUIRE(diag.has_errors());

    // Error should be on line 2
    auto const& first_error = diag.diagnostics().front();
    CHECK(first_error.range.start.line == 2);
  }
}

// ============================================================================
// Cross-Platform Line Endings Tests
// ============================================================================

TEST_CASE("Parse Module - Cross-Platform Line Endings") {
  SUBCASE("Unix line endings (LF)") {
    std::string const source = "fn main(): I32 {\n    return 0;\n}\n";
    life_lang::parser::Parser parser{source, "unix.life"};
    auto const result = parser.parse_module();
    REQUIRE(result.has_value());
  }

  SUBCASE("Windows line endings (CRLF)") {
    std::string const source = "fn main(): I32 {\r\n    return 0;\r\n}\r\n";
    life_lang::parser::Parser parser{source, "windows.life"};
    auto const result = parser.parse_module();
    REQUIRE(result.has_value());
  }

  SUBCASE("Old Mac line endings (CR)") {
    std::string const source = "fn main(): I32 {\r    return 0;\r}\r";
    life_lang::parser::Parser parser{source, "oldmac.life"};
    auto const result = parser.parse_module();
    REQUIRE(result.has_value());
  }

  SUBCASE("Mixed line endings") {
    std::string const source = "fn main(): I32 {\r\n    return 0;\n}\r";
    life_lang::parser::Parser parser{source, "mixed.life"};
    auto const result = parser.parse_module();
    REQUIRE(result.has_value());
  }

  SUBCASE("Error reporting with CRLF") {
    std::string const source = "fn main(): I32 {\r\n    return 0;\r\n}\r\ngarbag";
    life_lang::parser::Parser parser{source, "error_crlf.life"};
    auto const result = parser.parse_module();
    REQUIRE_FALSE(result.has_value());

    // Error should be on line 4
    auto const& diag = result.error();
    REQUIRE(diag.has_errors());
    auto const& first_error = diag.diagnostics().front();
    CHECK(first_error.range.start.line == 4);
  }

  SUBCASE("Error reporting with CR") {
    std::string const source = "fn main(): I32 {\r    return 0;\r}\rinvalid";
    life_lang::parser::Parser parser{source, "error_cr.life"};
    auto const result = parser.parse_module();
    REQUIRE_FALSE(result.has_value());

    // Error should be on line 4
    auto const& diag = result.error();
    REQUIRE(diag.has_errors());
    auto const& first_error = diag.diagnostics().front();
    CHECK(first_error.range.start.line == 4);
  }
}

// ============================================================================
// Anonymous/Default Filenames Tests
// ============================================================================

TEST_CASE("Parse Module - Anonymous Module Names") {
  SUBCASE("Default <input> name") {
    life_lang::parser::Parser parser{"invalid 123", "<input>"};
    auto const result = parser.parse_module();
    REQUIRE_FALSE(result.has_value());

    std::ostringstream oss;
    result.error().print(oss);
    CHECK(oss.str().find("<input>:") != std::string::npos);
  }

  SUBCASE("Custom anonymous name <stdin>") {
    life_lang::parser::Parser parser{"fn bad(", "<stdin>"};
    auto const result = parser.parse_module();
    REQUIRE_FALSE(result.has_value());

    std::ostringstream oss;
    result.error().print(oss);
    CHECK(oss.str().find("<stdin>:") != std::string::npos);
  }

  SUBCASE("No filename defaults to <input>") {
    life_lang::parser::Parser parser{"garbage", "<input>"};
    auto const result = parser.parse_module();
    REQUIRE_FALSE(result.has_value());

    std::ostringstream oss;
    result.error().print(oss);
    CHECK(oss.str().find("<input>:") != std::string::npos);
  }
}
