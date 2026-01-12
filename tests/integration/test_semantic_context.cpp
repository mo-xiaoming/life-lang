#include <doctest/doctest.h>

#include "diagnostics.hpp"
#include "semantic/semantic_context.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>

using life_lang::Diagnostic_Manager;
using namespace life_lang::semantic;
namespace fs = std::filesystem;

// Test fixture for creating temporary module directories
struct Temp_Module_Fixture {
  fs::path temp_dir;
  fs::path temp_src;

  Temp_Module_Fixture() {
    auto const now = std::chrono::system_clock::now();
    auto const timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    temp_dir = fs::temp_directory_path() / ("life_test_" + std::to_string(timestamp));
    temp_src = temp_dir / "src";
    fs::create_directories(temp_src);
  }

  Temp_Module_Fixture(Temp_Module_Fixture const&) = default;
  Temp_Module_Fixture(Temp_Module_Fixture&&) = delete;
  Temp_Module_Fixture& operator=(Temp_Module_Fixture const&) = default;
  Temp_Module_Fixture& operator=(Temp_Module_Fixture&&) = delete;

  ~Temp_Module_Fixture() {
    if (fs::exists(temp_dir)) {
      fs::remove_all(temp_dir);
    }
  }

  void create_file(fs::path const& relative_path_, std::string const& content_) const {
    auto const full_path = temp_src / relative_path_;
    fs::create_directories(full_path.parent_path());
    std::ofstream file(full_path);
    file << content_;
  }
};

