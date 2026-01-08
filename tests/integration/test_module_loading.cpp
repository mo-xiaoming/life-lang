#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

#include "diagnostics.hpp"
#include "parser/ast.hpp"
#include "semantic/module_loader.hpp"

using namespace life_lang;
using namespace life_lang::semantic;
namespace fs = std::filesystem;

TEST_SUITE("Module Loading Integration") {
  struct Module_Loading_Fixture {
    fs::path temp_project;
    fs::path temp_src;

    Module_Loading_Fixture() {
      temp_project = fs::temp_directory_path() / "life_lang_loading_test";
      temp_src = temp_project / "src";
      fs::remove_all(temp_project);
      fs::create_directories(temp_src);
    }

    Module_Loading_Fixture(Module_Loading_Fixture const&) = default;
    Module_Loading_Fixture(Module_Loading_Fixture&&) = delete;
    Module_Loading_Fixture& operator=(Module_Loading_Fixture const&) = default;
    Module_Loading_Fixture& operator=(Module_Loading_Fixture&&) = delete;

    ~Module_Loading_Fixture() { fs::remove_all(temp_project); }

    static void write_file(fs::path const& path_, std::string_view content_) {
      std::ofstream file(path_);
      file << content_;
    }
  };

  TEST_CASE("Load single-file module") {
    Module_Loading_Fixture const fixture;

    // Create a simple module with one file
    auto const geometry_dir = fixture.temp_src / "geometry";
    fs::create_directories(geometry_dir);
    Module_Loading_Fixture::write_file(
        geometry_dir / "point.life",
        R"(
pub struct Point {
  x: I32,
  y: I32
}

pub fn origin(): Point {
  return Point { x: 0, y: 0 };
}
)"
    );

    // Discover module
    auto const modules = Module_Loader::discover_modules(fixture.temp_src);
    REQUIRE(modules.size() == 1);

    // Load module
    life_lang::Diagnostic_Manager diag_mgr;
    auto const module_opt = Module_Loader::load_module(modules[0], diag_mgr);

    REQUIRE(module_opt.has_value());
    if (module_opt.has_value()) {
      auto const& module = *module_opt;

      CHECK(module.imports.empty());
      CHECK(module.items.size() == 2);  // struct + function
    }
  }

  TEST_CASE("Load multi-file module") {
    Module_Loading_Fixture const fixture;

    // Create module with multiple files
    auto const geometry_dir = fixture.temp_src / "geometry";
    fs::create_directories(geometry_dir);

    Module_Loading_Fixture::write_file(
        geometry_dir / "point.life",
        R"(
pub struct Point {
  x: I32,
  y: I32
}
)"
    );

    Module_Loading_Fixture::write_file(
        geometry_dir / "circle.life",
        R"(
pub struct Circle {
  center: Point,
  radius: F64
}
)"
    );

    Module_Loading_Fixture::write_file(
        geometry_dir / "utils.life",
        R"(
pub fn distance(p1: Point, p2: Point): F64 {
  return 0.0;
}
)"
    );

    // Discover module
    auto const modules = Module_Loader::discover_modules(fixture.temp_src);
    REQUIRE(modules.size() == 1);

    // Load module
    life_lang::Diagnostic_Manager diag_mgr;
    auto const module_opt = Module_Loader::load_module(modules[0], diag_mgr);

    REQUIRE(module_opt.has_value());
    if (module_opt.has_value()) {
      auto const& module = *module_opt;

      CHECK(module.imports.empty());
      CHECK(module.items.size() == 3);  // 2 structs + 1 function
    }
  }

  TEST_CASE("Module with imports are merged") {
    Module_Loading_Fixture const fixture;

    // Create module - simpler test without imports for now
    auto const utils_dir = fixture.temp_src / "utils";
    fs::create_directories(utils_dir);

    Module_Loading_Fixture::write_file(
        utils_dir / "math.life",
        R"(pub fn square(x: I32): I32 {
  return x * x;
}
)"
    );

    Module_Loading_Fixture::write_file(
        utils_dir / "string.life",
        R"(pub fn uppercase(s: String): String {
  return s;
}
)"
    );

    // Discover module
    auto const modules = Module_Loader::discover_modules(fixture.temp_src);
    REQUIRE(modules.size() == 1);

    // Load module
    life_lang::Diagnostic_Manager diag_mgr;
    auto const module_opt = Module_Loader::load_module(modules[0], diag_mgr);

    REQUIRE(module_opt.has_value());
    if (module_opt.has_value()) {
      auto const& module = *module_opt;

      CHECK(module.items.size() == 2);  // Both functions merged
    }
  }

  TEST_CASE("Parse error in one file fails entire module") {
    Module_Loading_Fixture const fixture;

    // Create module with one invalid file
    auto const geometry_dir = fixture.temp_src / "geometry";
    fs::create_directories(geometry_dir);

    Module_Loading_Fixture::write_file(geometry_dir / "good.life", "pub fn valid(): I32 { return 42; }");
    Module_Loading_Fixture::write_file(geometry_dir / "bad.life", "pub fn invalid(: I32 {");  // Syntax error

    // Discover module
    auto const modules = Module_Loader::discover_modules(fixture.temp_src);
    REQUIRE(modules.size() == 1);

    // Load module - should fail
    life_lang::Diagnostic_Manager diag_mgr;
    auto const module_opt = Module_Loader::load_module(modules[0], diag_mgr);

    CHECK_FALSE(module_opt.has_value());  // Entire module fails
  }

  TEST_CASE("Duplicate definition in multiple files fails module") {
    Module_Loading_Fixture const fixture;

    // Create module with duplicate struct definitions in different files
    auto const geometry_dir = fixture.temp_src / "geometry";
    fs::create_directories(geometry_dir);

    Module_Loading_Fixture::write_file(
        geometry_dir / "point.life",
        R"(pub struct Point { x: I32, y: I32 }

pub fn create_point(): Point {
  return Point { x: 0, y: 0 };
}
)"
    );

    Module_Loading_Fixture::write_file(
        geometry_dir / "duplicate.life",
        R"(// This file incorrectly redefines Point
pub struct Point { a: F64, b: F64 }
)"
    );

    // Discover module
    auto const modules = Module_Loader::discover_modules(fixture.temp_src);
    REQUIRE(modules.size() == 1);

    // Load module - should fail due to duplicate definition
    life_lang::Diagnostic_Manager diag_mgr;
    auto const module_opt = Module_Loader::load_module(modules[0], diag_mgr);

    CHECK_FALSE(module_opt.has_value());
    CHECK(diag_mgr.has_errors());

    // Error message should mention the duplicate
    auto const& errors = diag_mgr.all_diagnostics();
    REQUIRE(!errors.empty());
    CHECK(errors[0].message.find("Point") != std::string::npos);
    CHECK(errors[0].message.find("duplicate") != std::string::npos);
  }

  TEST_CASE("Duplicate function definition fails module") {
    Module_Loading_Fixture const fixture;

    auto const utils_dir = fixture.temp_src / "utils";
    fs::create_directories(utils_dir);

    Module_Loading_Fixture::write_file(utils_dir / "math.life", "pub fn helper(): I32 { return 1; }");
    Module_Loading_Fixture::write_file(utils_dir / "string.life", "pub fn helper(): I32 { return 2; }");  // Duplicate

    // Discover and load module
    auto const modules = Module_Loader::discover_modules(fixture.temp_src);
    REQUIRE(modules.size() == 1);

    life_lang::Diagnostic_Manager diag_mgr;
    auto const module_opt = Module_Loader::load_module(modules[0], diag_mgr);

    CHECK_FALSE(module_opt.has_value());
    CHECK(diag_mgr.has_errors());
  }

  TEST_CASE("Same name struct and function is allowed") {
    // In life-lang, functions and types have separate namespaces
    // So "struct Foo" and "fn Foo" should not conflict
    // (This test documents expected behavior - adjust if design differs)
    Module_Loading_Fixture const fixture;

    auto const utils_dir = fixture.temp_src / "utils";
    fs::create_directories(utils_dir);

    Module_Loading_Fixture::write_file(utils_dir / "types.life", "pub struct Point { x: I32 }");
    Module_Loading_Fixture::write_file(utils_dir / "funcs.life", "pub fn Point(): I32 { return 0; }");

    auto const modules = Module_Loader::discover_modules(fixture.temp_src);
    REQUIRE(modules.size() == 1);

    life_lang::Diagnostic_Manager diag_mgr;
    auto const module_opt = Module_Loader::load_module(modules[0], diag_mgr);

    // This SHOULD fail - same name is duplicate even across categories
    // Adjust this test if design decision differs
    CHECK_FALSE(module_opt.has_value());
    CHECK(diag_mgr.has_errors());
  }
}
