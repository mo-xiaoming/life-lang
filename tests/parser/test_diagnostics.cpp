#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "rules.hpp"

using life_lang::Diagnostic_Engine;
using life_lang::Source_Range;
using life_lang::parser::parse_module;

// Test helper to check diagnostic output
struct Diagnostic_Test_Case {
  std::string input;
  std::string filename;
  bool should_succeed;
  std::string expected_error_substring;  // Substring that should appear in error
  std::size_t expected_line{1};
  std::size_t expected_column{1};
};

TEST_CASE("Parse Module with Diagnostics", "[parser][diagnostics]") {
  std::vector<Diagnostic_Test_Case> const tests = {
      // Valid cases
      {.input = "",
       .filename = "empty.life",
       .should_succeed = true,
       .expected_error_substring = "",
       .expected_line = 1,
       .expected_column = 1},

      {.input = "fn main(): I32 { return 0; }",
       .filename = "valid.life",
       .should_succeed = true,
       .expected_error_substring = "",
       .expected_line = 1,
       .expected_column = 1},

      {.input = "   \n\t  ",
       .filename = "whitespace.life",
       .should_succeed = true,
       .expected_error_substring = "",
       .expected_line = 1,
       .expected_column = 1},

      // Invalid syntax - incomplete function
      // Note: Spirit X3 error reporting currently points to start of failed parse
      {.input = "fn bad syntax",
       .filename = "incomplete.life",
       .should_succeed = false,
       .expected_error_substring = "",  // Parse failure
       .expected_line = 1,
       .expected_column = 1},

      // Invalid - starts with number
      {.input = "123 invalid",
       .filename = "number.life",
       .should_succeed = false,
       .expected_error_substring = "",  // Will get "Unexpected input"
       .expected_line = 1,
       .expected_column = 1},

      // Invalid - incomplete function declaration
      // Note: Spirit X3 error reporting currently points to start of failed parse
      {.input = "fn foo(",
       .filename = "incomplete_decl.life",
       .should_succeed = false,
       .expected_error_substring = "",  // Parse failure
       .expected_line = 1,
       .expected_column = 1},

      // Invalid - extra text after valid module
      {.input = "fn main(): I32 { return 0; } garbage",
       .filename = "extra_text.life",
       .should_succeed = false,
       .expected_error_substring = "Unexpected input after module",
       .expected_line = 1,
       .expected_column = 30},

      // Multi-line error
      {.input = "fn main(): I32 {\n"
                "    return 0;\n"
                "} extra stuff",
       .filename = "multiline.life",
       .should_succeed = false,
       .expected_error_substring = "Unexpected input after module",
       .expected_line = 3,
       .expected_column = 3},

      // Error on second line
      // Note: Spirit X3 error reporting currently points to start of failed parse
      {.input = "fn main(): I32 { return 0; }\n"
                "fn bad(",
       .filename = "error_line2.life",
       .should_succeed = false,
       .expected_error_substring = "",  // Will get parsing error
       .expected_line = 2,
       .expected_column = 1},  // Error reported at start of line
  };

  for (auto const& test : tests) {
    DYNAMIC_SECTION("Test: " << test.filename) {
      auto const result = parse_module(test.input, test.filename);

      if (test.should_succeed) {
        REQUIRE(result.has_value());
        if (result) {
          // Verify we got a valid module
          INFO("Successfully parsed: " << test.input);
        }
      } else {
        REQUIRE_FALSE(result.has_value());
        if (!result) {
          auto const& diag_engine = result.error();

          // Verify we have diagnostics
          REQUIRE(diag_engine.has_errors());
          REQUIRE_FALSE(diag_engine.diagnostics().empty());

          // Check the first diagnostic
          auto const& first_diag = diag_engine.diagnostics().front();

          // Verify filename
          CHECK(diag_engine.filename() == test.filename);

          // Verify location (at least the line number)
          CHECK(first_diag.range.start.line == test.expected_line);

          // If expected_column is set, check it
          if (test.expected_column > 1) {
            CHECK(first_diag.range.start.column == test.expected_column);
          }

          // If we expect a specific error substring, check for it
          if (!test.expected_error_substring.empty()) {
            CHECK(first_diag.message.find(test.expected_error_substring) != std::string::npos);
          }

          // Print diagnostic for debugging (only in verbose mode)
          std::ostringstream oss;
          diag_engine.print(oss);
          INFO("Diagnostic output:\n" << oss.str());
        }
      }
    }
  }
}