TEST_SUITE("Semantic Context") {
  TEST_CASE("Load single module with function") {
    Temp_Module_Fixture const fixture;
    // Modules are directories, not single files
    fs::create_directories(fixture.temp_src / "math");
    fixture.create_file(
        "math/operations.life",
        "pub fn add(x: I32, y: I32): I32 { return x + y; }\n"
        "fn helper(): I32 { return 42; }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    auto const paths = ctx.module_paths();
    REQUIRE(paths.size() == 1);
    CHECK(paths[0] == "Math");

    // Check module exists
    auto const* module = ctx.get_module("Math");
    REQUIRE(module != nullptr);
    CHECK(module->items.size() == 2);

    // Find public function
    auto const* add_func = ctx.find_func_def("Math", "add");
    REQUIRE(add_func != nullptr);
    CHECK(add_func->is_pub);

    // Find private function
    auto const* helper_func = ctx.find_func_def("Math", "helper");
    REQUIRE(helper_func != nullptr);
    CHECK(!helper_func->is_pub);

    // Non-existent function
    CHECK(ctx.find_func_def("Math", "nonexistent") == nullptr);
  }

  TEST_CASE("Load module with struct definition") {
    Temp_Module_Fixture const fixture;
    fs::create_directories(fixture.temp_src / "geometry");
    fixture.create_file(
        "geometry/shapes.life",
        "pub struct Point { pub x: I32, pub y: I32 }\n"
        "struct Internal_Helper { data: I32 }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Find public struct
    auto const* point_struct = ctx.find_type_def("Geometry", "Point");
    REQUIRE(point_struct != nullptr);
    CHECK(point_struct->is_pub);

    // Find private struct
    auto const* helper_struct = ctx.find_type_def("Geometry", "Internal_Helper");
    REQUIRE(helper_struct != nullptr);
    CHECK(!helper_struct->is_pub);

    // Non-existent type
    CHECK(ctx.find_type_def("Geometry", "Circle") == nullptr);
  }

  TEST_CASE("Load multiple modules") {
    Temp_Module_Fixture const fixture;
    fs::create_directories(fixture.temp_src / "math");
    fs::create_directories(fixture.temp_src / "geometry");
    fs::create_directories(fixture.temp_src / "utils" / "string");

    fixture.create_file("math/ops.life", "pub fn add(x: I32, y: I32): I32 { return x + y; }\n");
    fixture.create_file("geometry/shapes.life", "pub struct Point { x: I32, y: I32 }\n");
    fixture.create_file("utils/string/util.life", "pub fn length(s: String): I32 { return 0; }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    auto const paths = ctx.module_paths();
    REQUIRE(paths.size() == 3);

    // All modules should be accessible
    CHECK(ctx.get_module("Math") != nullptr);
    CHECK(ctx.get_module("Geometry") != nullptr);
    CHECK(ctx.get_module("Utils.String") != nullptr);

    // Check each module has its definitions
    CHECK(ctx.find_func_def("Math", "add") != nullptr);
    CHECK(ctx.find_type_def("Geometry", "Point") != nullptr);
    CHECK(ctx.find_func_def("Utils.String", "length") != nullptr);

    // Cross-module searches should fail
    CHECK(ctx.find_func_def("Geometry", "add") == nullptr);
    CHECK(ctx.find_type_def("Math", "Point") == nullptr);
  }

  TEST_CASE("Get non-existent module") {
    Temp_Module_Fixture const fixture;
    fs::create_directories(fixture.temp_src / "math");
    fixture.create_file("math/ops.life", "pub fn add(x: I32, y: I32): I32 { return x + y; }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    CHECK(ctx.get_module("NonExistent") == nullptr);
    CHECK(ctx.get_module("Math.Nested") == nullptr);
  }

  TEST_CASE("Load fails with parse error") {
    Temp_Module_Fixture const fixture;
    fs::create_directories(fixture.temp_src / "broken");
    fixture.create_file("broken/bad.life", "pub fn broken syntax error\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    // Should return false due to parse error
    CHECK(!ctx.load_modules(fixture.temp_src));
  }

  TEST_CASE("Find type definitions by kind") {
    Temp_Module_Fixture const fixture;
    fs::create_directories(fixture.temp_src / "types");
    fixture.create_file(
        "types/defs.life",
        "pub struct Point { x: I32, y: I32 }\n"
        "pub enum Option<T> { Some(T), None }\n"
        "pub trait Display { fn show(self): String; }\n"
        "pub type Distance = F64;\n"
        "pub fn not_a_type(): I32 { return 0; }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // All type definitions should be found
    CHECK(ctx.find_type_def("Types", "Point") != nullptr);
    CHECK(ctx.find_type_def("Types", "Option") != nullptr);
    CHECK(ctx.find_type_def("Types", "Display") != nullptr);
    CHECK(ctx.find_type_def("Types", "Distance") != nullptr);

    // Function should NOT be found by find_type_def
    CHECK(ctx.find_type_def("Types", "not_a_type") == nullptr);
    // But should be found by find_func_def
    CHECK(ctx.find_func_def("Types", "not_a_type") != nullptr);
  }

  TEST_CASE("Find methods in impl blocks") {
    Temp_Module_Fixture const fixture;
    fs::create_directories(fixture.temp_src / "geometry");
    fixture.create_file(
        "geometry/point.life",
        "pub struct Point { pub x: I32, pub y: I32 }\n"
        "\n"
        "impl Point {\n"
        "  pub fn distance(self): F64 { return 0.0; }\n"
        "  fn internal_helper(self): I32 { return 0; }\n"
        "}\n"
        "\n"
        "pub fn free_function(): I32 { return 42; }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Find public method in impl block
    auto const* distance_method = ctx.find_method_def("Geometry", "Point", "distance");
    REQUIRE(distance_method != nullptr);
    CHECK(distance_method->is_pub);
    CHECK(distance_method->declaration.name == "distance");

    // Find private method in impl block
    auto const* helper_method = ctx.find_method_def("Geometry", "Point", "internal_helper");
    REQUIRE(helper_method != nullptr);
    CHECK(!helper_method->is_pub);

    // Non-existent method
    CHECK(ctx.find_method_def("Geometry", "Point", "nonexistent") == nullptr);

    // Non-existent type
    CHECK(ctx.find_method_def("Geometry", "Circle", "distance") == nullptr);

    // Free function should NOT be found via find_method_def
    CHECK(ctx.find_method_def("Geometry", "Point", "free_function") == nullptr);
    // But should be found via find_func_def
    CHECK(ctx.find_func_def("Geometry", "free_function") != nullptr);
  }

  TEST_CASE("Find methods in generic impl blocks") {
    Temp_Module_Fixture const fixture;
    fs::create_directories(fixture.temp_src / "collections");
    fixture.create_file(
        "collections/list.life",
        "pub struct List<T> { data: T }\n"
        "\n"
        "impl<T> List<T> {\n"
        "  pub fn len(self): I32 { return 0; }\n"
        "  pub fn push(mut self, item: T): () { return (); }\n"
        "}\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Find method in generic impl block - type name is just "List" (without type params)
    auto const* len_method = ctx.find_method_def("Collections", "List", "len");
    REQUIRE(len_method != nullptr);
    CHECK(len_method->declaration.name == "len");

    auto const* push_method = ctx.find_method_def("Collections", "List", "push");
    REQUIRE(push_method != nullptr);
    CHECK(push_method->declaration.name == "push");
  }

  TEST_CASE("Empty module loads successfully") {
    Temp_Module_Fixture const fixture;
    fs::create_directories(fixture.temp_src / "empty");
    fixture.create_file("empty/nothing.life", "// Just a comment\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    auto const* module = ctx.get_module("Empty");
    REQUIRE(module != nullptr);
    CHECK(module->items.empty());
  }

  TEST_CASE("Module with only imports") {
    Temp_Module_Fixture const fixture;
    fs::create_directories(fixture.temp_src / "importer");
    fixture.create_file(
        "importer/main.life",
        "import Std.IO.{ println };\n"
        "// No other items\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    auto const* module = ctx.get_module("Importer");
    REQUIRE(module != nullptr);
    CHECK(module->items.empty());
    CHECK(module->imports.size() == 1);
  }

  TEST_CASE("Circular import detected - simple A -> B -> A") {
    Temp_Module_Fixture const fixture;

    // Module A imports from B
    fs::create_directories(fixture.temp_src / "module_a");
    fixture.create_file(
        "module_a/main.life",
        "import Module_B.{ helper_b };\n"
        "pub fn helper_a(): I32 { return 1; }\n"
    );

    // Module B imports from A (circular!)
    fs::create_directories(fixture.temp_src / "module_b");
    fixture.create_file(
        "module_b/main.life",
        "import Module_A.{ helper_a };\n"
        "pub fn helper_b(): I32 { return 2; }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);

    // Should fail due to circular import
    CHECK_FALSE(ctx.load_modules(fixture.temp_src));
    CHECK(diag_mgr.has_errors());

    // Error message should mention "circular"
    auto const& errors = diag_mgr.all_diagnostics();
    REQUIRE(!errors.empty());
    CHECK(errors[0].message.find("circular") != std::string::npos);
  }

  TEST_CASE("Circular import detected - chain A -> B -> C -> A") {
    Temp_Module_Fixture const fixture;

    fs::create_directories(fixture.temp_src / "mod_a");
    fixture.create_file(
        "mod_a/main.life",
        "import Mod_B.{ b_func };\n"
        "pub fn a_func(): I32 { return 1; }\n"
    );

    fs::create_directories(fixture.temp_src / "mod_b");
    fixture.create_file(
        "mod_b/main.life",
        "import Mod_C.{ c_func };\n"
        "pub fn b_func(): I32 { return 2; }\n"
    );

    fs::create_directories(fixture.temp_src / "mod_c");
    fixture.create_file(
        "mod_c/main.life",
        "import Mod_A.{ a_func };\n"  // Circular back to A
        "pub fn c_func(): I32 { return 3; }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);

    CHECK_FALSE(ctx.load_modules(fixture.temp_src));
    CHECK(diag_mgr.has_errors());
  }

  TEST_CASE("Self-import is circular") {
    Temp_Module_Fixture const fixture;

    fs::create_directories(fixture.temp_src / "selfie");
    fixture.create_file(
        "selfie/main.life",
        "import Selfie.{ helper };\n"  // Importing from self!
        "pub fn helper(): I32 { return 1; }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);

    CHECK_FALSE(ctx.load_modules(fixture.temp_src));
    CHECK(diag_mgr.has_errors());
  }

  TEST_CASE("Non-circular imports succeed") {
    Temp_Module_Fixture const fixture;

    // Linear dependency: C -> B -> A (no cycle)
    fs::create_directories(fixture.temp_src / "base");
    fixture.create_file("base/main.life", "pub fn base_func(): I32 { return 1; }\n");

    fs::create_directories(fixture.temp_src / "middle");
    fixture.create_file(
        "middle/main.life",
        "import Base.{ base_func };\n"
        "pub fn middle_func(): I32 { return 2; }\n"
    );

    fs::create_directories(fixture.temp_src / "top");
    fixture.create_file(
        "top/main.life",
        "import Middle.{ middle_func };\n"
        "pub fn top_func(): I32 { return 3; }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);

    // Should succeed - no circular imports
    CHECK(ctx.load_modules(fixture.temp_src));
    CHECK_FALSE(diag_mgr.has_errors());

    // All modules should be loaded
    CHECK(ctx.get_module("Base") != nullptr);
    CHECK(ctx.get_module("Middle") != nullptr);
    CHECK(ctx.get_module("Top") != nullptr);
  }
}
