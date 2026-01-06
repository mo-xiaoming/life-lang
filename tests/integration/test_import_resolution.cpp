#include <doctest/doctest.h>

#include "diagnostics.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_context.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>

using namespace life_lang::semantic;
namespace fs = std::filesystem;

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

TEST_SUITE("Import Resolution") {
  TEST_CASE("Simple import - type resolution") {
    Temp_Module_Fixture const fixture;

    // Create Geometry module with pub struct Point
    fs::create_directories(fixture.temp_src / "geometry");
    fixture.create_file("geometry/types.life", "pub struct Point { x: I32, y: I32 }\n");

    // Create Main module that imports Point
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/app.life",
        "import Geometry.{ Point };\n"
        "pub fn make_point(): Point { return Point { x: 0, y: 0 }; }\n"
    );

    Semantic_Context ctx;
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse a Point type reference in Main module context
    life_lang::Diagnostic_Engine diag("<test>", "Point");
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    // Resolve Point - should find it via import from Geometry
    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      REQUIRE(result.has_value());
      if (result.has_value()) {
        auto const& [module_path, item] = *result;
        CHECK(module_path == "Geometry");
        CHECK(item != nullptr);
        CHECK(item->is_pub);
      }
    }
  }

  TEST_CASE("Aliased import - type resolution") {
    Temp_Module_Fixture const fixture;

    // Create Geometry module
    fs::create_directories(fixture.temp_src / "geometry");
    fixture.create_file("geometry/types.life", "pub struct Circle { radius: F64 }\n");

    // Create Main module with aliased import
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/app.life",
        "import Geometry.{ Circle as C };\n"
        "pub fn make_circle(): C { return C { radius: 1.0 }; }\n"
    );

    Semantic_Context ctx;
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Resolve using the alias "C"
    life_lang::Diagnostic_Engine diag("<test>", "C");
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      REQUIRE(result.has_value());
      if (result.has_value()) {
        auto const& [module_path, item] = *result;
        CHECK(module_path == "Geometry");
        CHECK(item != nullptr);
      }
    }
  }

  TEST_CASE("Multi-level module path") {
    Temp_Module_Fixture const fixture;

    // Create nested module: Std/Collections
    fs::create_directories(fixture.temp_src / "std" / "collections");
    fixture.create_file("std/collections/vec.life", "pub struct Vec { size: I32 }\n");

    // Create Main module with multi-level import
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/app.life",
        "import Std.Collections.{ Vec };\n"
        "pub fn make_vec(): Vec { return Vec { size: 0 }; }\n"
    );

    Semantic_Context ctx;
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Resolve Vec
    life_lang::Diagnostic_Engine diag("<test>", "Vec");
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      REQUIRE(result.has_value());
      if (result.has_value()) {
        auto const& [module_path, item] = *result;
        CHECK(module_path == "Std.Collections");
      }
    }
  }

  TEST_CASE("Fully qualified name - no import needed") {
    Temp_Module_Fixture const fixture;

    // Create Geometry module
    fs::create_directories(fixture.temp_src / "geometry");
    fixture.create_file("geometry/types.life", "pub struct Point { x: I32, y: I32 }\n");

    // Create Main module WITHOUT import - uses fully qualified name
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/app.life", "pub fn uses_geometry(): I32 { return 42; }\n");

    Semantic_Context ctx;
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse fully qualified type: Geometry.Point
    life_lang::Diagnostic_Engine diag("<test>", "Geometry.Point");
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    // Resolve should work without import
    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      REQUIRE(result.has_value());
      if (result.has_value()) {
        auto const& [module_path, item] = *result;
        CHECK(module_path == "Geometry");
      }
    }
  }

  TEST_CASE("Local definition takes precedence over import") {
    Temp_Module_Fixture const fixture;

    // Create Geometry module
    fs::create_directories(fixture.temp_src / "geometry");
    fixture.create_file("geometry/types.life", "pub struct Point { x: I32, y: I32 }\n");

    // Create Main module with local Point AND import
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/app.life",
        "import Geometry.{ Point };\n"
        "struct Point { value: String }\n"  // Local definition
        "fn test(): Point { return Point { value: \"local\" }; }\n"
    );

    Semantic_Context ctx;
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Resolve Point - should find local definition, not imported one
    life_lang::Diagnostic_Engine diag("<test>", "Point");
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      REQUIRE(result.has_value());
      if (result.has_value()) {
        auto const& [module_path, item] = *result;
        CHECK(module_path == "Main");  // Local, not Geometry
      }
    }
  }

  TEST_CASE("Function import and resolution") {
    Temp_Module_Fixture const fixture;

    // Create Math module with pub function
    fs::create_directories(fixture.temp_src / "math");
    fixture.create_file("math/ops.life", "pub fn add(x: I32, y: I32): I32 { return x + y; }\n");

    // Create Main module that imports add
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/app.life",
        "import Math.{ add };\n"
        "pub fn calculate(): I32 { return add(1, 2); }\n"
    );

    Semantic_Context ctx;
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse function call: add
    life_lang::Diagnostic_Engine diag("<test>", "add");
    life_lang::parser::Parser parser(diag);
    auto const var_name_opt = parser.parse_variable_name();
    REQUIRE(var_name_opt.has_value());

    // Resolve add - should find it via import
    if (var_name_opt.has_value()) {
      auto const result = ctx.resolve_var_name("Main", *var_name_opt);
      REQUIRE(result.has_value());
      if (result.has_value()) {
        auto const& [module_path, item] = *result;
        CHECK(module_path == "Math");
        CHECK(item->is_pub);
      }
    }
  }

  TEST_CASE("Non-pub type cannot be imported") {
    Temp_Module_Fixture const fixture;

    // Create Geometry module with non-pub struct
    fs::create_directories(fixture.temp_src / "geometry");
    fixture.create_file("geometry/types.life", "struct Internal { x: I32 }\n");  // No pub!

    // Create Main module that tries to import Internal
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/app.life",
        "import Geometry.{ Internal };\n"
        "fn test(): Internal { return Internal { x: 0 }; }\n"
    );

    Semantic_Context ctx;
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Try to resolve Internal
    life_lang::Diagnostic_Engine diag("<test>", "Internal");
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    // Should NOT resolve because Internal is not pub
    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      CHECK(!result.has_value());
    }
  }

  TEST_CASE("Non-existent type - import fails silently") {
    Temp_Module_Fixture const fixture;

    // Create Geometry module WITHOUT Point
    fs::create_directories(fixture.temp_src / "geometry");
    fixture.create_file("geometry/types.life", "pub struct Circle { r: F64 }\n");

    // Create Main module that tries to import non-existent Point
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/app.life",
        "import Geometry.{ Point };\n"  // Point doesn't exist!
        "fn test(): I32 { return 0; }\n"
    );

    Semantic_Context ctx;
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Try to resolve Point
    life_lang::Diagnostic_Engine diag("<test>", "Point");
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    // Should not resolve
    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      CHECK(!result.has_value());
    }
  }

  TEST_CASE("Multiple imports from same module") {
    Temp_Module_Fixture const fixture;

    // Create Geometry module with multiple types
    fs::create_directories(fixture.temp_src / "geometry");
    fixture.create_file(
        "geometry/types.life",
        "pub struct Point { x: I32, y: I32 }\n"
        "pub struct Circle { center: Point, radius: F64 }\n"
        "pub struct Line { start: Point, end: Point }\n"
    );

    // Create Main module importing multiple items
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/app.life",
        "import Geometry.{ Point, Circle, Line };\n"
        "pub fn test(): Point { return Point { x: 0, y: 0 }; }\n"
    );

    Semantic_Context ctx;
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // All three should resolve
    for (auto const* name: {"Point", "Circle", "Line"}) {
      life_lang::Diagnostic_Engine diag("<test>", name);
      life_lang::parser::Parser parser(diag);
      auto const type_name_opt = parser.parse_type_name();
      REQUIRE(type_name_opt.has_value());

      if (type_name_opt.has_value()) {
        auto const result = ctx.resolve_type_name("Main", *type_name_opt);
        REQUIRE(result.has_value());
        if (result.has_value()) {
          CHECK(result->first == "Geometry");
        }
      }
    }
  }
}
