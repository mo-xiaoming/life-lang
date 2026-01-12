#include "internal_rules.hpp"
#include "utils.hpp"

using namespace life_lang::parser;
using namespace test_sexp;

namespace {

// Simple literals
constexpr auto k_simple_literals_input = "1 | 2 | 3";
inline auto const k_simple_literals_expected =
    or_pattern({literal_pattern(integer("1")), literal_pattern(integer("2")), literal_pattern(integer("3"))});

// Variable names
constexpr auto k_variable_names_input = "x | y | z";
inline auto const k_variable_names_expected =
    or_pattern({simple_pattern("x"), simple_pattern("y"), simple_pattern("z")});

// Two alternatives
constexpr auto k_two_alternatives_input = "true | false";
inline auto const k_two_alternatives_expected =
    or_pattern({literal_pattern("(bool true)"), literal_pattern("(bool false)")});

// Nested in tuple
constexpr auto k_nested_in_tuple_input = "(1 | 2, 3 | 4)";
inline auto const k_nested_in_tuple_expected = tuple_pattern(
    {or_pattern({literal_pattern(integer("1")), literal_pattern(integer("2"))}),
     or_pattern({literal_pattern(integer("3")), literal_pattern(integer("4"))})}
);

// Wildcard alternatives
constexpr auto k_wildcard_alternatives_input = "_ | x";
inline auto const k_wildcard_alternatives_expected = or_pattern({wildcard_pattern(), simple_pattern("x")});

// In let statement
constexpr auto k_in_let_statement_input = "let x | y = value;";
inline auto const k_in_let_statement_pattern_expected = or_pattern({simple_pattern("x"), simple_pattern("y")});
inline auto const k_in_let_statement_expected = let_statement(k_in_let_statement_pattern_expected, var_name("value"));

// Strings
constexpr auto k_strings_input = R"("hello" | "world")";
inline auto const k_strings_expected =
    or_pattern({literal_pattern(string(R"("hello")")), literal_pattern(string(R"("world")"))});

// Single pattern (no |)
constexpr auto k_single_pattern_input = "42";
inline auto const k_single_pattern_expected = literal_pattern(integer("42"));  // Should NOT be wrapped in or_pattern

// Enum variants (future: requires type names)
constexpr auto k_enum_variants_input = "Some | None";
inline auto const k_enum_variants_expected = or_pattern({simple_pattern("Some"), simple_pattern("None")});

// Four alternatives
constexpr auto k_four_alternatives_input = "1 | 2 | 3 | 4";
inline auto const k_four_alternatives_expected = or_pattern(
    {literal_pattern(integer("1")),
     literal_pattern(integer("2")),
     literal_pattern(integer("3")),
     literal_pattern(integer("4"))}
);

// Mixed types (semantic analysis will reject)
constexpr auto k_mixed_types_input = R"(1 | "hello")";
inline auto const k_mixed_types_expected =
    or_pattern({literal_pattern(integer("1")), literal_pattern(string(R"("hello")"))});

}  // namespace

TEST_CASE("Or patterns") {
  struct Test_Case {
    std::string_view name;
    std::string_view input;
    std::string expected;
  };

  std::vector<Test_Case> const test_cases = {
      {.name = "simple literals", .input = k_simple_literals_input, .expected = k_simple_literals_expected},
      {.name = "variable names", .input = k_variable_names_input, .expected = k_variable_names_expected},
      {.name = "two alternatives", .input = k_two_alternatives_input, .expected = k_two_alternatives_expected},
      {.name = "nested in tuple", .input = k_nested_in_tuple_input, .expected = k_nested_in_tuple_expected},
      {.name = "wildcard alternatives",
       .input = k_wildcard_alternatives_input,
       .expected = k_wildcard_alternatives_expected},
      {.name = "strings", .input = k_strings_input, .expected = k_strings_expected},
      {.name = "single pattern (no |)", .input = k_single_pattern_input, .expected = k_single_pattern_expected},
      {.name = "enum variants", .input = k_enum_variants_input, .expected = k_enum_variants_expected},
      {.name = "four alternatives", .input = k_four_alternatives_input, .expected = k_four_alternatives_expected},
      {.name = "mixed types", .input = k_mixed_types_input, .expected = k_mixed_types_expected},
  };

  for (auto const& tc: test_cases) {
    SUBCASE(std::string(tc.name).c_str()) {
      life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{tc.input});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

      life_lang::parser::Parser parser{diagnostics};
      auto const pattern = parser.parse_pattern();
      REQUIRE(pattern.has_value());
      if (pattern.has_value()) {
        CHECK(to_sexp_string(*pattern, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Or pattern - in let statement") {
  life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{k_in_let_statement_input});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

  life_lang::parser::Parser parser{diagnostics};
  auto const stmt = parser.parse_statement();
  REQUIRE(stmt.has_value());
  if (stmt.has_value()) {
    CHECK(to_sexp_string(*stmt, 0) == k_in_let_statement_expected);
  }
}
