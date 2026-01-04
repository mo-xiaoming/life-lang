// Tests for name resolution

#include <doctest/doctest.h>

#include "diagnostics.hpp"
#include "parser/ast.hpp"
#include "semantic/name_resolver.hpp"
#include "semantic/symbol_table.hpp"
#include "semantic/type.hpp"

using namespace life_lang;
using namespace life_lang::semantic;
using namespace life_lang::ast;

TEST_SUITE("Name Resolution") {
  TEST_CASE("Resolve simple variable name") {
    Symbol_Table symtab;
    Diagnostic_Engine diag("<test>", "");
    Name_Resolver resolver(symtab, diag);

    // Enter a module scope and declare a variable
    symtab.enter_scope(Scope_Kind::Module);

    auto const symbol = make_symbol("x", Symbol_Kind::Variable, Primitive_Type{.kind = Primitive_Type::Kind::I32});
    auto const declare_result = symtab.declare(symbol);
    REQUIRE_FALSE(declare_result.has_value());  // Should succeed (no error)

    // Resolve the variable
    Var_Name const var_name{.segments = {{.value = "x", .type_params = {}}}};
    auto const result = resolver.resolve_var_name(var_name);

    REQUIRE(result.has_value());
    CHECK(result.value().name == "x");
    CHECK(result.value().kind == Symbol_Kind::Variable);
  }

  TEST_CASE("Undefined variable reports error") {
    Symbol_Table symtab;
    Diagnostic_Engine diag("<test>", "");
    Name_Resolver resolver(symtab, diag);

    symtab.enter_scope(Scope_Kind::Module);

    Var_Name const var_name{.segments = {{.value = "undefined", .type_params = {}}}};
    auto const result = resolver.resolve_var_name(var_name);

    CHECK_FALSE(result.has_value());
    CHECK(diag.has_errors());
  }

  TEST_CASE("Resolve simple type name") {
    Symbol_Table symtab;
    Diagnostic_Engine diag("<test>", "");
    Name_Resolver resolver(symtab, diag);

    symtab.enter_scope(Scope_Kind::Module);

    // Declare I32 as a primitive type
    auto const i32_type = Primitive_Type{.kind = Primitive_Type::Kind::I32};
    auto const symbol = make_symbol("I32", Symbol_Kind::Type, i32_type);
    auto const declare_result = symtab.declare(symbol);
    REQUIRE_FALSE(declare_result.has_value());

    // Resolve the type
    Type_Name const type_name = Path_Type{.segments = {{.value = "I32", .type_params = {}}}};
    auto const result = resolver.resolve_type_name(type_name);

    REQUIRE(result.has_value());
    CHECK(std::holds_alternative<Primitive_Type>(result.value()));
  }

  TEST_CASE("Undefined type reports error") {
    Symbol_Table symtab;
    Diagnostic_Engine diag("<test>", "");
    Name_Resolver resolver(symtab, diag);

    [[maybe_unused]] auto* scope = symtab.create_module_scope("test");

    Type_Name const type_name = Path_Type{.segments = {{.value = "UnknownType", .type_params = {}}}};
    auto const result = resolver.resolve_type_name(type_name);

    CHECK_FALSE(result.has_value());
    CHECK(diag.has_errors());
  }
}
