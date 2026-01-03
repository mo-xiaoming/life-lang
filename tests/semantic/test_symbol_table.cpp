#include "semantic/symbol_table.hpp"

#include <doctest/doctest.h>

using namespace life_lang::semantic;

TEST_SUITE("Symbol Table") {
  TEST_CASE("Symbol creation") {
    auto i32 = make_primitive_type(Primitive_Type::Kind::I32);
    auto loc = make_symbol_location("test.life", life_lang::Source_Position{.line = 1, .column = 5});

    auto sym = make_symbol("x", Symbol_Kind::Variable, i32, Visibility::Module_Internal, loc);

    CHECK(sym.name == "x");
    CHECK(sym.kind == Symbol_Kind::Variable);
    CHECK(sym.type == i32);
    CHECK(sym.visibility == Visibility::Module_Internal);
    CHECK(sym.location.filename == "test.life");
    CHECK(sym.location.position.line == 1);
    CHECK(sym.location.position.column == 5);
  }

  TEST_CASE("Scope - basic operations") {
    Scope scope(Scope_Kind::Module);
    auto i32 = make_primitive_type(Primitive_Type::Kind::I32);

    SUBCASE("Declare and lookup") {
      auto error = scope.declare("x", make_symbol("x", Symbol_Kind::Variable, i32));
      CHECK(!error);

      auto sym = scope.lookup("x");
      REQUIRE(sym.has_value());
      if (sym.has_value()) {
        CHECK(sym.value().name == "x");
        CHECK(sym.value().kind == Symbol_Kind::Variable);
      }
    }

    SUBCASE("Duplicate declaration fails") {
      auto error1 = scope.declare("x", make_symbol("x", Symbol_Kind::Variable, i32));
      CHECK(!error1);

      auto error2 = scope.declare("x", make_symbol("x", Symbol_Kind::Variable, i32));
      REQUIRE(error2.has_value());
      if (error2.has_value()) {
        CHECK(error2.value().find("already declared") != std::string::npos);
      }
    }

    SUBCASE("Lookup non-existent symbol") {
      auto sym = scope.lookup("unknown");
      CHECK_FALSE(sym.has_value());
    }

    SUBCASE("Contains check") {
      auto error = scope.declare("x", make_symbol("x", Symbol_Kind::Variable, i32));
      CHECK(!error);

      CHECK(scope.contains("x"));
      CHECK_FALSE(scope.contains("y"));
    }

    SUBCASE("Local vs parent lookup") {
      auto sym = scope.lookup_local("x");
      CHECK_FALSE(sym.has_value());

      auto error = scope.declare("x", make_symbol("x", Symbol_Kind::Variable, i32));
      CHECK(!error);
      sym = scope.lookup_local("x");
      CHECK(sym.has_value());
    }
  }

  TEST_CASE("Scope - parent chain") {
    auto i32 = make_primitive_type(Primitive_Type::Kind::I32);
    auto string_t = make_primitive_type(Primitive_Type::Kind::String);

    Scope parent_scope(Scope_Kind::Module);
    [[maybe_unused]] auto err1 = parent_scope.declare("x", make_symbol("x", Symbol_Kind::Variable, i32));

    Scope child_scope(Scope_Kind::Block, &parent_scope);
    [[maybe_unused]] auto err2 = child_scope.declare("y", make_symbol("y", Symbol_Kind::Variable, string_t));

    SUBCASE("Child can see parent symbols") {
      auto sym = child_scope.lookup("x");
      REQUIRE(sym.has_value());
      if (sym.has_value()) {
        CHECK(sym.value().name == "x");
      }
    }

    SUBCASE("Child can see own symbols") {
      auto sym = child_scope.lookup("y");
      REQUIRE(sym.has_value());
      if (sym.has_value()) {
        CHECK(sym.value().name == "y");
      }
    }

    SUBCASE("Parent cannot see child symbols") {
      auto sym = parent_scope.lookup("y");
      CHECK_FALSE(sym.has_value());
    }

    SUBCASE("lookup_local only searches current scope") {
      auto sym = child_scope.lookup_local("x");
      CHECK_FALSE(sym.has_value());  // x is in parent, not child

      sym = child_scope.lookup_local("y");
      CHECK(sym.has_value());  // y is in child
    }

    SUBCASE("Shadowing") {
      // Declare 'x' in child scope (shadows parent's 'x')
      [[maybe_unused]] auto err = child_scope.declare("x", make_symbol("x", Symbol_Kind::Variable, string_t));

      auto sym = child_scope.lookup("x");
      REQUIRE(sym.has_value());
      if (sym.has_value()) {
        CHECK(sym.value().type == string_t);  // Child's version (String, not I32)
      }
    }
  }

  TEST_CASE("Symbol_Table - module scopes") {
    Symbol_Table symtab;

    SUBCASE("Create module scope") {
      auto* scope = symtab.create_module_scope("Geometry");
      CHECK(scope != nullptr);
      CHECK(scope->kind() == Scope_Kind::Module);
    }

    SUBCASE("Get existing module scope") {
      [[maybe_unused]] auto* created = symtab.create_module_scope("Geometry");
      auto* scope = symtab.get_module_scope("Geometry");
      CHECK(scope != nullptr);
    }

    SUBCASE("Get non-existent module scope") {
      auto* scope = symtab.get_module_scope("Unknown");
      CHECK(scope == nullptr);
    }

    SUBCASE("Multiple modules") {
      auto* geo_scope = symtab.create_module_scope("Geometry");
      auto* math_scope = symtab.create_module_scope("Math");

      CHECK(geo_scope != math_scope);
      CHECK(symtab.get_module_scope("Geometry") == geo_scope);
      CHECK(symtab.get_module_scope("Math") == math_scope);
    }
  }

  TEST_CASE("Symbol_Table - scope stack") {
    Symbol_Table symtab;
    auto i32 = make_primitive_type(Primitive_Type::Kind::I32);

    // Create and enter module scope
    [[maybe_unused]] auto* module_scope = symtab.create_module_scope("Test");
    symtab.enter_scope(Scope_Kind::Module);

    SUBCASE("Current scope") {
      CHECK(symtab.current_scope() != nullptr);
      CHECK(symtab.current_scope()->kind() == Scope_Kind::Module);
    }

    SUBCASE("Nested scopes") {
      symtab.enter_scope(Scope_Kind::Function);
      CHECK(symtab.current_scope()->kind() == Scope_Kind::Function);

      symtab.enter_scope(Scope_Kind::Block);
      CHECK(symtab.current_scope()->kind() == Scope_Kind::Block);

      symtab.exit_scope();
      CHECK(symtab.current_scope()->kind() == Scope_Kind::Function);

      symtab.exit_scope();
      CHECK(symtab.current_scope()->kind() == Scope_Kind::Module);
    }

    SUBCASE("Declare in current scope") {
      auto error = symtab.declare("x", make_symbol("x", Symbol_Kind::Variable, i32));
      CHECK(!error);

      auto sym = symtab.lookup("x");
      REQUIRE(sym.has_value());
      if (sym.has_value()) {
        CHECK(sym.value().name == "x");
      }
    }

    SUBCASE("Lookup through scope chain") {
      [[maybe_unused]] auto err = symtab.declare("x", make_symbol("x", Symbol_Kind::Variable, i32));

      symtab.enter_scope(Scope_Kind::Block);
      auto sym = symtab.lookup("x");
      REQUIRE(sym.has_value());  // Can see parent's 'x'
      if (sym.has_value()) {
        CHECK(sym.value().name == "x");
      }
    }
  }

  TEST_CASE("Symbol_Table - builtin types") {
    CHECK(Builtin_Types::k_i32.to_string() == "I32");
    CHECK(Builtin_Types::k_bool_type.to_string() == "Bool");
    CHECK(Builtin_Types::k_string.to_string() == "String");
    CHECK(Builtin_Types::k_unit.to_string() == "()");
  }

  TEST_CASE("Visibility levels") {
    auto i32 = make_primitive_type(Primitive_Type::Kind::I32);

    SUBCASE("Module internal symbol") {
      auto sym = make_symbol("helper", Symbol_Kind::Function, i32, Visibility::Module_Internal);
      CHECK(sym.visibility == Visibility::Module_Internal);
    }

    SUBCASE("Public symbol") {
      auto sym = make_symbol("create", Symbol_Kind::Function, i32, Visibility::Public);
      CHECK(sym.visibility == Visibility::Public);
    }
  }

  TEST_CASE("Symbol kinds") {
    auto i32 = make_primitive_type(Primitive_Type::Kind::I32);

    SUBCASE("Variable symbol") {
      auto sym = make_symbol("x", Symbol_Kind::Variable, i32);
      CHECK(sym.kind == Symbol_Kind::Variable);
    }

    SUBCASE("Function symbol") {
      auto fn_type = make_function_type({i32, i32}, i32);
      auto sym = make_symbol("add", Symbol_Kind::Function, fn_type);
      CHECK(sym.kind == Symbol_Kind::Function);
    }

    SUBCASE("Type symbol") {
      auto point_type = make_struct_type("Point");
      auto sym = make_symbol("Point", Symbol_Kind::Type, point_type);
      CHECK(sym.kind == Symbol_Kind::Type);
    }
  }
}
