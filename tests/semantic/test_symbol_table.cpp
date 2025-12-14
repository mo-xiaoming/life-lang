#include <catch2/catch_test_macros.hpp>
#include "symbol_table.hpp"

using life_lang::Scope_Kind;
using life_lang::Source_Location;
using life_lang::Symbol;
using life_lang::Symbol_Kind;
using life_lang::Symbol_Table;

TEST_CASE("Symbol_Table - Basic operations", "[symbol_table]") {
  Symbol_Table table;

  SECTION("Starts with global scope") {
    REQUIRE(table.current_scope_kind() == Scope_Kind::Global);
  }

  SECTION("Insert and lookup symbol") {
    Symbol const func{
        .name = "main",
        .kind = Symbol_Kind::Function,
        .location = Source_Location{.file = "test.life", .line = 1, .column = 1},
        .type_annotation = "fn(): I32",
        .generic_params = {},
    };

    REQUIRE(table.insert(func));

    auto const* found = table.lookup("main");
    REQUIRE(found != nullptr);
    REQUIRE(found->name == "main");
    REQUIRE(found->kind == Symbol_Kind::Function);
  }

  SECTION("Duplicate symbols rejected") {
    Symbol const func1{
        .name = "foo",
        .kind = Symbol_Kind::Function,
        .location = Source_Location{.file = "test.life", .line = 1, .column = 1},
        .type_annotation = "fn(): I32",
        .generic_params = {},
    };

    Symbol const func2{
        .name = "foo",
        .kind = Symbol_Kind::Function,
        .location = Source_Location{.file = "test.life", .line = 10, .column = 1},
        .type_annotation = "fn(): Bool",
        .generic_params = {},
    };

    REQUIRE(table.insert(func1));
    REQUIRE_FALSE(table.insert(func2));  // Duplicate
  }

  SECTION("Undefined symbol lookup returns nullptr") {
    auto const* found = table.lookup("nonexistent");
    REQUIRE(found == nullptr);
  }
}

TEST_CASE("Symbol_Table - Scoping", "[symbol_table]") {
  Symbol_Table table;

  SECTION("Nested scopes") {
    // Global: x = 1
    Symbol const global_x{
        .name = "x",
        .kind = Symbol_Kind::Variable,
        .location = Source_Location{.file = "test.life", .line = 1, .column = 1},
        .type_annotation = "I32",
        .generic_params = {},
    };
    REQUIRE(table.insert(global_x));

    // Function scope
    table.enter_scope(Scope_Kind::Function);
    REQUIRE(table.current_scope_kind() == Scope_Kind::Function);

    // Can still see global x
    auto const* found = table.lookup("x");
    REQUIRE(found != nullptr);
    REQUIRE(found->name == "x");

    // Shadow with local x
    Symbol const local_x{
        .name = "x",
        .kind = Symbol_Kind::Variable,
        .location = Source_Location{.file = "test.life", .line = 5, .column = 5},
        .type_annotation = "String",
        .generic_params = {},
    };
    REQUIRE(table.insert(local_x));

    // Lookup finds local x (shadows global)
    found = table.lookup("x");
    REQUIRE(found != nullptr);
    REQUIRE(found->type_annotation == "String");

    // Exit function scope
    table.exit_scope();
    REQUIRE(table.current_scope_kind() == Scope_Kind::Global);

    // Back to global x
    found = table.lookup("x");
    REQUIRE(found != nullptr);
    REQUIRE(found->type_annotation == "I32");
  }

  SECTION("Multiple nested scopes") {
    table.enter_scope(Scope_Kind::Function);
    table.enter_scope(Scope_Kind::Block);
    table.enter_scope(Scope_Kind::Block);

    Symbol const var{
        .name = "y",
        .kind = Symbol_Kind::Variable,
        .location = Source_Location{.file = "test.life", .line = 1, .column = 1},
        .type_annotation = "Bool",
        .generic_params = {},
    };
    REQUIRE(table.insert(var));

    auto const* found = table.lookup("y");
    REQUIRE(found != nullptr);

    table.exit_scope();
    found = table.lookup("y");
    REQUIRE(found == nullptr);  // Out of scope
  }
}

TEST_CASE("Symbol_Table - Type vs value lookups", "[symbol_table]") {
  Symbol_Table table;

  Symbol const point_type{
      .name = "Point",
      .kind = Symbol_Kind::Type,
      .location = Source_Location{.file = "test.life", .line = 1, .column = 1},
      .type_annotation = "struct",
      .generic_params = {},
  };

  Symbol const point_var{
      .name = "point",
      .kind = Symbol_Kind::Variable,
      .location = Source_Location{.file = "test.life", .line = 10, .column = 1},
      .type_annotation = "Point",
      .generic_params = {},
  };

  REQUIRE(table.insert(point_type));
  REQUIRE(table.insert(point_var));

  SECTION("lookup_type finds only types") {
    auto const* found = table.lookup_type("Point");
    REQUIRE(found != nullptr);
    REQUIRE(found->kind == Symbol_Kind::Type);

    found = table.lookup_type("point");
    REQUIRE(found == nullptr);  // Not a type
  }

  SECTION("lookup_value finds only values") {
    auto const* found = table.lookup_value("point");
    REQUIRE(found != nullptr);
    REQUIRE(found->kind == Symbol_Kind::Variable);

    found = table.lookup_value("Point");
    REQUIRE(found == nullptr);  // Not a value
  }
}

TEST_CASE("Symbol_Table - Context queries", "[symbol_table]") {
  Symbol_Table table;

  SECTION("in_impl_scope") {
    REQUIRE_FALSE(table.in_impl_scope());

    table.enter_scope(Scope_Kind::Impl);
    REQUIRE(table.in_impl_scope());

    table.enter_scope(Scope_Kind::Block);  // Nested inside impl
    REQUIRE(table.in_impl_scope());

    table.exit_scope();
    table.exit_scope();
    REQUIRE_FALSE(table.in_impl_scope());
  }

  SECTION("in_function_scope") {
    REQUIRE_FALSE(table.in_function_scope());

    table.enter_scope(Scope_Kind::Function);
    REQUIRE(table.in_function_scope());

    table.exit_scope();
    REQUIRE_FALSE(table.in_function_scope());
  }

  SECTION("in_loop_scope") {
    REQUIRE_FALSE(table.in_loop_scope());

    table.enter_scope(Scope_Kind::Function);
    table.enter_scope(Scope_Kind::Loop);
    REQUIRE(table.in_loop_scope());

    table.exit_scope();
    REQUIRE_FALSE(table.in_loop_scope());
  }
}

TEST_CASE("Symbol_Table - Generic parameters", "[symbol_table]") {
  Symbol_Table table;

  Symbol const vec_type{
      .name = "Vec",
      .kind = Symbol_Kind::Type,
      .location = Source_Location{.file = "test.life", .line = 1, .column = 1},
      .type_annotation = "struct",
      .generic_params = {"T"},
  };

  REQUIRE(table.insert(vec_type));

  auto const* found = table.lookup("Vec");
  REQUIRE(found != nullptr);
  REQUIRE(found->generic_params.size() == 1);
  REQUIRE(found->generic_params[0] == "T");
}
