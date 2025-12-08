#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// Simple match with identifier patterns
constexpr auto k_simple_patterns_should_succeed = true;
constexpr auto k_simple_patterns_input = R"(
  match x {
    zero => "zero",
    one => "one",
    other => "other"
  }
)";
inline auto const k_simple_patterns_expected = "";

// Match with guard conditions
constexpr auto k_with_guard_should_succeed = true;
constexpr auto k_with_guard_input = R"(
  match n {
    x if x < zero => "neg",
    zero => "zero",
    other => "pos"
  }
)";
inline auto const k_with_guard_expected = "";

// Match with tuple pattern
constexpr auto k_tuple_pattern_should_succeed = true;
constexpr auto k_tuple_pattern_input = R"(
  match pair {
    (zero, zero) => "origin",
    (x, y) => format(x, y)
  }
)";
inline auto const k_tuple_pattern_expected = "";

// Match with struct pattern with field values
constexpr auto k_struct_pattern_should_succeed = true;
constexpr auto k_struct_pattern_input = R"(
  match point {
    Point { x: 0, y: 0 } => "origin",
    Point { x: 3, y: 4 } => "specific"
  }
)";
inline auto const k_struct_pattern_expected = test_json::match_expr(
    test_json::var_name("point"),
    {
        test_json::match_arm(
            test_json::struct_pattern(
                test_json::type_name("Point"),
                {
                    test_json::field_pattern("x", test_json::literal_pattern(test_json::integer("0"))),
                    test_json::field_pattern("y", test_json::literal_pattern(test_json::integer("0"))),
                }
            ),
            test_json::string(R"(\"origin\")")
        ),
        test_json::match_arm(
            test_json::struct_pattern(
                test_json::type_name("Point"),
                {
                    test_json::field_pattern("x", test_json::literal_pattern(test_json::integer("3"))),
                    test_json::field_pattern("y", test_json::literal_pattern(test_json::integer("4"))),
                }
            ),
            test_json::string(R"(\"specific\")")
        ),
    }
);

// Literal integer patterns
constexpr auto k_literal_int_should_succeed = true;
constexpr auto k_literal_int_input = R"(
  match x {
    0 => "zero",
    1 => "one",
    42 => "answer",
    100 => "century"
  }
)";
inline auto const k_literal_int_expected = test_json::match_expr(
    test_json::var_name("x"),
    {
        test_json::match_arm(test_json::literal_pattern(test_json::integer("0")), test_json::string(R"(\"zero\")")),
        test_json::match_arm(test_json::literal_pattern(test_json::integer("1")), test_json::string(R"(\"one\")")),
        test_json::match_arm(test_json::literal_pattern(test_json::integer("42")), test_json::string(R"(\"answer\")")),
        test_json::match_arm(
            test_json::literal_pattern(test_json::integer("100")),
            test_json::string(R"(\"century\")")
        ),
    }
);

// Literal string patterns
constexpr auto k_literal_string_should_succeed = true;
constexpr auto k_literal_string_input = R"(
  match name {
    "Alice" => 1,
    "Bob" => 2,
    "Charlie" => 3
  }
)";
inline auto const k_literal_string_expected = test_json::match_expr(
    test_json::var_name("name"),
    {
        test_json::match_arm(test_json::literal_pattern(test_json::string(R"(\"Alice\")")), test_json::integer("1")),
        test_json::match_arm(test_json::literal_pattern(test_json::string(R"(\"Bob\")")), test_json::integer("2")),
        test_json::match_arm(test_json::literal_pattern(test_json::string(R"(\"Charlie\")")), test_json::integer("3")),
    }
);

// Wildcard pattern
constexpr auto k_wildcard_should_succeed = true;
constexpr auto k_wildcard_input = R"(
  match x {
    0 => "zero",
    1 => "one",
    _ => "other"
  }
)";
inline auto const k_wildcard_expected = test_json::match_expr(
    test_json::var_name("x"),
    {
        test_json::match_arm(test_json::literal_pattern(test_json::integer("0")), test_json::string(R"(\"zero\")")),
        test_json::match_arm(test_json::literal_pattern(test_json::integer("1")), test_json::string(R"(\"one\")")),
        test_json::match_arm(test_json::wildcard_pattern(), test_json::string(R"(\"other\")")),
    }
);

// Mixed literal and identifier patterns
constexpr auto k_mixed_patterns_should_succeed = true;
constexpr auto k_mixed_patterns_input = R"(
  match x {
    0 => "zero",
    n => add(n, 1)
  }
)";
inline auto const k_mixed_patterns_expected = test_json::match_expr(
    test_json::var_name("x"),
    {
        test_json::match_arm(test_json::literal_pattern(test_json::integer("0")), test_json::string(R"(\"zero\")")),
        test_json::match_arm(
            test_json::simple_pattern("n"),
            test_json::function_call(test_json::var_name("add"), {test_json::var_name("n"), test_json::integer("1")})
        ),
    }
);

