#include "internal_rules.hpp"
#include "utils.hpp"

using namespace life_lang::parser;
using namespace test_sexp;

namespace {

constexpr auto k_basic_input = R"(r"hello world")";
inline auto const k_basic_expected = string(R"(r"hello world")");

constexpr auto k_with_backslashes_input = R"(r"C:\path\to\file.txt")";
inline auto const k_with_backslashes_expected = string(R"(r"C:\path\to\file.txt")");

constexpr auto k_with_double_quotes_input = R"(r#"He said "hello" to me"#)";
inline auto const k_with_double_quotes_expected = string(R"(r#"He said "hello" to me"#)");

constexpr auto k_multi_line_input = R"(r"line 1
line 2
line 3")";
inline auto const k_multi_line_expected = string(R"(r"line 1
line 2
line 3")");

constexpr auto k_regex_pattern_input = R"(r"\d+\.\d+")";
inline auto const k_regex_pattern_expected = string(R"(r"\d+\.\d+")");

constexpr auto k_json_content_input = R"(r#"{"key": "value", "number": 42}"#)";
inline auto const k_json_content_expected = string(R"(r#"{"key": "value", "number": 42}"#)");

constexpr auto k_empty_input = R"(r"")";
inline auto const k_empty_expected = string(R"(r"")");

constexpr auto k_only_newlines_input = "r\"\n\n\n\"";
inline auto const k_only_newlines_expected = string("r\"\n\n\n\"");

constexpr auto k_multiple_delimiters_input = R"(r##"Contains "# and "#" patterns"##)";
inline auto const k_multiple_delimiters_expected = string(R"(r##"Contains "# and "#" patterns"##)");

constexpr auto k_no_escape_n_input = R"(r"Line 1\nLine 2")";
inline auto const k_no_escape_n_expected = string(R"(r"Line 1\nLine 2")");

constexpr auto k_no_escape_t_input = R"(r"Column1\tColumn2")";
inline auto const k_no_escape_t_expected = string(R"(r"Column1\tColumn2")");

constexpr auto k_literal_backslash_quote_input = R"(r#"Path: \"C:\Users\""#)";
inline auto const k_literal_backslash_quote_expected = string(R"(r#"Path: \"C:\Users\""#)");

constexpr auto k_windows_path_input = R"(r"C:\Users\Documents\file.txt")";
inline auto const k_windows_path_expected = string(R"(r"C:\Users\Documents\file.txt")");

}  // namespace

TEST_CASE("Raw string - valid") {
  struct Test_Case {
    std::string_view name;
    std::string_view input;
    std::string expected;
  };

  std::vector<Test_Case> const test_cases = {
      {.name = "basic", .input = k_basic_input, .expected = k_basic_expected},
      {.name = "with backslashes", .input = k_with_backslashes_input, .expected = k_with_backslashes_expected},
      {.name = "with double quotes using delimiter",
       .input = k_with_double_quotes_input,
       .expected = k_with_double_quotes_expected},
      {.name = "multi-line", .input = k_multi_line_input, .expected = k_multi_line_expected},
      {.name = "regex pattern", .input = k_regex_pattern_input, .expected = k_regex_pattern_expected},
      {.name = "JSON content", .input = k_json_content_input, .expected = k_json_content_expected},
      {.name = "empty", .input = k_empty_input, .expected = k_empty_expected},
      {.name = "only newlines", .input = k_only_newlines_input, .expected = k_only_newlines_expected},
      {.name = "multiple delimiters", .input = k_multiple_delimiters_input, .expected = k_multiple_delimiters_expected},
      {.name = "no escape processing for \\n", .input = k_no_escape_n_input, .expected = k_no_escape_n_expected},
      {.name = "no escape processing for \\t", .input = k_no_escape_t_input, .expected = k_no_escape_t_expected},
      {.name = "literal backslash-quote",
       .input = k_literal_backslash_quote_input,
       .expected = k_literal_backslash_quote_expected},
      {.name = "Windows path", .input = k_windows_path_input, .expected = k_windows_path_expected},
  };

  for (auto const& tc: test_cases) {
    SUBCASE(std::string(tc.name).c_str()) {
      life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{tc.input});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

      life_lang::parser::Parser parser{diagnostics};
      auto const expr = parser.parse_expr();
      REQUIRE(expr.has_value());
      if (expr.has_value()) {
        CHECK(to_sexp_string(*expr, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Raw string - unterminated error") {
  life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{R"(r"unterminated)"});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  CHECK_FALSE(expr.has_value());
}

TEST_CASE("Raw string - unterminated with delimiter") {
  life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{R"(r#"unterminated)"});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  CHECK_FALSE(expr.has_value());
}

TEST_CASE("Raw string - wrong delimiter count") {
  life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{R"(r##"wrong delimiter"#)"});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  CHECK_FALSE(expr.has_value());
}
