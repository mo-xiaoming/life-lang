#include "internal_rules.hpp"
#include "utils.hpp"

using namespace std::string_literals;

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
      std::string const expected = R"((cast (var ((var_segment "x"))) (path ((type_segment "I64")))))";
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast integer literal to F32") {
    auto const input = "42 as F32"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      std::string const expected = R"((cast (integer "42") (path ((type_segment "F32"))))";
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast with qualified type") {
    auto const input = "value as Std.Option"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      std::string const expected =
          R"((cast (var ((var_segment "value"))) (path ((type_segment "Std") (type_segment "Option"))))";
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast to generic type") {
    auto const input = "data as Vec<I32>"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      std::string const expected =
          R"((cast (var ((var_segment "data"))) (path ((type_segment "Vec" ((path ((type_segment "I32"))))))))";
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast to pointer type (U64)") {
    auto const input = "ptr as U64"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      std::string const expected = R"((cast (var ((var_segment "ptr"))) (path ((type_segment "U64"))))";
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  // ========================================================================
  // Precedence Tests
  // ========================================================================

  TEST_CASE("Cast has higher precedence than addition") {
    // x + y as I64 => x + (y as I64)
    auto const input = "x + y as I64"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      std::string const expected =
          R"((binary + (var ((var_segment "x"))) (cast (var ((var_segment "y"))) (path ((type_segment "I64")))))";
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast has higher precedence than multiplication") {
    // a * b as F64 => a * (b as F64)
    auto const input = "a * b as F64"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      std::string const expected =
          R"((binary * (var ((var_segment "a"))) (cast (var ((var_segment "b"))) (path ((type_segment "F64")))))";
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
      std::string const expected =
          R"((cast (field_access (var ((var_segment "obj"))) "field") (path ((type_segment "I32"))))";
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
      std::string const expected = R"((cast (call (var ((var_segment "func"))) ()) (path ((type_segment "Bool"))))";
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
      std::string const expected =
          R"((cast (index (var ((var_segment "arr"))) (integer "0")) (path ((type_segment "U8"))))";
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
      std::string const expected =
          R"((cast (cast (var ((var_segment "x"))) (path ((type_segment "I32")))) (path ((type_segment "I64"))))";
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
      std::string const expected =
          R"((cast (binary + (var ((var_segment "x"))) (var ((var_segment "y")))) (path ((type_segment "I64"))))";
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  // ========================================================================
  // Complex Expressions with Casts
  // ========================================================================

  TEST_CASE("Cast in binary expression context") {
    // result = x as I64 + y as I64
    auto const input = "x as I64 + y as I64"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      std::string const expected =
          R"((binary + (cast (var ((var_segment "x"))) (path ((type_segment "I64")))) (cast (var ((var_segment "y"))) (path ((type_segment "I64")))))";
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
      std::string const expected =
          R"((cast (field_access (call (var ((var_segment "obj") (var_segment "method"))) ()) "value") (path ((type_segment "String"))))";
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast to tuple type") {
    auto const input = "value as (I32, String)"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      std::string const expected =
          R"foo(cast (var ((var_segment "value"))) (tuple_type ((path ((type_segment "I32"))) (path ((type_segment "String"))))))foo";
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast to array type") {
    auto const input = "list as [I32; 10]"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      std::string const expected =
          R"((cast (var ((var_segment "list"))) (array_type (path ((type_segment "I32"))) "10"))";
      CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
    }
  }

  TEST_CASE("Cast to function type") {
    auto const input = "callback as fn(I32): Bool"s;
    auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

    REQUIRE(result.has_value());
    if (result.has_value()) {
      auto const& value = *result;
      std::string const expected =
          R"((cast (var ((var_segment "callback"))) (func_type ((path ((type_segment "I32")))) (path ((type_segment "Bool")))))";
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
    CHECK(life_lang::ast::to_sexp_string(value, 0) == R"(var ((var_segment "as_value")))");
  }
}

TEST_CASE("Cast in comparison context") {
  // x as I64 == y as I64
  auto const input = "x as I64 == y as I64"s;
  auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

  REQUIRE(result.has_value());
  if (result.has_value()) {
    auto const& value = *result;
    std::string const expected =
        R"((binary == (cast (var ((var_segment "x"))) (path ((type_segment "I64")))) (cast (var ((var_segment "y"))) (path ((type_segment "I64")))))";
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
    std::string const expected =
        R"((call (var ((var_segment "print"))) ((cast (var ((var_segment "value"))) (path ((type_segment "String"))))))";
    CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
  }
}

TEST_CASE("Unary negation with cast") {
  // -x as I64 => -(x as I64) (cast has higher precedence than unary)
  auto const input = "-x as I64"s;
  auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

  REQUIRE(result.has_value());
  if (result.has_value()) {
    auto const& value = *result;
    std::string const expected = R"((cast (unary - (var ((var_segment "x")))) (path ((type_segment "I64"))))";
    CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
  }
}

TEST_CASE("Cast with range expression") {
  // 0 as U32..100 as U32
  auto const input = "0 as U32..100 as U32"s;
  auto const result = Parse_Helper<life_lang::ast::Expr>::parse(input);

  REQUIRE(result.has_value());
  if (result.has_value()) {
    auto const& value = *result;
    std::string const expected =
        R"((range (cast (integer "0") (path ((type_segment "U32")))) (cast (integer "100") (path ((type_segment "U32"))))))";
    CHECK(life_lang::ast::to_sexp_string(value, 0) == expected);
  }
}
