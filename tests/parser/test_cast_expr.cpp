#include "internal_rules.hpp"
#include "utils.hpp"

using namespace std::string_literals;
using namespace test_sexp;

TEST_SUITE("Parser - Cast Expression") {
  // ========================================================================
  // Basic Cast Expressions
  // ========================================================================

  TEST_CASE("Simple cast to I64") {
    auto const input = "x as I64"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = cast_expr(var_name("x"), type_name("I64"));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast integer literal to F32") {
    auto const input = "42 as F32"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = cast_expr(integer("42"), type_name("F32"));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast with qualified type") {
    auto const input = "value as Std.Option"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = cast_expr(var_name("value"), type_name("Std", "Option"));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast to generic type") {
    auto const input = "data as Vec<I32>"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected =
          cast_expr(var_name("data"), R"((path ((type_segment "Vec" ((path ((type_segment "I32"))))))))");
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast to pointer type (U64)") {
    auto const input = "ptr as U64"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = cast_expr(var_name("ptr"), type_name("U64"));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  // ========================================================================
  // Precedence Tests
  // ========================================================================

  TEST_CASE("Cast has lower precedence than addition") {
    // x + y as I64 => x + (y as I64) because 'as' binds tighter than '+'
    auto const input = "x + y as I64"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = binary_expr("+", var_name("x"), cast_expr(var_name("y"), type_name("I64")));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast has lower precedence than multiplication") {
    // a * b as F64 => a * (b as F64) because 'as' binds tighter than '*'
    auto const input = "a * b as F64"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = binary_expr("*", var_name("a"), cast_expr(var_name("b"), type_name("F64")));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast has lower precedence than field access") {
    // obj.field as I32 => (obj.field) as I32
    auto const input = "obj.field as I32"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = cast_expr(field_access(var_name("obj"), "field"), type_name("I32"));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast has lower precedence than function call") {
    // func() as Bool => (func()) as Bool
    auto const input = "func() as Bool"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = cast_expr(function_call(var_name("func"), {}), type_name("Bool"));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast has lower precedence than indexing") {
    // arr[0] as U8 => (arr[0]) as U8
    auto const input = "arr[0] as U8"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = cast_expr(R"((index (var ((var_segment "arr"))) (integer "0")))", type_name("U8"));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Multiple casts are left-associative") {
    // x as I32 as I64 => (x as I32) as I64
    auto const input = "x as I32 as I64"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = cast_expr(cast_expr(var_name("x"), type_name("I32")), type_name("I64"));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Parentheses override cast precedence") {
    // (x + y) as I64 => (x + y) as I64 (explicit grouping)
    auto const input = "(x + y) as I64"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = cast_expr(binary_expr("+", var_name("x"), var_name("y")), type_name("I64"));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  // ========================================================================
  // Complex Expressions with Casts
  // ========================================================================

  TEST_CASE("Cast in binary expression context") {
    // x as I64 + y as I64 => (x as I64) + (y as I64)
    auto const input = "x as I64 + y as I64"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected =
          binary_expr("+", cast_expr(var_name("x"), type_name("I64")), cast_expr(var_name("y"), type_name("I64")));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast with method chain") {
    // obj.method().value as String
    auto const input = "obj.method().value as String"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected =
          cast_expr(field_access(function_call(var_name_path({"obj", "method"}), {}), "value"), type_name("String"));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast to tuple type") {
    auto const input = "value as (I32, String)"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = cast_expr(
          var_name("value"),
          R"((tuple_type ((path ((type_segment "I32"))) (path ((type_segment "String"))))))"
      );
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast to array type") {
    auto const input = "list as [I32; 10]"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = cast_expr(var_name("list"), array_type(type_name("I32"), "10"));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast to function type") {
    auto const input = "callback as fn(I32): Bool"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      auto const expected = cast_expr(var_name("callback"), func_type({type_name("I32")}, type_name("Bool")));
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  // ========================================================================
  // Edge Cases and Error Handling
  // ========================================================================

  TEST_CASE("Cast requires type name after 'as'") {
    auto const input = "x as"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    // Should fail - no type after 'as'
    CHECK_FALSE(result.has_value());
  }
}

TEST_CASE("'as' is only keyword when followed by type") {
  // Variable named 'as_value' should parse correctly
  auto const input = "as_value"s;
  auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

  REQUIRE(result.has_value());
  if (result.has_value()) {
    auto const& value = *result;
    auto const expected = var_name("as_value");
    CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
  }
}

TEST_CASE("Cast in comparison context") {
  // x as I64 == y as I64 => (x as I64) == (y as I64)
  auto const input = "x as I64 == y as I64"s;
  auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

  REQUIRE(result.has_value());
  if (result.has_value()) {
    auto const& value = *result;
    auto const expected =
        binary_expr("==", cast_expr(var_name("x"), type_name("I64")), cast_expr(var_name("y"), type_name("I64")));
    CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
  }
}

TEST_CASE("Cast in function call argument") {
  // print(value as String)
  auto const input = "print(value as String)"s;
  auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

  REQUIRE(result.has_value());
  if (result.has_value()) {
    auto const& value = *result;
    auto const expected = function_call(var_name("print"), {cast_expr(var_name("value"), type_name("String"))});
    CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
  }
}

TEST_CASE("Unary negation with cast") {
  // -x as I64 => (-x) as I64 (cast has lower precedence than unary)
  auto const input = "-x as I64"s;
  auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

  REQUIRE(result.has_value());
  if (result.has_value()) {
    auto const& value = *result;
    auto const expected = cast_expr(unary_expr("-", var_name("x")), type_name("I64"));
    CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
  }
}

TEST_CASE("Cast with range expression") {
  // 0 as U32..100 as U32 => (0 as U32)..(100 as U32)
  auto const input = "0 as U32..100 as U32"s;
  auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

  REQUIRE(result.has_value());
  if (result.has_value()) {
    auto const& value = *result;
    auto const expected =
        range_expr(cast_expr(integer("0"), type_name("U32")), cast_expr(integer("100"), type_name("U32")), false);
    CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
  }
}
