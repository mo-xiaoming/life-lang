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

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <sstream>

#include "rules.hpp"

using life_lang::parser::parse_module;

// ============================================================================
// Complete Input Validation Tests
// ============================================================================

TEST_CASE("Parse Module - Complete Input Validation", "[integration][parse_module]") {
  struct Test_Case {
    std::string name;
    std::string input;
    bool should_succeed;
    std::string expected_error_pattern;  // Optional: substring to check in error message
  };

  auto const test = GENERATE(
      Catch::Generators::values<Test_Case>({
          // === Valid cases - input fully consumed ===
          {"empty module", "", true, ""},
          {"whitespace only", "   \n\t  ", true, ""},
          {"single function", "fn main(): I32 { return 0; }", true, ""},
          {"single struct", "struct Point { x: I32, y: I32 }", true, ""},
          {"struct and function", "struct Point { x: I32 } fn main(): I32 { return 0; }", true, ""},
          {"multiple structs", "struct Point { x: I32 } struct Line { start: Point, end: Point }", true, ""},
          {"multiple functions", "fn add(a: I32, b: I32): I32 { return 0; } fn main(): I32 { return 0; }", true, ""},

          // === Invalid cases - parsing failures ===
          {"incomplete function", "fn bad syntax", false, "Expecting: '('"},
          {"incomplete struct", "struct Point {", false, ""},
          {"starts with number", "123 invalid", false, "Unexpected input"},
          {"incomplete function declaration", "fn foo(", false, ""},
          {"incomplete parameter", "fn foo(x", false, ""},

          // === Invalid cases - extra text after valid parse ===
          {"extra text after function", "fn main(): I32 { return 0; } garbage", false, "Unexpected input after module"},
          {"extra text after struct", "struct Point { x: I32 } garbage", false, "Unexpected input after module"},
      })
  );

  DYNAMIC_SECTION(test.name) {
    auto const result = parse_module(test.input, "test.life");

    // Verify parse success/failure matches expectation
    CHECK(test.should_succeed == result.has_value());

    if (!test.should_succeed && !result) {
      // Verify we got diagnostics with errors
      REQUIRE(result.error().has_errors());
      REQUIRE_FALSE(result.error().diagnostics().empty());

      // If expected error pattern specified, verify it appears in diagnostic
      if (!test.expected_error_pattern.empty()) {
        std::ostringstream oss;
        result.error().print(oss);
        CHECK(oss.str().find(test.expected_error_pattern) != std::string::npos);
      }
    }
  }
}

// ============================================================================
// Struct Literals and Field Access Tests
// ============================================================================

TEST_CASE("Parse Module - Struct Literals and Field Access", "[integration][parse_module]") {
  struct Test_Case {
    std::string name;
    std::string input;
    bool should_succeed;
  };

  auto const test = GENERATE(
      Catch::Generators::values<Test_Case>({
          // === Struct literals in function bodies ===
          {"function returning struct literal",
           "struct Point { x: I32, y: I32 } fn origin(): Point { return Point { x: 0, y: 0 }; }", true},
          {"struct literal with trailing comma",
           "struct Point { x: I32, y: I32 } fn origin(): Point { return Point { x: 0, y: 0, }; }", true},
          {"nested struct literal",
           "struct Inner { v: I32 } struct Outer { i: Inner } "
           "fn make(): Outer { return Outer { i: Inner { v: 1 } }; }",
           true},

          // === Field access in function bodies ===
          {"field access in return", "struct Point { x: I32 } fn get_x(p: Point): I32 { return p.x; }", true},
          {"chained field access",
           "struct Inner { val: I32 } struct Outer { inner: Inner } "
           "fn get_val(o: Outer): I32 { return o.inner.val; }",
           true},
          {"field access on struct literal",
           "struct Point { x: I32, y: I32 } fn get_x(): I32 { return Point { x: 42, y: 0 }.x; }", true},

          // === Complex combinations ===
          {"struct with nested struct fields",
           "struct Point { x: I32, y: I32 } struct Rect { top_left: Point, bottom_right: Point }", true},
          {"function with struct literal and field access",
           "struct Point { x: I32, y: I32 } fn double_x(p: Point): I32 { return p.x; }", true},
          {"multiple struct operations",
           "struct Point { x: I32, y: I32 } "
           "fn process(p: Point): Point { return Point { x: p.x, y: p.y }; }",
           true},

          // === Invalid cases ===
          {"incomplete struct literal", "struct Point { x: I32 } fn bad(): Point { return Point { x: ", false},
          {"missing closing brace in struct literal", "struct Point { x: I32 } fn bad(): Point { return Point { x: 0 ",
           false},
          {"incomplete field access", "struct Point { x: I32 } fn bad(p: Point): I32 { return p.", false},
      })
  );

  DYNAMIC_SECTION(test.name) {
    auto const result = parse_module(test.input, "test.life");

    CHECK(test.should_succeed == result.has_value());

    if (!test.should_succeed && !result) {
      REQUIRE(result.error().has_errors());
    }
  }
}

