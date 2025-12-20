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
inline auto const k_simple_patterns_expected = test_sexp::match_expr(
    test_sexp::var_name("x"),
    {test_sexp::match_arm(test_sexp::simple_pattern("zero"), test_sexp::string(R"("zero")")),
     test_sexp::match_arm(test_sexp::simple_pattern("one"), test_sexp::string(R"("one")")),
     test_sexp::match_arm(test_sexp::simple_pattern("other"), test_sexp::string(R"("other")"))}
);

// Match with guard conditions
constexpr auto k_with_guard_should_succeed = true;
constexpr auto k_with_guard_input = R"(
  match n {
    x if x < zero => "neg",
    zero => "zero",
    other => "pos"
  }
)";
inline auto const k_with_guard_expected = test_sexp::match_expr(
    test_sexp::var_name("n"),
    {test_sexp::match_arm_with_guard(
         test_sexp::simple_pattern("x"),
         test_sexp::binary_expr("<", test_sexp::var_name("x"), test_sexp::var_name("zero")),
         test_sexp::string(R"("neg")")
     ),
     test_sexp::match_arm(test_sexp::simple_pattern("zero"), test_sexp::string(R"("zero")")),
     test_sexp::match_arm(test_sexp::simple_pattern("other"), test_sexp::string(R"("pos")"))}
);

// Match with tuple pattern
constexpr auto k_tuple_pattern_should_succeed = true;
constexpr auto k_tuple_pattern_input = R"(
  match pair {
    (zero, zero) => "origin",
    (x, y) => format(x, y)
  }
)";
inline auto const k_tuple_pattern_expected = test_sexp::match_expr(
    test_sexp::var_name("pair"),
    {test_sexp::match_arm(
         test_sexp::tuple_pattern({test_sexp::simple_pattern("zero"), test_sexp::simple_pattern("zero")}),
         test_sexp::string(R"("origin")")
     ),
     test_sexp::match_arm(
         test_sexp::tuple_pattern({test_sexp::simple_pattern("x"), test_sexp::simple_pattern("y")}),
         test_sexp::function_call(test_sexp::var_name("format"), {test_sexp::var_name("x"), test_sexp::var_name("y")})
     )}
);