TEST_CASE("Diagnostic Format Matches Clang Style", "[parser][diagnostics][format]") {
  std::string const invalid_input = "fn bad syntax here";
  auto const result = parse_module(invalid_input, "test.life");

  REQUIRE_FALSE(result.has_value());

  std::ostringstream oss;
  result.error().print(oss);
  std::string const output = oss.str();

  // Expected format with actual highlighting from parse_module
  // Note: "fn bad syntax here" = 18 chars, highlight_len = 18 -> 17 tildes + caret
  std::string const expected =
      "test.life:1:1: error: Unexpected input after module\n"
      "    fn bad syntax here\n"
      "    ^~~~~~~~~~~~~~~~~~\n";

  CHECK(output == expected);
}

TEST_CASE("Multi-line Source Diagnostic", "[parser][diagnostics][multiline]") {
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

  // Print and verify format
  std::ostringstream oss;
  diag.print(oss);
  std::string const output = oss.str();

  // Expected format showing exact error location and highlighting
  // Note: "unexpected garbage" = 18 chars, so highlight_len = 18, -> caret + 17 tildes
  std::string const expected =
      "multiline.life:4:1: error: Unexpected input after module\n"
      "    unexpected garbage\n"
      "    ^~~~~~~~~~~~~~~~~~\n";

  CHECK(output == expected);
}

TEST_CASE("Anonymous Module Names", "[parser][diagnostics]") {
  SECTION("Default <input> name") {
    auto const result = parse_module("invalid 123", "<input>");
    REQUIRE_FALSE(result.has_value());

    std::ostringstream oss;
    result.error().print(oss);
    CHECK(oss.str().find("<input>:") != std::string::npos);  // NOLINT(abseil-string-find-str-contains)
  }

  SECTION("Custom anonymous name <stdin>") {
    auto const result = parse_module("fn bad(", "<stdin>");
    REQUIRE_FALSE(result.has_value());

    std::ostringstream oss;
    result.error().print(oss);
    CHECK(oss.str().find("<stdin>:") != std::string::npos);  // NOLINT(abseil-string-find-str-contains)
  }

  SECTION("REPL mode <repl>") {
    auto const result = parse_module("garbage", "<repl>");
    REQUIRE_FALSE(result.has_value());

    std::ostringstream oss;
    result.error().print(oss);
    CHECK(oss.str().find("<repl>:") != std::string::npos);  // NOLINT(abseil-string-find-str-contains)
  }
}

TEST_CASE("Diagnostic Source Line Retrieval", "[parser][diagnostics][source]") {
  std::string const source =
      "line 1\n"
      "line 2\n"
      "line 3\n";

  Diagnostic_Engine const diag("test.life", source);

  // Test get_line function
  CHECK(diag.get_line(1) == "line 1");
  CHECK(diag.get_line(2) == "line 2");
  CHECK(diag.get_line(3) == "line 3");
  CHECK(diag.get_line(4).empty());  // Beyond end
  CHECK(diag.get_line(0).empty());  // Invalid (0-indexed)
}

