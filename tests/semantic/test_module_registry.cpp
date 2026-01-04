#include <doctest/doctest.h>

#include "parser/ast.hpp"
#include "semantic/module_registry.hpp"

using namespace life_lang;
using namespace life_lang::semantic;

TEST_SUITE("Module Registry") {
  TEST_CASE("Register and retrieve modules") {
    Module_Registry registry;
    ast::Module geometry_ast;
    ast::Module math_ast;

    // Register modules
    CHECK(registry.register_module("Geometry", &geometry_ast));
    CHECK(registry.register_module("Std.Math", &math_ast));

    // Retrieve modules
    auto const* geo = registry.get_module("Geometry");
    REQUIRE(geo != nullptr);
    CHECK(geo->path == "Geometry");
    CHECK(geo->ast == &geometry_ast);

    auto const* math = registry.get_module("Std.Math");
    REQUIRE(math != nullptr);
    CHECK(math->path == "Std.Math");
    CHECK(math->ast == &math_ast);

    // Non-existent module
    CHECK(registry.get_module("Unknown") == nullptr);
  }

  TEST_CASE("Duplicate registration fails") {
    Module_Registry registry;
    ast::Module const module_ast;

    CHECK(registry.register_module("Geometry", &module_ast));
    CHECK_FALSE(registry.register_module("Geometry", &module_ast));  // Duplicate
  }

  TEST_CASE("Check module existence") {
    Module_Registry registry;
    ast::Module const module_ast;

    CHECK_FALSE(registry.has_module("Geometry"));
    CHECK(registry.register_module("Geometry", &module_ast));
    CHECK(registry.has_module("Geometry"));
  }

  TEST_CASE("List all module paths") {
    Module_Registry registry;
    ast::Module const ast1;
    ast::Module const ast2;
    ast::Module const ast3;

    CHECK(registry.register_module("Geometry", &ast1));
    CHECK(registry.register_module("Std.Math", &ast2));
    CHECK(registry.register_module("Utils", &ast3));

    auto const paths = registry.all_module_paths();
    CHECK(paths.size() == 3);
    CHECK(std::ranges::find(paths, "Geometry") != paths.end());
    CHECK(std::ranges::find(paths, "Std.Math") != paths.end());
    CHECK(std::ranges::find(paths, "Utils") != paths.end());
  }

  TEST_CASE("Add dependencies") {
    Module_Registry registry;
    ast::Module const main_ast;
    ast::Module const geo_ast;
    ast::Module const math_ast;

    CHECK(registry.register_module("Main", &main_ast));
    CHECK(registry.register_module("Geometry", &geo_ast));
    CHECK(registry.register_module("Std.Math", &math_ast));
    // Main depends on Geometry and Math
    registry.add_dependency("Main", "Geometry");
    registry.add_dependency("Main", "Std.Math");

    auto const* main_info = registry.get_module("Main");
    REQUIRE(main_info != nullptr);
    CHECK(main_info->dependencies.size() == 2);
    CHECK(std::ranges::find(main_info->dependencies, "Geometry") != main_info->dependencies.end());
    CHECK(std::ranges::find(main_info->dependencies, "Std.Math") != main_info->dependencies.end());
  }

  TEST_CASE("Topological sort - no dependencies") {
    Module_Registry registry;
    ast::Module const ast1;
    ast::Module const ast2;
    ast::Module const ast3;

    CHECK(registry.register_module("A", &ast1));
    CHECK(registry.register_module("B", &ast2));
    CHECK(registry.register_module("C", &ast3));
    auto const sorted = registry.topological_sort();
    CHECK(sorted.size() == 3);
    // All modules should be present (order doesn't matter without dependencies)
    CHECK(std::ranges::find(sorted, "A") != sorted.end());
    CHECK(std::ranges::find(sorted, "B") != sorted.end());
    CHECK(std::ranges::find(sorted, "C") != sorted.end());
  }

  TEST_CASE("Topological sort - linear dependencies") {
    Module_Registry registry;
    ast::Module const a;
    ast::Module const b;
    ast::Module const c;

    CHECK(registry.register_module("A", &a));
    CHECK(registry.register_module("B", &b));
    CHECK(registry.register_module("C", &c));
    // C depends on B, B depends on A
    registry.add_dependency("C", "B");
    registry.add_dependency("B", "A");

    auto const sorted = registry.topological_sort();
    REQUIRE(sorted.size() == 3);

    // A should come before B, B before C
    CHECK(sorted == std::vector<std::string>{"A", "B", "C"});
  }

  TEST_CASE("Topological sort - diamond dependencies") {
    Module_Registry registry;
    ast::Module const a;
    ast::Module const b;
    ast::Module const c;
    ast::Module const d;

    CHECK(registry.register_module("A", &a));
    CHECK(registry.register_module("B", &b));
    CHECK(registry.register_module("C", &c));
    CHECK(registry.register_module("D", &d));

    // D depends on B and C, both B and C depend on A
    registry.add_dependency("D", "B");
    registry.add_dependency("D", "C");
    registry.add_dependency("B", "A");
    registry.add_dependency("C", "A");

    auto const sorted = registry.topological_sort();
    REQUIRE(sorted.size() == 4);

    // A must come first, D must come last
    CHECK(sorted[0] == "A");
    CHECK(sorted[3] == "D");
    // B and C in middle (order between them doesn't matter)
  }

  TEST_CASE("Topological sort - circular dependency detected") {
    Module_Registry registry;
    ast::Module const a;
    ast::Module const b;
    ast::Module const c;

    CHECK(registry.register_module("A", &a));
    CHECK(registry.register_module("B", &b));
    CHECK(registry.register_module("C", &c));

    // Create cycle: A -> B -> C -> A
    registry.add_dependency("A", "B");
    registry.add_dependency("B", "C");
    registry.add_dependency("C", "A");

    auto const sorted = registry.topological_sort();
    CHECK(sorted.empty());  // Should return empty on cycle
  }
}
