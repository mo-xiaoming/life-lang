#include "internal_rules.hpp"
#include "utils.hpp"

using namespace life_lang::parser;
using namespace test_sexp;

namespace {

constexpr auto k_type_only_input = "Point { .. }";
inline auto const k_type_only_expected = struct_pattern_with_rest(type_name("Point"), {});

constexpr auto k_single_field_input = "User { name, .. }";
inline auto const k_single_field_expected =
    struct_pattern_with_rest(type_name("User"), {field_pattern("name", simple_pattern("name"))});

constexpr auto k_multiple_fields_input = "Config { host, port, .. }";
inline auto const k_multiple_fields_expected = struct_pattern_with_rest(
    type_name("Config"),
    {field_pattern("host", simple_pattern("host")), field_pattern("port", simple_pattern("port"))}
);

constexpr auto k_explicit_binding_input = "Request { method: m, url: u, .. }";
inline auto const k_explicit_binding_expected = struct_pattern_with_rest(
    type_name("Request"),
    {field_pattern("method", simple_pattern("m")), field_pattern("url", simple_pattern("u"))}
);

constexpr auto k_trailing_comma_input = "User { name, age, .. }";
inline auto const k_trailing_comma_expected = struct_pattern_with_rest(
    type_name("User"),
    {field_pattern("name", simple_pattern("name")), field_pattern("age", simple_pattern("age"))}
);

constexpr auto k_no_rest_input = "Point { x, y }";
inline auto const k_no_rest_expected = struct_pattern(
    type_name("Point"),
    {field_pattern("x", simple_pattern("x")), field_pattern("y", simple_pattern("y"))}
);

}  // namespace

TEST_CASE("Rest patterns") {
  struct Test_Case {
    std::string_view name;
    std::string_view input;
    std::string expected;
  };

  std::vector<Test_Case> const test_cases = {
      {.name = "type-only matching (empty fields with ..)",
       .input = k_type_only_input,
       .expected = k_type_only_expected},
      {.name = "single field with rest", .input = k_single_field_input, .expected = k_single_field_expected},
      {.name = "multiple fields with rest", .input = k_multiple_fields_input, .expected = k_multiple_fields_expected},
      {.name = "with explicit pattern binding",
       .input = k_explicit_binding_input,
       .expected = k_explicit_binding_expected},
      {.name = "trailing comma before ..", .input = k_trailing_comma_input, .expected = k_trailing_comma_expected},
      {.name = "no rest (all fields explicit)", .input = k_no_rest_input, .expected = k_no_rest_expected},
  };

  for (auto const& tc: test_cases) {
    SUBCASE(std::string(tc.name).c_str()) {
      life_lang::Diagnostic_Engine diagnostics{"<test>", tc.input};

      life_lang::parser::Parser parser{diagnostics};
      auto const pattern = parser.parse_pattern();
      REQUIRE(pattern.has_value());
      if (pattern.has_value()) {
        CHECK(to_sexp_string(*pattern, 0) == tc.expected);
      }
    }
  }
}

TEST_CASE("Rest pattern - error: .. not at end") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", "Config { .., debug }"};

  life_lang::parser::Parser parser{diagnostics};
  auto const pattern = parser.parse_pattern();
  // Parser should accept .. anywhere for now, semantic analysis will enforce position
  // Actually, the parser enforces that .. must be last
  CHECK_FALSE(pattern.has_value());
}

TEST_CASE("Rest pattern - error: comma after ..") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", "User { name, .., }"};

  life_lang::parser::Parser parser{diagnostics};
  auto const pattern = parser.parse_pattern();
  CHECK_FALSE(pattern.has_value());
}

TEST_CASE("Rest pattern - in match expression") {
  constexpr std::string_view k_input = R"(
    match value {
      Point { .. } => "point",
      Circle { radius, .. } => "circle",
    }
  )";
  life_lang::Diagnostic_Engine diagnostics{"<test>", k_input};
  life_lang::parser::Parser parser{diagnostics};
  auto const expr = parser.parse_expr();
  REQUIRE(expr.has_value());
  if (expr.has_value()) {
    auto const expected = match_expr(
        var_name("value"),
        {match_arm(struct_pattern_with_rest(type_name("Point"), {}), string(R"("point")")),
         match_arm(
             struct_pattern_with_rest(type_name("Circle"), {field_pattern("radius", simple_pattern("radius"))}),
             string(R"("circle")")
         )}
    );
    CHECK(to_sexp_string(*expr, 0) == expected);
  }
}

TEST_CASE("Rest pattern - nested in tuple pattern") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", "(Point { x, .. }, Circle { .. })"};

  life_lang::parser::Parser parser{diagnostics};
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = tuple_pattern(
        {struct_pattern_with_rest(type_name("Point"), {field_pattern("x", simple_pattern("x"))}),
         struct_pattern_with_rest(type_name("Circle"), {})}
    );
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}

TEST_CASE("Rest pattern - with nested patterns") {
  life_lang::Diagnostic_Engine diagnostics{"<test>", "Request { method, body: Some(data), .. }"};

  life_lang::parser::Parser parser{diagnostics};
  auto const pattern = parser.parse_pattern();
  REQUIRE(pattern.has_value());
  if (pattern.has_value()) {
    auto const expected = struct_pattern_with_rest(
        type_name("Request"),
        {field_pattern("method", simple_pattern("method")),
         field_pattern("body", enum_pattern(type_name("Some"), {simple_pattern("data")}))}
    );
    CHECK(to_sexp_string(*pattern, 0) == expected);
  }
}