// ============================================================================
// Diagnostic Format Tests (Clang-Style Output)
// ============================================================================

TEST_CASE("Parse Module - Diagnostic Format Matches Clang Style", "[integration][parse_module][diagnostics]") {
  SECTION("Single-line parse error with source context") {
    std::string const invalid_input = "fn bad syntax here";
    auto const result = parse_module(invalid_input, "test.life");

    REQUIRE_FALSE(result.has_value());

    std::ostringstream oss;
    result.error().print(oss);
    std::string const output = oss.str();

    // Expected format: file:line:col: level: message
    //                  source line
    //                  caret pointing to error
    std::string const expected =
        "test.life:1:1: error: Failed to parse module: Expecting: '(' here:\n"
        "    fn bad syntax here\n"
        "    ^\n";

    CHECK(output == expected);
  }

  SECTION("Multi-line source with error on last line") {
    std::string const source =
        "fn main(): I32 {\n"
        "    return 0;\n"
        "}\n"
        "unexpected garbage";

    auto const result = parse_module(source, "multiline.life");

    REQUIRE_FALSE(result.has_value());

    auto const& diag = result.error();
    REQUIRE(diag.has_errors());

    // Error should be on line 4
    auto const& first_error = diag.diagnostics().front();
    CHECK(first_error.range.start.line == 4);

    // Verify clang-style formatting with proper highlighting
    std::ostringstream oss;
    diag.print(oss);
    std::string const output = oss.str();

    // "unexpected garbage" = 18 chars, so caret + 17 tildes
    std::string const expected =
        "multiline.life:4:1: error: Unexpected input after module\n"
        "    unexpected garbage\n"
        "    ^~~~~~~~~~~~~~~~~~\n";

    CHECK(output == expected);
  }

  SECTION("Error on non-first line") {
    std::string const source =
        "fn main(): I32 { return 0; }\n"
        "fn bad(";

    auto const result = parse_module(source, "error_line2.life");

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

TEST_CASE("Parse Module - Cross-Platform Line Endings", "[integration][parse_module][line-endings]") {
  SECTION("Unix line endings (LF)") {
    std::string const source = "fn main(): I32 {\n    return 0;\n}\n";
    auto const result = parse_module(source, "unix.life");
    REQUIRE(result.has_value());
  }

  SECTION("Windows line endings (CRLF)") {
    std::string const source = "fn main(): I32 {\r\n    return 0;\r\n}\r\n";
    auto const result = parse_module(source, "windows.life");
    REQUIRE(result.has_value());
  }

  SECTION("Old Mac line endings (CR)") {
    std::string const source = "fn main(): I32 {\r    return 0;\r}\r";
    auto const result = parse_module(source, "oldmac.life");
    REQUIRE(result.has_value());
  }

  SECTION("Mixed line endings") {
    std::string const source = "fn main(): I32 {\r\n    return 0;\n}\r";
    auto const result = parse_module(source, "mixed.life");
    REQUIRE(result.has_value());
  }

  SECTION("Error reporting with CRLF") {
    std::string const source = "fn main(): I32 {\r\n    return 0;\r\n}\r\ngarbag";
    auto const result = parse_module(source, "error_crlf.life");
    REQUIRE_FALSE(result.has_value());

    // Error should be on line 4
    auto const& diag = result.error();
    REQUIRE(diag.has_errors());
    auto const& first_error = diag.diagnostics().front();
    CHECK(first_error.range.start.line == 4);
  }

  SECTION("Error reporting with CR") {
    std::string const source = "fn main(): I32 {\r    return 0;\r}\rinvalid";
    auto const result = parse_module(source, "error_cr.life");
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

TEST_CASE("Parse Module - Anonymous Module Names", "[integration][parse_module]") {
  SECTION("Default <input> name") {
    auto const result = parse_module("invalid 123", "<input>");
    REQUIRE_FALSE(result.has_value());

    std::ostringstream oss;
    result.error().print(oss);
    CHECK(oss.str().find("<input>:") != std::string::npos);
  }

  SECTION("Custom anonymous name <stdin>") {
    auto const result = parse_module("fn bad(", "<stdin>");
    REQUIRE_FALSE(result.has_value());

    std::ostringstream oss;
    result.error().print(oss);
    CHECK(oss.str().find("<stdin>:") != std::string::npos);
  }

  SECTION("No filename defaults to <input>") {
    auto const result = parse_module("garbage");
    REQUIRE_FALSE(result.has_value());

    std::ostringstream oss;
    result.error().print(oss);
    CHECK(oss.str().find("<input>:") != std::string::npos);
  }
}