TEST_CASE("Diagnostic Range Highlighting", "[parser][diagnostics][highlighting]") {
  SECTION("Single-line error with specific range") {
    std::string const source = "fn main() { bad_syntax }";
    Diagnostic_Engine diag("test.life", source);

    // Simulate error on "bad_syntax" (columns 13-23)
    Source_Range const range{.start = {.line = 1, .column = 13}, .end = {.line = 1, .column = 23}};
    diag.add_error(range, "Unknown identifier");

    std::ostringstream oss;
    diag.print(oss);
    std::string const output = oss.str();

    // Expected: caret at start, tildes under error range
    std::string const expected =
        "test.life:1:13: error: Unknown identifier\n"
        "    fn main() { bad_syntax }\n"
        "                ^~~~~~~~~~\n";

    CHECK(output == expected);
  }

  SECTION("Single-character error") {
    std::string const source = "x + y";
    Diagnostic_Engine diag("simple.life", source);

    // Error on single character '+'
    Source_Range const range{.start = {.line = 1, .column = 3}, .end = {.line = 1, .column = 4}};
    diag.add_error(range, "Unexpected operator");

    std::ostringstream oss;
    diag.print(oss);
    std::string const output = oss.str();

    std::string const expected =
        "simple.life:1:3: error: Unexpected operator\n"
        "    x + y\n"
        "      ^\n";

    CHECK(output == expected);
  }

  SECTION("Multi-line error range") {
    std::string const source =
        "fn main() {\n"
        "    let x = very_long +\n"
        "            expression;\n"
        "    return x;\n"
        "}";

    Diagnostic_Engine diag("multiline_range.life", source);

    // Error spanning lines 2-3
    Source_Range const range{.start = {.line = 2, .column = 13}, .end = {.line = 3, .column = 23}};
    diag.add_error(range, "Expression too complex");

    std::ostringstream oss;
    diag.print(oss);
    std::string const output = oss.str();

    // Expected: ^~~~ on first line to end of line, NO "..." (consecutive lines), ~~~^ on last line
    // First line: "very_long +" from column 13 to end = 11 chars ("very_long +"), so "^" + 10 tildes
    // Last line: "            expression;" - column 23 is ';', visual_column gives 22, so 21 tildes + caret
    std::string const expected =
        "multiline_range.life:2:13: error: Expression too complex\n"
        "        let x = very_long +\n"
        "                ^~~~~~~~~~~\n"
        "                expression;\n"
        "    ~~~~~~~~~~~~~~~~~~~~~^\n";

    CHECK(output == expected);
  }

  SECTION("Error at line start") {
    std::string const source = "invalid_token\nfn main(): I32 { return 0; }";
    Diagnostic_Engine diag("start.life", source);

    // Error at very start of line
    Source_Range const range{.start = {.line = 1, .column = 1}, .end = {.line = 1, .column = 14}};
    diag.add_error(range, "Invalid token at start");

    std::ostringstream oss;
    diag.print(oss);
    std::string const output = oss.str();

    // Note: columns 1-14 = highlight_len 13, so caret + 12 tildes
    std::string const expected =
        "start.life:1:1: error: Invalid token at start\n"
        "    invalid_token\n"
        "    ^~~~~~~~~~~~~\n";

    CHECK(output == expected);
  }
}

TEST_CASE("Multiple Diagnostics", "[parser][diagnostics][multiple]") {
  std::string const source =
      "error1\n"
      "error2\n"
      "error3";

  Diagnostic_Engine diag("multiple.life", source);

  // Add multiple errors and a warning
  diag.add_error(Source_Range{.start = {.line = 1, .column = 1}, .end = {.line = 1, .column = 7}}, "First error");
  diag.add_error(Source_Range{.start = {.line = 2, .column = 1}, .end = {.line = 2, .column = 7}}, "Second error");
  diag.add_warning(Source_Range{.start = {.line = 3, .column = 1}, .end = {.line = 3, .column = 7}}, "A warning");

  CHECK(diag.has_errors());  // Should have errors
  CHECK(diag.diagnostics().size() == 3);

  std::ostringstream oss;
  diag.print(oss);
  std::string const output = oss.str();

  // Expected: all three diagnostics with proper formatting
  // Note: "error1" = 6 chars, columns 1-7, highlight_len = 6 -> caret + 5 tildes
  std::string const expected =
      "multiple.life:1:1: error: First error\n"
      "    error1\n"
      "    ^~~~~~\n"
      "multiple.life:2:1: error: Second error\n"
      "    error2\n"
      "    ^~~~~~\n"
      "multiple.life:3:1: warning: A warning\n"
      "    error3\n"
      "    ^~~~~~\n";

  CHECK(output == expected);
}

TEST_CASE("Cross-Platform Line Endings", "[parser][diagnostics][line-endings]") {
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
