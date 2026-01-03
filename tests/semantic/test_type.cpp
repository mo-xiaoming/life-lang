#include "semantic/type.hpp"

#include <doctest/doctest.h>

using namespace life_lang::semantic;

TEST_SUITE("Type System") {
  TEST_CASE("Primitive types") {
    SUBCASE("Type creation") {
      auto i8 = make_primitive_type(Primitive_Type::Kind::I8);
      auto i16 = make_primitive_type(Primitive_Type::Kind::I16);
      auto i32 = make_primitive_type(Primitive_Type::Kind::I32);
      auto i64 = make_primitive_type(Primitive_Type::Kind::I64);
      auto u8 = make_primitive_type(Primitive_Type::Kind::U8);
      auto u16 = make_primitive_type(Primitive_Type::Kind::U16);
      auto u32 = make_primitive_type(Primitive_Type::Kind::U32);
      auto u64 = make_primitive_type(Primitive_Type::Kind::U64);
      auto f32 = make_primitive_type(Primitive_Type::Kind::F32);
      auto f64 = make_primitive_type(Primitive_Type::Kind::F64);
      auto bool_t = make_primitive_type(Primitive_Type::Kind::Bool);
      auto char_t = make_primitive_type(Primitive_Type::Kind::Char);
      auto string_t = make_primitive_type(Primitive_Type::Kind::String);

      CHECK(i8.is_primitive());
      CHECK(i16.is_primitive());
      CHECK(i32.is_primitive());
      CHECK(i64.is_primitive());
      CHECK(u8.is_primitive());
      CHECK(u16.is_primitive());
      CHECK(u32.is_primitive());
      CHECK(u64.is_primitive());
      CHECK(f32.is_primitive());
      CHECK(f64.is_primitive());
      CHECK(bool_t.is_primitive());
      CHECK(char_t.is_primitive());
      CHECK(string_t.is_primitive());
    }

    SUBCASE("Type equality") {
      auto i32_a = make_primitive_type(Primitive_Type::Kind::I32);
      auto i32_b = make_primitive_type(Primitive_Type::Kind::I32);
      auto i64 = make_primitive_type(Primitive_Type::Kind::I64);

      CHECK(i32_a == i32_b);
      CHECK_FALSE(i32_a == i64);
    }

    SUBCASE("Type predicates") {
      auto i32 = make_primitive_type(Primitive_Type::Kind::I32);
      auto u32 = make_primitive_type(Primitive_Type::Kind::U32);
      auto f64 = make_primitive_type(Primitive_Type::Kind::F64);
      auto bool_t = make_primitive_type(Primitive_Type::Kind::Bool);

      // Signed integer
      CHECK(i32.is_numeric());
      CHECK(i32.is_integral());
      CHECK(i32.is_signed_int());
      CHECK_FALSE(i32.is_unsigned_int());
      CHECK_FALSE(i32.is_floating());

      // Unsigned integer
      CHECK(u32.is_numeric());
      CHECK(u32.is_integral());
      CHECK_FALSE(u32.is_signed_int());
      CHECK(u32.is_unsigned_int());
      CHECK_FALSE(u32.is_floating());

      // Float
      CHECK(f64.is_numeric());
      CHECK_FALSE(f64.is_integral());
      CHECK_FALSE(f64.is_signed_int());
      CHECK_FALSE(f64.is_unsigned_int());
      CHECK(f64.is_floating());

      // Bool (not numeric)
      CHECK_FALSE(bool_t.is_numeric());
      CHECK_FALSE(bool_t.is_integral());
      CHECK_FALSE(bool_t.is_signed_int());
      CHECK_FALSE(bool_t.is_unsigned_int());
      CHECK_FALSE(bool_t.is_floating());
    }

    SUBCASE("Type to_string") {
      auto i32 = make_primitive_type(Primitive_Type::Kind::I32);
      auto string_t = make_primitive_type(Primitive_Type::Kind::String);

      CHECK(i32.to_string() == "I32");
      CHECK(string_t.to_string() == "String");
    }
  }

  TEST_CASE("Unit type") {
    auto unit = make_unit_type();

    CHECK(unit.is_unit());
    CHECK_FALSE(unit.is_primitive());
    CHECK(unit.to_string() == "()");

    auto unit2 = make_unit_type();
    CHECK(unit == unit2);
  }

  TEST_CASE("Error type") {
    auto err = make_error_type();

    CHECK(err.is_error());
    CHECK_FALSE(err.is_primitive());
    CHECK(err.to_string() == "<error>");
  }

  TEST_CASE("Struct types") {
    SUBCASE("Simple struct") {
      auto point = make_struct_type(
          "Point",
          {},
          {{"x", make_primitive_type(Primitive_Type::Kind::I32)}, {"y", make_primitive_type(Primitive_Type::Kind::I32)}}
      );

      CHECK(point.to_string() == "Point");
      CHECK_FALSE(point.is_primitive());
    }

    SUBCASE("Generic struct") {
      auto vec_t = make_struct_type("Vec", {"T"}, {});
      CHECK(vec_t.to_string() == "Vec<T>");
    }

    SUBCASE("Struct equality") {
      auto point_a = make_struct_type("Point");
      auto point_b = make_struct_type("Point");
      auto circle = make_struct_type("Circle");

      CHECK(point_a == point_b);
      CHECK_FALSE(point_a == circle);
    }
  }

  TEST_CASE("Enum types") {
    SUBCASE("Simple enum") {
      auto color = make_enum_type("Color", {}, {"Red", "Green", "Blue"});
      CHECK(color.to_string() == "Color");
    }

    SUBCASE("Generic enum") {
      auto option = make_enum_type("Option", {"T"}, {"Some", "None"});
      CHECK(option.to_string() == "Option<T>");
    }

    SUBCASE("Enum equality") {
      auto option_a = make_enum_type("Option", {"T"});
      auto option_b = make_enum_type("Option", {"T"});
      auto result = make_enum_type("Result", {"T", "E"});

      CHECK(option_a == option_b);
      CHECK_FALSE(option_a == result);
    }
  }

  TEST_CASE("Function types") {
    SUBCASE("Simple function") {
      auto i32 = make_primitive_type(Primitive_Type::Kind::I32);
      auto fn_type = make_function_type({i32, i32}, i32);

      CHECK(fn_type.to_string() == "fn(I32, I32): I32");
    }

    SUBCASE("Function equality") {
      auto i32 = make_primitive_type(Primitive_Type::Kind::I32);
      auto i64 = make_primitive_type(Primitive_Type::Kind::I64);

      auto fn1 = make_function_type({i32, i32}, i32);
      auto fn2 = make_function_type({i32, i32}, i32);
      auto fn3 = make_function_type({i32, i32}, i64);

      CHECK(fn1 == fn2);
      CHECK_FALSE(fn1 == fn3);
    }
  }

  TEST_CASE("Array types") {
    SUBCASE("Sized array") {
      auto i32 = make_primitive_type(Primitive_Type::Kind::I32);
      auto arr = make_array_type(i32, 10);

      CHECK(arr.to_string() == "[I32; 10]");
    }

    SUBCASE("Unsized array") {
      auto i32 = make_primitive_type(Primitive_Type::Kind::I32);
      auto arr = make_array_type(i32);

      CHECK(arr.to_string() == "[I32]");
    }

    SUBCASE("Array equality") {
      auto i32 = make_primitive_type(Primitive_Type::Kind::I32);
      auto arr1 = make_array_type(i32, 10);
      auto arr2 = make_array_type(i32, 10);
      auto arr3 = make_array_type(i32, 20);

      CHECK(arr1 == arr2);
      CHECK_FALSE(arr1 == arr3);
    }
  }

  TEST_CASE("Tuple types") {
    SUBCASE("Simple tuple") {
      auto i32 = make_primitive_type(Primitive_Type::Kind::I32);
      auto string_t = make_primitive_type(Primitive_Type::Kind::String);
      auto tuple = make_tuple_type({i32, string_t});

      CHECK(tuple.to_string() == "(I32, String)");
    }

    SUBCASE("Tuple equality") {
      auto i32 = make_primitive_type(Primitive_Type::Kind::I32);
      auto i64 = make_primitive_type(Primitive_Type::Kind::I64);

      auto tuple1 = make_tuple_type({i32, i32});
      auto tuple2 = make_tuple_type({i32, i32});
      auto tuple3 = make_tuple_type({i32, i64});

      CHECK(tuple1 == tuple2);
      CHECK_FALSE(tuple1 == tuple3);
    }
  }

  TEST_CASE("Generic types") {
    auto t = make_generic_type("T");
    auto u = make_generic_type("U");

    CHECK(t.to_string() == "T");
    CHECK(u.to_string() == "U");

    auto t2 = make_generic_type("T");
    CHECK(t == t2);
    CHECK_FALSE(t == u);
  }

  TEST_CASE("Builtin types registry") {
    SUBCASE("Lookup by name") {
      auto i32 = Builtin_Types::lookup("I32");
      REQUIRE(i32.has_value());
      if (i32.has_value()) {
        CHECK(i32.value().to_string() == "I32");
      }

      auto string_t = Builtin_Types::lookup("String");
      REQUIRE(string_t.has_value());
      if (string_t.has_value()) {
        CHECK(string_t.value().to_string() == "String");
      }

      auto unknown = Builtin_Types::lookup("Unknown");
      CHECK_FALSE(unknown.has_value());
    }

    SUBCASE("All builtins present") {
      // Signed integers
      CHECK(Builtin_Types::lookup("I8").has_value());
      CHECK(Builtin_Types::lookup("I16").has_value());
      CHECK(Builtin_Types::lookup("I32").has_value());
      CHECK(Builtin_Types::lookup("I64").has_value());
      // Unsigned integers
      CHECK(Builtin_Types::lookup("U8").has_value());
      CHECK(Builtin_Types::lookup("U16").has_value());
      CHECK(Builtin_Types::lookup("U32").has_value());
      CHECK(Builtin_Types::lookup("U64").has_value());
      // Floats
      CHECK(Builtin_Types::lookup("F32").has_value());
      CHECK(Builtin_Types::lookup("F64").has_value());
      // Other types
      CHECK(Builtin_Types::lookup("Bool").has_value());
      CHECK(Builtin_Types::lookup("Char").has_value());
      CHECK(Builtin_Types::lookup("String").has_value());
    }

    SUBCASE("Direct static access") {
      CHECK(Builtin_Types::k_i8.to_string() == "I8");
      CHECK(Builtin_Types::k_i16.to_string() == "I16");
      CHECK(Builtin_Types::k_i32.to_string() == "I32");
      CHECK(Builtin_Types::k_i64.to_string() == "I64");
      CHECK(Builtin_Types::k_u8.to_string() == "U8");
      CHECK(Builtin_Types::k_u16.to_string() == "U16");
      CHECK(Builtin_Types::k_u32.to_string() == "U32");
      CHECK(Builtin_Types::k_u64.to_string() == "U64");
      CHECK(Builtin_Types::k_f32.to_string() == "F32");
      CHECK(Builtin_Types::k_f64.to_string() == "F64");
      CHECK(Builtin_Types::k_bool_type.to_string() == "Bool");
      CHECK(Builtin_Types::k_char_type.to_string() == "Char");
      CHECK(Builtin_Types::k_string.to_string() == "String");
      CHECK(Builtin_Types::k_unit.to_string() == "()");
    }
  }
}