// Wildcard with guard (guard on previous pattern)
constexpr auto k_wildcard_guard_should_succeed = true;
constexpr auto k_wildcard_guard_input = R"(
  match x {
    n if n < 0 => "neg",
    _ => "non-neg"
  }
)";
inline auto const k_wildcard_guard_expected = test_json::match_expr(
    test_json::var_name("x"),
    {
        test_json::match_arm_with_guard(
            test_json::simple_pattern("n"),
            test_json::binary_expr("<", test_json::var_name("n"), test_json::integer("0")),
            test_json::string(R"(\"neg\")")
        ),
        test_json::match_arm(test_json::wildcard_pattern(), test_json::string(R"(\"non-neg\")")),
    }
);

// Literal in tuple pattern
constexpr auto k_literal_in_tuple_should_succeed = true;
constexpr auto k_literal_in_tuple_input = R"(
  match pair {
    (0, 0) => "origin",
    (0, y) => "y-axis",
    (x, 0) => "x-axis",
    (x, y) => "elsewhere"
  }
)";
inline auto const k_literal_in_tuple_expected = test_json::match_expr(
    test_json::var_name("pair"),
    {
        test_json::match_arm(
            test_json::tuple_pattern(
                {test_json::literal_pattern(test_json::integer("0")),
                 test_json::literal_pattern(test_json::integer("0"))}
            ),
            test_json::string(R"(\"origin\")")
        ),
        test_json::match_arm(
            test_json::tuple_pattern(
                {test_json::literal_pattern(test_json::integer("0")), test_json::simple_pattern("y")}
            ),
            test_json::string(R"(\"y-axis\")")
        ),
        test_json::match_arm(
            test_json::tuple_pattern(
                {test_json::simple_pattern("x"), test_json::literal_pattern(test_json::integer("0"))}
            ),
            test_json::string(R"(\"x-axis\")")
        ),
        test_json::match_arm(
            test_json::tuple_pattern({test_json::simple_pattern("x"), test_json::simple_pattern("y")}),
            test_json::string(R"(\"elsewhere\")")
        ),
    }
);

// Wildcard in tuple pattern
constexpr auto k_wildcard_in_tuple_should_succeed = true;
constexpr auto k_wildcard_in_tuple_input = R"(
  match pair {
    (_, _) => "any point"
  }
)";
inline auto const k_wildcard_in_tuple_expected = test_json::match_expr(
    test_json::var_name("pair"),
    {
        test_json::match_arm(
            test_json::tuple_pattern({test_json::wildcard_pattern(), test_json::wildcard_pattern()}),
            test_json::string(R"(\"any point\")")
        ),
    }
);

// String literal with guard
constexpr auto k_string_literal_guard_should_succeed = true;
constexpr auto k_string_literal_guard_input = R"(
  match x {
    "admin" if is_verified => "ok",
    "admin" => "unverified",
    _ => "unknown"
  }
)";
inline auto const k_string_literal_guard_expected = test_json::match_expr(
    test_json::var_name("x"),
    {
        test_json::match_arm_with_guard(
            test_json::literal_pattern(test_json::string(R"(\"admin\")")),
            test_json::var_name("is_verified"),
            test_json::string(R"(\"ok\")")
        ),
        test_json::match_arm(
            test_json::literal_pattern(test_json::string(R"(\"admin\")")),
            test_json::string(R"(\"unverified\")")
        ),
        test_json::match_arm(test_json::wildcard_pattern(), test_json::string(R"(\"unknown\")")),
    }
);

// Multiple wildcards in different positions
constexpr auto k_multiple_wildcards_should_succeed = true;
constexpr auto k_multiple_wildcards_input = R"(
  match pair {
    (0, _) => "first zero",
    (_, 0) => "second zero",
    (_, _) => "neither"
  }
)";
inline auto const k_multiple_wildcards_expected = test_json::match_expr(
    test_json::var_name("pair"),
    {
        test_json::match_arm(
            test_json::tuple_pattern(
                {test_json::literal_pattern(test_json::integer("0")), test_json::wildcard_pattern()}
            ),
            test_json::string(R"(\"first zero\")")
        ),
        test_json::match_arm(
            test_json::tuple_pattern(
                {test_json::wildcard_pattern(), test_json::literal_pattern(test_json::integer("0"))}
            ),
            test_json::string(R"(\"second zero\")")
        ),
        test_json::match_arm(
            test_json::tuple_pattern({test_json::wildcard_pattern(), test_json::wildcard_pattern()}),
            test_json::string(R"(\"neither\")")
        ),
    }
);

