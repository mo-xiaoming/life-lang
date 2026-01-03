#include "internal_rules.hpp"
#include "utils.hpp"

using namespace life_lang::parser;
using namespace test_sexp;

namespace {

constexpr auto k_single_variable_input = R"("value: {x}")";
inline auto const k_single_variable_expected = string_interp({string_part("value: "), var_name("x")});

constexpr auto k_multiple_variables_input = R"xxx("({x}, {y})")xxx";
inline auto const k_multiple_variables_expected =
    string_interp({string_part("("), var_name("x"), string_part(", "), var_name("y"), string_part(")")});

constexpr auto k_expression_input = R"("result: {1 + 2}")";
inline auto const k_expression_expected =
    string_interp({string_part("result: "), binary_expr("+", integer("1"), integer("2"))});

constexpr auto k_field_access_input = R"("name: {user.name}")";
inline auto const k_field_access_expected =
    string_interp({string_part("name: "), field_access(var_name("user"), "name")});

constexpr auto k_function_call_input = R"("result: {calculate(x, y)}")";
inline auto const k_function_call_expected =
    string_interp({string_part("result: "), function_call(var_name("calculate"), {var_name("x"), var_name("y")})});

constexpr auto k_method_call_input = R"("upper: {name.to_upper()}")";
inline auto const k_method_call_expected =
    string_interp({string_part("upper: "), function_call(var_name_path({"name", "to_upper"}), {})});

constexpr auto k_cast_expression_input = R"("value: {x as I64}")";
inline auto const k_cast_expression_expected =
    string_interp({string_part("value: "), cast_expr(var_name("x"), type_name("I64"))});

constexpr auto k_nested_expression_input = R"("total: {(a + b) * c}")";
inline auto const k_nested_expression_expected = string_interp(
    {string_part("total: "), binary_expr("*", binary_expr("+", var_name("a"), var_name("b")), var_name("c"))}
);

constexpr auto k_with_escape_sequences_input = R"("path: {path}\n")";
inline auto const k_with_escape_sequences_expected =
    string_interp({string_part("path: "), var_name("path"), string_part("\\n")});

constexpr auto k_starting_with_expression_input = R"("{x} is the value")";
inline auto const k_starting_with_expression_expected = string_interp({var_name("x"), string_part(" is the value")});

constexpr auto k_ending_with_expression_input = R"("value is {x}")";
inline auto const k_ending_with_expression_expected = string_interp({string_part("value is "), var_name("x")});

constexpr auto k_only_expression_input = R"("{x}")";
inline auto const k_only_expression_expected = string_interp({var_name("x")});

constexpr auto k_adjacent_expressions_input = R"("{x}{y}")";
inline auto const k_adjacent_expressions_expected = string_interp({var_name("x"), var_name("y")});

constexpr auto k_empty_braces_input = R"("{}")";
inline auto const k_empty_braces_expected = string("\"{}\"");

constexpr auto k_format_placeholders_input = R"xxx("({}, {})")xxx";
inline auto const k_format_placeholders_expected = string("\"({}, {})\"");

constexpr auto k_no_interpolation_input = R"("plain string")";
inline auto const k_no_interpolation_expected = string("\"plain string\"");

constexpr auto k_escaped_braces_input = R"("Literal: \{not interpolated\}")";
inline auto const k_escaped_braces_expected = string(R"("Literal: \{not interpolated\}")");

constexpr auto k_mixed_escaped_input = R"("Escaped: \{literal\}, Interpolated: {x}")";
inline auto const k_mixed_escaped_expected =
    string_interp({string_part("Escaped: \\{literal\\}, Interpolated: "), var_name("x")});

constexpr auto k_json_escaped_input = R"("JSON: \{\"key\": \"value\"\}")";
inline auto const k_json_escaped_expected = string(R"("JSON: \{\"key\": \"value\"\}")");

constexpr auto k_comparison_expression_input = R"("check: {x == y}")";
inline auto const k_comparison_expression_expected =
    string_interp({string_part("check: "), binary_expr("==", var_name("x"), var_name("y"))});

constexpr auto k_array_index_input = R"("item: {items[0]}")";
inline auto const k_array_index_expected =
    string_interp({string_part("item: "), index_expr(var_name("items"), integer("0"))});

}  // namespace

TEST_CASE("String interpolation") {
  struct Test_Case {
    std::string_view name;
    std::string_view input;
    std::string expected;
  };

  std::vector<Test_Case> const test_cases = {
      {.name = "single variable", .input = k_single_variable_input, .expected = k_single_variable_expected},
      {.name = "multiple variables", .input = k_multiple_variables_input, .expected = k_multiple_variables_expected},
      {.name = "expression", .input = k_expression_input, .expected = k_expression_expected},
      {.name = "field access", .input = k_field_access_input, .expected = k_field_access_expected},
      {.name = "function call", .input = k_function_call_input, .expected = k_function_call_expected},
      {.name = "method call", .input = k_method_call_input, .expected = k_method_call_expected},
      {.name = "cast expression", .input = k_cast_expression_input, .expected = k_cast_expression_expected},
      {.name = "nested expression", .input = k_nested_expression_input, .expected = k_nested_expression_expected},
      {.name = "with escape sequences",
       .input = k_with_escape_sequences_input,
       .expected = k_with_escape_sequences_expected},
      {.name = "starting with expression",
       .input = k_starting_with_expression_input,
       .expected = k_starting_with_expression_expected},
      {.name = "ending with expression",
       .input = k_ending_with_expression_input,
       .expected = k_ending_with_expression_expected},
      {.name = "only expression", .input = k_only_expression_input, .expected = k_only_expression_expected},
      {.name = "adjacent expressions",
       .input = k_adjacent_expressions_input,
       .expected = k_adjacent_expressions_expected},
      {.name = "empty string with expression (empty braces)",
       .input = k_empty_braces_input,
       .expected = k_empty_braces_expected},
      {.name = "format placeholders (not interpolation)",
       .input = k_format_placeholders_input,
       .expected = k_format_placeholders_expected},
      {.name = "no interpolation", .input = k_no_interpolation_input, .expected = k_no_interpolation_expected},
      {.name = "escaped braces - literal braces",
       .input = k_escaped_braces_input,
       .expected = k_escaped_braces_expected},
      {.name = "mixed escaped and interpolated braces",
       .input = k_mixed_escaped_input,
       .expected = k_mixed_escaped_expected},
      {.name = "only opening escaped brace (JSON)", .input = k_json_escaped_input, .expected = k_json_escaped_expected},
      {.name = "comparison expression",
       .input = k_comparison_expression_input,
       .expected = k_comparison_expression_expected},
      {.name = "array index", .input = k_array_index_input, .expected = k_array_index_expected},
  };

  for (auto const& tc: test_cases) {
    SUBCASE(std::string(tc.name).c_str()) {
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

TEST_CASE("String interpolation - tuple access") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", R"("first: {pair.0}")"};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = string_interp({string_part("first: "), field_access(var_name("pair"), "0")});
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("String interpolation - complex expression") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", R"("value: {data.items[index].name.to_upper()}")"};

  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const output = to_sexp_string(*expr, 0);
    // Verify it's a string_interp with the complex chain
    CHECK(output.starts_with("(string_interp "));
    CHECK(output.find("data") != std::string::npos);
    CHECK(output.find("items") != std::string::npos);
    CHECK(output.find("index") != std::string::npos);
    CHECK(output.find("name") != std::string::npos);
    CHECK(output.find("to_upper") != std::string::npos);
  }
}