// Match with struct pattern with field values
constexpr auto k_struct_pattern_should_succeed = true;
constexpr auto k_struct_pattern_input = R"(
  match point {
    Point { x: 0, y: 0 } => "origin",
    Point { x: 3, y: 4 } => "specific"
  }
)";
inline auto const k_struct_pattern_expected = test_sexp::match_expr(
    test_sexp::var_name("point"),
    {
        test_sexp::match_arm(
            test_sexp::struct_pattern(
                test_sexp::type_name("Point"),
                {
                    test_sexp::field_pattern("x", test_sexp::literal_pattern(test_sexp::integer("0"))),
                    test_sexp::field_pattern("y", test_sexp::literal_pattern(test_sexp::integer("0"))),
                }
            ),
            test_sexp::string(R"("origin")")
        ),
        test_sexp::match_arm(
            test_sexp::struct_pattern(
                test_sexp::type_name("Point"),
                {
                    test_sexp::field_pattern("x", test_sexp::literal_pattern(test_sexp::integer("3"))),
                    test_sexp::field_pattern("y", test_sexp::literal_pattern(test_sexp::integer("4"))),
                }
            ),
            test_sexp::string(R"("specific")")
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
inline auto const k_literal_int_expected = test_sexp::match_expr(
    test_sexp::var_name("x"),
    {
        test_sexp::match_arm(test_sexp::literal_pattern(test_sexp::integer("0")), test_sexp::string(R"("zero")")),
        test_sexp::match_arm(test_sexp::literal_pattern(test_sexp::integer("1")), test_sexp::string(R"("one")")),
        test_sexp::match_arm(test_sexp::literal_pattern(test_sexp::integer("42")), test_sexp::string(R"("answer")")),
        test_sexp::match_arm(test_sexp::literal_pattern(test_sexp::integer("100")), test_sexp::string(R"("century")")),
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
inline auto const k_literal_string_expected = test_sexp::match_expr(
    test_sexp::var_name("name"),
    {
        test_sexp::match_arm(test_sexp::literal_pattern(test_sexp::string(R"("Alice")")), test_sexp::integer("1")),
        test_sexp::match_arm(test_sexp::literal_pattern(test_sexp::string(R"("Bob")")), test_sexp::integer("2")),
        test_sexp::match_arm(test_sexp::literal_pattern(test_sexp::string(R"("Charlie")")), test_sexp::integer("3")),
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
inline auto const k_wildcard_expected = test_sexp::match_expr(
    test_sexp::var_name("x"),
    {
        test_sexp::match_arm(test_sexp::literal_pattern(test_sexp::integer("0")), test_sexp::string(R"("zero")")),
        test_sexp::match_arm(test_sexp::literal_pattern(test_sexp::integer("1")), test_sexp::string(R"("one")")),
        test_sexp::match_arm(test_sexp::wildcard_pattern(), test_sexp::string(R"("other")")),
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
inline auto const k_mixed_patterns_expected = test_sexp::match_expr(
    test_sexp::var_name("x"),
    {
        test_sexp::match_arm(test_sexp::literal_pattern(test_sexp::integer("0")), test_sexp::string(R"("zero")")),
        test_sexp::match_arm(
            test_sexp::simple_pattern("n"),
            test_sexp::function_call(test_sexp::var_name("add"), {test_sexp::var_name("n"), test_sexp::integer("1")})
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
inline auto const k_wildcard_guard_expected = test_sexp::match_expr(
    test_sexp::var_name("x"),
    {
        test_sexp::match_arm_with_guard(
            test_sexp::simple_pattern("n"),
            test_sexp::binary_expr("<", test_sexp::var_name("n"), test_sexp::integer("0")),
            test_sexp::string(R"("neg")")
        ),
        test_sexp::match_arm(test_sexp::wildcard_pattern(), test_sexp::string(R"("non-neg")")),
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
inline auto const k_literal_in_tuple_expected = test_sexp::match_expr(
    test_sexp::var_name("pair"),
    {
        test_sexp::match_arm(
            test_sexp::tuple_pattern(
                {test_sexp::literal_pattern(test_sexp::integer("0")),
                 test_sexp::literal_pattern(test_sexp::integer("0"))}
            ),
            test_sexp::string(R"("origin")")
        ),
        test_sexp::match_arm(
            test_sexp::tuple_pattern(
                {test_sexp::literal_pattern(test_sexp::integer("0")), test_sexp::simple_pattern("y")}
            ),
            test_sexp::string(R"("y-axis")")
        ),
        test_sexp::match_arm(
            test_sexp::tuple_pattern(
                {test_sexp::simple_pattern("x"), test_sexp::literal_pattern(test_sexp::integer("0"))}
            ),
            test_sexp::string(R"("x-axis")")
        ),
        test_sexp::match_arm(
            test_sexp::tuple_pattern({test_sexp::simple_pattern("x"), test_sexp::simple_pattern("y")}),
            test_sexp::string(R"("elsewhere")")
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
inline auto const k_wildcard_in_tuple_expected = test_sexp::match_expr(
    test_sexp::var_name("pair"),
    {
        test_sexp::match_arm(
            test_sexp::tuple_pattern({test_sexp::wildcard_pattern(), test_sexp::wildcard_pattern()}),
            test_sexp::string(R"("any point")")
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
inline auto const k_string_literal_guard_expected = test_sexp::match_expr(
    test_sexp::var_name("x"),
    {
        test_sexp::match_arm_with_guard(
            test_sexp::literal_pattern(test_sexp::string(R"("admin")")),
            test_sexp::var_name("is_verified"),
            test_sexp::string(R"("ok")")
        ),
        test_sexp::match_arm(
            test_sexp::literal_pattern(test_sexp::string(R"("admin")")),
            test_sexp::string(R"("unverified")")
        ),
        test_sexp::match_arm(test_sexp::wildcard_pattern(), test_sexp::string(R"("unknown")")),
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
inline auto const k_multiple_wildcards_expected = test_sexp::match_expr(
    test_sexp::var_name("pair"),
    {
        test_sexp::match_arm(
            test_sexp::tuple_pattern(
                {test_sexp::literal_pattern(test_sexp::integer("0")), test_sexp::wildcard_pattern()}
            ),
            test_sexp::string(R"("first zero")")
        ),
        test_sexp::match_arm(
            test_sexp::tuple_pattern(
                {test_sexp::wildcard_pattern(), test_sexp::literal_pattern(test_sexp::integer("0"))}
            ),
            test_sexp::string(R"("second zero")")
        ),
        test_sexp::match_arm(
            test_sexp::tuple_pattern({test_sexp::wildcard_pattern(), test_sexp::wildcard_pattern()}),
            test_sexp::string(R"("neither")")
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
inline auto const k_trailing_comma_expected = test_sexp::match_expr(
    test_sexp::var_name("x"),
    {test_sexp::match_arm(test_sexp::simple_pattern("zero"), test_sexp::string(R"("zero")")),
     test_sexp::match_arm(test_sexp::simple_pattern("one"), test_sexp::string(R"("one")"))}
);

// Single arm
constexpr auto k_single_arm_should_succeed = true;
constexpr auto k_single_arm_input = R"(
  match x {
    any => 42
  }
)";
inline auto const k_single_arm_expected = test_sexp::match_expr(
    test_sexp::var_name("x"),
    {test_sexp::match_arm(test_sexp::simple_pattern("any"), test_sexp::integer(42))}
);

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
inline auto const k_nested_match_expected = test_sexp::match_expr(
    test_sexp::var_name("x"),
    {test_sexp::match_arm(
         test_sexp::simple_pattern("zero"),
         test_sexp::match_expr(
             test_sexp::var_name("y"),
             {test_sexp::match_arm(test_sexp::simple_pattern("one"), test_sexp::string(R"("a")")),
              test_sexp::match_arm(test_sexp::simple_pattern("other"), test_sexp::string(R"("b")"))}
         )
     ),
     test_sexp::match_arm(test_sexp::simple_pattern("other"), test_sexp::string(R"("c")"))}
);

// Struct pattern with identifier bindings
constexpr auto k_struct_with_bindings_should_succeed = true;
constexpr auto k_struct_with_bindings_input = R"(
  match point {
    Point { x: px, y: py } => add(px, py)
  }
)";
inline auto const k_struct_with_bindings_expected = test_sexp::match_expr(
    test_sexp::var_name("point"),
    {
        test_sexp::match_arm(
            test_sexp::struct_pattern(
                test_sexp::type_name("Point"),
                {
                    test_sexp::field_pattern("x", test_sexp::simple_pattern("px")),
                    test_sexp::field_pattern("y", test_sexp::simple_pattern("py")),
                }
            ),
            test_sexp::function_call(test_sexp::var_name("add"), {test_sexp::var_name("px"), test_sexp::var_name("py")})
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
inline auto const k_struct_with_wildcard_expected = test_sexp::match_expr(
    test_sexp::var_name("point"),
    {
        test_sexp::match_arm(
            test_sexp::struct_pattern(
                test_sexp::type_name("Point"),
                {
                    test_sexp::field_pattern("x", test_sexp::wildcard_pattern()),
                    test_sexp::field_pattern("y", test_sexp::literal_pattern(test_sexp::integer("0"))),
                }
            ),
            test_sexp::string(R"("on x-axis")")
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
inline auto const k_nested_struct_expected = test_sexp::match_expr(
    test_sexp::var_name("line"),
    {
        test_sexp::match_arm(
            test_sexp::struct_pattern(
                test_sexp::type_name("Line"),
                {
                    test_sexp::field_pattern(
                        "start",
                        test_sexp::struct_pattern(
                            test_sexp::type_name("Point"),
                            {
                                test_sexp::field_pattern("x", test_sexp::literal_pattern(test_sexp::integer("0"))),
                                test_sexp::field_pattern("y", test_sexp::literal_pattern(test_sexp::integer("0"))),
                            }
                        )
                    ),
                    test_sexp::field_pattern("end", test_sexp::simple_pattern("p")),
                }
            ),
            test_sexp::function_call(test_sexp::var_name("process"), {test_sexp::var_name("p")})
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
inline auto const k_struct_shorthand_expected = test_sexp::match_expr(
    test_sexp::var_name("point"),
    {
        test_sexp::match_arm(
            test_sexp::struct_pattern(
                test_sexp::type_name("Point"),
                {
                    test_sexp::field_pattern("x", test_sexp::simple_pattern("x")),
                    test_sexp::field_pattern("y", test_sexp::simple_pattern("y")),
                }
            ),
            test_sexp::function_call(test_sexp::var_name("add"), {test_sexp::var_name("x"), test_sexp::var_name("y")})
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
inline auto const k_struct_mixed_expected = test_sexp::match_expr(
    test_sexp::var_name("point"),
    {
        test_sexp::match_arm(
            test_sexp::struct_pattern(
                test_sexp::type_name("Point"),
                {
                    test_sexp::field_pattern("x", test_sexp::simple_pattern("x")),
                    test_sexp::field_pattern("y", test_sexp::literal_pattern(test_sexp::integer("0"))),
                }
            ),
            test_sexp::string(R"("on x-axis")")
        ),
        test_sexp::match_arm(
            test_sexp::struct_pattern(
                test_sexp::type_name("Point"),
                {
                    test_sexp::field_pattern("x", test_sexp::literal_pattern(test_sexp::integer("0"))),
                    test_sexp::field_pattern("y", test_sexp::simple_pattern("y")),
                }
            ),
            test_sexp::string(R"("on y-axis")")
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
inline auto const k_complex_guard_expected = test_sexp::match_expr(
    test_sexp::var_name("n"),
    {test_sexp::match_arm_with_guard(
         test_sexp::simple_pattern("x"),
         test_sexp::binary_expr(
             "&&",
             test_sexp::binary_expr(">", test_sexp::var_name("x"), test_sexp::var_name("zero")),
             test_sexp::binary_expr("<", test_sexp::var_name("x"), test_sexp::var_name("ten"))
         ),
         test_sexp::string(R"("single")")
     ),
     test_sexp::match_arm(test_sexp::simple_pattern("other"), test_sexp::string(R"("other")"))}
);

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

TEST_CASE("Parse Match_Expr") {
  std::vector<Expr_Params> const params_list = {
      // Basic patterns
      {.name = "simple patterns",
       .input = k_simple_patterns_input,
       .expected = k_simple_patterns_expected,
       .should_succeed = k_simple_patterns_should_succeed},
      {.name = "with guard",
       .input = k_with_guard_input,
       .expected = k_with_guard_expected,
       .should_succeed = k_with_guard_should_succeed},
      {.name = "tuple pattern",
       .input = k_tuple_pattern_input,
       .expected = k_tuple_pattern_expected,
       .should_succeed = k_tuple_pattern_should_succeed},
      {.name = "struct pattern",
       .input = k_struct_pattern_input,
       .expected = k_struct_pattern_expected,
       .should_succeed = k_struct_pattern_should_succeed},
      {.name = "struct with bindings",
       .input = k_struct_with_bindings_input,
       .expected = k_struct_with_bindings_expected,
       .should_succeed = k_struct_with_bindings_should_succeed},
      {.name = "struct with wildcard",
       .input = k_struct_with_wildcard_input,
       .expected = k_struct_with_wildcard_expected,
       .should_succeed = k_struct_with_wildcard_should_succeed},
      {.name = "nested struct",
       .input = k_nested_struct_input,
       .expected = k_nested_struct_expected,
       .should_succeed = k_nested_struct_should_succeed},
      {.name = "struct shorthand",
       .input = k_struct_shorthand_input,
       .expected = k_struct_shorthand_expected,
       .should_succeed = k_struct_shorthand_should_succeed},
      {.name = "struct mixed",
       .input = k_struct_mixed_input,
       .expected = k_struct_mixed_expected,
       .should_succeed = k_struct_mixed_should_succeed},
      {.name = "trailing comma",
       .input = k_trailing_comma_input,
       .expected = k_trailing_comma_expected,
       .should_succeed = k_trailing_comma_should_succeed},
      {.name = "single arm",
       .input = k_single_arm_input,
       .expected = k_single_arm_expected,
       .should_succeed = k_single_arm_should_succeed},
      {.name = "nested match",
       .input = k_nested_match_input,
       .expected = k_nested_match_expected,
       .should_succeed = k_nested_match_should_succeed},
      {.name = "complex guard",
       .input = k_complex_guard_input,
       .expected = k_complex_guard_expected,
       .should_succeed = k_complex_guard_should_succeed},

      // Literal patterns
      {.name = "literal integers",
       .input = k_literal_int_input,
       .expected = k_literal_int_expected,
       .should_succeed = k_literal_int_should_succeed},
      {.name = "literal strings",
       .input = k_literal_string_input,
       .expected = k_literal_string_expected,
       .should_succeed = k_literal_string_should_succeed},
      {.name = "mixed patterns",
       .input = k_mixed_patterns_input,
       .expected = k_mixed_patterns_expected,
       .should_succeed = k_mixed_patterns_should_succeed},
      {.name = "literal in tuple",
       .input = k_literal_in_tuple_input,
       .expected = k_literal_in_tuple_expected,
       .should_succeed = k_literal_in_tuple_should_succeed},
      {.name = "string literal with guard",
       .input = k_string_literal_guard_input,
       .expected = k_string_literal_guard_expected,
       .should_succeed = k_string_literal_guard_should_succeed},

      // Wildcard patterns
      {.name = "wildcard",
       .input = k_wildcard_input,
       .expected = k_wildcard_expected,
       .should_succeed = k_wildcard_should_succeed},
      {.name = "wildcard with guard",
       .input = k_wildcard_guard_input,
       .expected = k_wildcard_guard_expected,
       .should_succeed = k_wildcard_guard_should_succeed},
      {.name = "wildcard in tuple",
       .input = k_wildcard_in_tuple_input,
       .expected = k_wildcard_in_tuple_expected,
       .should_succeed = k_wildcard_in_tuple_should_succeed},
      {.name = "multiple wildcards",
       .input = k_multiple_wildcards_input,
       .expected = k_multiple_wildcards_expected,
       .should_succeed = k_multiple_wildcards_should_succeed},

      // Invalid cases
      {.name = "missing arrow",
       .input = k_missing_arrow_input,
       .expected = k_missing_arrow_expected,
       .should_succeed = k_missing_arrow_should_succeed},
      {.name = "missing result",
       .input = k_missing_result_input,
       .expected = k_missing_result_expected,
       .should_succeed = k_missing_result_should_succeed},
      {.name = "missing brace",
       .input = k_missing_brace_input,
       .expected = k_missing_brace_expected,
       .should_succeed = k_missing_brace_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