// Match with trailing comma
constexpr auto k_trailing_comma_should_succeed = true;
constexpr auto k_trailing_comma_input = R"(
  match x {
    zero => "zero",
    one => "one",
  }
)";
inline auto const k_trailing_comma_expected = "";

// Single arm
constexpr auto k_single_arm_should_succeed = true;
constexpr auto k_single_arm_input = R"(
  match x {
    any => 42
  }
)";
inline auto const k_single_arm_expected = "";

// Nested match
constexpr auto k_nested_match_should_succeed = true;
constexpr auto k_nested_match_input = R"(
  match x {
    zero => match y {
      one => "a",
      other => "b"
    },
    other => "c"
  }
)";
inline auto const k_nested_match_expected = "";

// Struct pattern with identifier bindings
constexpr auto k_struct_with_bindings_should_succeed = true;
constexpr auto k_struct_with_bindings_input = R"(
  match point {
    Point { x: px, y: py } => add(px, py)
  }
)";
inline auto const k_struct_with_bindings_expected = test_json::match_expr(
    test_json::var_name("point"),
    {
        test_json::match_arm(
            test_json::struct_pattern(
                test_json::type_name("Point"),
                {
                    test_json::field_pattern("x", test_json::simple_pattern("px")),
                    test_json::field_pattern("y", test_json::simple_pattern("py")),
                }
            ),
            test_json::function_call(test_json::var_name("add"), {test_json::var_name("px"), test_json::var_name("py")})
        ),
    }
);

// Struct pattern with wildcard fields
constexpr auto k_struct_with_wildcard_should_succeed = true;
constexpr auto k_struct_with_wildcard_input = R"(
  match point {
    Point { x: _, y: 0 } => "on x-axis"
  }
)";
inline auto const k_struct_with_wildcard_expected = test_json::match_expr(
    test_json::var_name("point"),
    {
        test_json::match_arm(
            test_json::struct_pattern(
                test_json::type_name("Point"),
                {
                    test_json::field_pattern("x", test_json::wildcard_pattern()),
                    test_json::field_pattern("y", test_json::literal_pattern(test_json::integer("0"))),
                }
            ),
            test_json::string(R"(\"on x-axis\")")
        ),
    }
);

// Nested struct pattern
constexpr auto k_nested_struct_should_succeed = true;
constexpr auto k_nested_struct_input = R"(
  match line {
    Line { start: Point { x: 0, y: 0 }, end: p } => process(p)
  }
)";
inline auto const k_nested_struct_expected = test_json::match_expr(
    test_json::var_name("line"),
    {
        test_json::match_arm(
            test_json::struct_pattern(
                test_json::type_name("Line"),
                {
                    test_json::field_pattern(
                        "start",
                        test_json::struct_pattern(
                            test_json::type_name("Point"),
                            {
                                test_json::field_pattern("x", test_json::literal_pattern(test_json::integer("0"))),
                                test_json::field_pattern("y", test_json::literal_pattern(test_json::integer("0"))),
                            }
                        )
                    ),
                    test_json::field_pattern("end", test_json::simple_pattern("p")),
                }
            ),
            test_json::function_call(test_json::var_name("process"), {test_json::var_name("p")})
        ),
    }
);

// Struct pattern with shorthand syntax
constexpr auto k_struct_shorthand_should_succeed = true;
constexpr auto k_struct_shorthand_input = R"(
  match point {
    Point { x, y } => add(x, y)
  }
)";
inline auto const k_struct_shorthand_expected = test_json::match_expr(
    test_json::var_name("point"),
    {
        test_json::match_arm(
            test_json::struct_pattern(
                test_json::type_name("Point"),
                {
                    test_json::field_pattern("x", test_json::simple_pattern("x")),
                    test_json::field_pattern("y", test_json::simple_pattern("y")),
                }
            ),
            test_json::function_call(test_json::var_name("add"), {test_json::var_name("x"), test_json::var_name("y")})
        ),
    }
);

// Mixed shorthand and explicit field patterns
constexpr auto k_struct_mixed_should_succeed = true;
constexpr auto k_struct_mixed_input = R"(
  match point {
    Point { x, y: 0 } => "on x-axis",
    Point { x: 0, y } => "on y-axis"
  }
)";
inline auto const k_struct_mixed_expected = test_json::match_expr(
    test_json::var_name("point"),
    {
        test_json::match_arm(
            test_json::struct_pattern(
                test_json::type_name("Point"),
                {
                    test_json::field_pattern("x", test_json::simple_pattern("x")),
                    test_json::field_pattern("y", test_json::literal_pattern(test_json::integer("0"))),
                }
            ),
            test_json::string(R"(\"on x-axis\")")
        ),
        test_json::match_arm(
            test_json::struct_pattern(
                test_json::type_name("Point"),
                {
                    test_json::field_pattern("x", test_json::literal_pattern(test_json::integer("0"))),
                    test_json::field_pattern("y", test_json::simple_pattern("y")),
                }
            ),
            test_json::string(R"(\"on y-axis\")")
        ),
    }
);

