#include <doctest/doctest.h>
#include <sstream>

#include "diagnostics.hpp"

using life_lang::Diagnostic_Engine;
using life_lang::Source_Range;

// ============================================================================
// Source Line Retrieval Tests
// ============================================================================

TEST_CASE("Diagnostic Source Line Retrieval") {
  std::string const source =
      "line 1\n"
      "line 2\n"
      "line 3\n";

  Diagnostic_Engine const diag("test.life", source);

  CHECK(diag.get_line(1) == "line 1");
  CHECK(diag.get_line(2) == "line 2");
  CHECK(diag.get_line(3) == "line 3");
}

// ============================================================================
// Range Highlighting Tests
// ============================================================================

TEST_CASE("Diagnostic Range Highlighting") {
  SUBCASE("Single-line error with specific range") {
    std::string const source = "fn main() { bad_syntax }";
    Diagnostic_Engine diag("test.life", source);

    // Simulate error on "bad_syntax" (columns 13-23)
    Source_Range const range{.start = {.line = 1, .column = 13}, .end = {.line = 1, .column = 23}};
    diag.add_error(range, "Unknown variable_name");

    std::ostringstream oss;
    diag.print(oss);
    std::string const output = oss.str();

    // Expected: caret at start, tildes under error range
    std::string const expected =
        "test.life:1:13: error: Unknown variable_name\n"
        "    fn main() { bad_syntax }\n"
        "                ^~~~~~~~~~\n";

    CHECK(output == expected);
  }

  SUBCASE("Single-character error") {
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

  SUBCASE("Multi-line error range") {
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

  SUBCASE("Multi(>2)-line error") {
    std::string const source =
        "line 1 content\n"
        "line 2 content\n"
        "line 3 content\n"
        "line 4 content\n"
        "line 5 content\n";

    Diagnostic_Engine diag("long_error.life", source);

    // Error spanning lines 2-4 (line 4 is 14 chars, so end at column 15 to include all)
    Source_Range const range{.start = {.line = 2, .column = 1}, .end = {.line = 4, .column = 15}};
    diag.add_error(range, "Multi-line error example");

    std::ostringstream oss;
    diag.print(oss);
    std::string const output = oss.str();

    // Expected: full highlight on first and last lines, "..." for middle lines
    std::string const expected =
        "long_error.life:2:1: error: Multi-line error example\n"
        "    line 2 content\n"
        "    ^~~~~~~~~~~~~~\n"
        "    ...\n"
        "    line 4 content\n"
        "    ~~~~~~~~~~~~~^\n";

    CHECK(output == expected);
  }

  SUBCASE("Error at line start") {
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

// ============================================================================
// Multiple Diagnostics Tests
// ============================================================================

TEST_CASE("Multiple Diagnostics") {
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

// ============================================================================
// Diagnostic State Tests
// ============================================================================

TEST_CASE("Diagnostic State Management") {
  SUBCASE("Empty diagnostic engine has no errors") {
    std::string const source = "some source code";
    Diagnostic_Engine const diag("test.life", source);

    CHECK_FALSE(diag.has_errors());
    CHECK(diag.diagnostics().empty());
  }

  SUBCASE("Adding error sets has_errors") {
    std::string const source = "some source code";
    Diagnostic_Engine diag("test.life", source);

    diag.add_error(Source_Range{.start = {.line = 1, .column = 1}, .end = {.line = 1, .column = 5}}, "Test error");

    CHECK(diag.has_errors());
    CHECK(diag.diagnostics().size() == 1);
  }

  SUBCASE("Adding warning does not set has_errors") {
    std::string const source = "some source code";
    Diagnostic_Engine diag("test.life", source);

    diag.add_warning(Source_Range{.start = {.line = 1, .column = 1}, .end = {.line = 1, .column = 5}}, "Test warning");

    CHECK_FALSE(diag.has_errors());
    CHECK(diag.diagnostics().size() == 1);
  }

  SUBCASE("Filename stored correctly") {
    std::string const source = "source";
    Diagnostic_Engine const diag("custom.life", source);

    CHECK(diag.filename() == "custom.life");
  }

  SUBCASE("Anonymous filename") {
    std::string const source = "source";
    Diagnostic_Engine const diag("<input>", source);

    CHECK(diag.filename() == "<input>");
  }
}