// Complex guard with logical operators
constexpr auto k_complex_guard_should_succeed = true;
constexpr auto k_complex_guard_input = R"(
  match n {
    x if x > zero && x < ten => "single",
    other => "other"
  }
)";
inline auto const k_complex_guard_expected = "";

// Invalid: missing arrow
constexpr auto k_missing_arrow_should_succeed = false;
constexpr auto k_missing_arrow_input = R"(
  match x {
    zero "zero"
  }
)";
inline auto const k_missing_arrow_expected = "";

// Invalid: missing result
constexpr auto k_missing_result_should_succeed = false;
constexpr auto k_missing_result_input = R"(
  match x {
    zero =>
  }
)";
inline auto const k_missing_result_expected = "";

// Invalid: missing closing brace
constexpr auto k_missing_brace_should_succeed = false;
constexpr auto k_missing_brace_input = R"(
  match x {
    zero => "zero"
)";
inline auto const k_missing_brace_expected = "";

}  // namespace

TEST_CASE("Parse Match_Expr", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          // Basic patterns
          {"simple patterns", k_simple_patterns_input, k_simple_patterns_expected, k_simple_patterns_should_succeed},
          {"with guard", k_with_guard_input, k_with_guard_expected, k_with_guard_should_succeed},
          {"tuple pattern", k_tuple_pattern_input, k_tuple_pattern_expected, k_tuple_pattern_should_succeed},
          {"struct pattern", k_struct_pattern_input, k_struct_pattern_expected, k_struct_pattern_should_succeed},
          {"struct with bindings",
           k_struct_with_bindings_input,
           k_struct_with_bindings_expected,
           k_struct_with_bindings_should_succeed},
          {"struct with wildcard",
           k_struct_with_wildcard_input,
           k_struct_with_wildcard_expected,
           k_struct_with_wildcard_should_succeed},
          {"nested struct", k_nested_struct_input, k_nested_struct_expected, k_nested_struct_should_succeed},
          {"struct shorthand",
           k_struct_shorthand_input,
           k_struct_shorthand_expected,
           k_struct_shorthand_should_succeed},
          {"struct mixed", k_struct_mixed_input, k_struct_mixed_expected, k_struct_mixed_should_succeed},
          {"trailing comma", k_trailing_comma_input, k_trailing_comma_expected, k_trailing_comma_should_succeed},
          {"single arm", k_single_arm_input, k_single_arm_expected, k_single_arm_should_succeed},
          {"nested match", k_nested_match_input, k_nested_match_expected, k_nested_match_should_succeed},
          {"complex guard", k_complex_guard_input, k_complex_guard_expected, k_complex_guard_should_succeed},

          // Literal patterns
          {"literal integers", k_literal_int_input, k_literal_int_expected, k_literal_int_should_succeed},
          {"literal strings", k_literal_string_input, k_literal_string_expected, k_literal_string_should_succeed},
          {"mixed patterns", k_mixed_patterns_input, k_mixed_patterns_expected, k_mixed_patterns_should_succeed},
          {"literal in tuple",
           k_literal_in_tuple_input,
           k_literal_in_tuple_expected,
           k_literal_in_tuple_should_succeed},
          {"string literal with guard",
           k_string_literal_guard_input,
           k_string_literal_guard_expected,
           k_string_literal_guard_should_succeed},

          // Wildcard patterns
          {"wildcard", k_wildcard_input, k_wildcard_expected, k_wildcard_should_succeed},
          {"wildcard with guard", k_wildcard_guard_input, k_wildcard_guard_expected, k_wildcard_guard_should_succeed},
          {"wildcard in tuple",
           k_wildcard_in_tuple_input,
           k_wildcard_in_tuple_expected,
           k_wildcard_in_tuple_should_succeed},
          {"multiple wildcards",
           k_multiple_wildcards_input,
           k_multiple_wildcards_expected,
           k_multiple_wildcards_should_succeed},

          // Invalid cases
          {"missing arrow", k_missing_arrow_input, k_missing_arrow_expected, k_missing_arrow_should_succeed},
          {"missing result", k_missing_result_input, k_missing_result_expected, k_missing_result_should_succeed},
          {"missing brace", k_missing_brace_input, k_missing_brace_expected, k_missing_brace_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
