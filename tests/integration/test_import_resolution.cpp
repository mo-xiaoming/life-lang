#include <doctest/doctest.h>

#include "diagnostics.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_context.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>

using life_lang::Diagnostic_Engine;
using life_lang::Diagnostic_Manager;
using life_lang::File_Id;
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

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse a Point type reference in Main module context
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"Point"});
    Diagnostic_Engine diag{registry, file_id};
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

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Resolve using the alias "C"
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"C"});
    Diagnostic_Engine diag{registry, file_id};
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

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Resolve Vec
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"Vec"});
    Diagnostic_Engine diag{registry, file_id};
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

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse fully qualified type: Geometry.Point
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"Geometry.Point"});
    Diagnostic_Engine diag{registry, file_id};
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

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Resolve Point - should find local definition, not imported one
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"Point"});
    Diagnostic_Engine diag{registry, file_id};
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

  TEST_CASE("function import and resolution") {
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

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse function call: add
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"add"});
    Diagnostic_Engine diag{registry, file_id};
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

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Try to resolve Internal
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"Internal"});
    Diagnostic_Engine diag{registry, file_id};
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

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Try to resolve Point
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"Point"});
    Diagnostic_Engine diag{registry, file_id};
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

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // All three should resolve
    for (auto const* name: {"Point", "Circle", "Line"}) {
      life_lang::Source_File_Registry registry;
      File_Id const file_id = registry.register_file("<test>", std::string{name});
      Diagnostic_Engine diag{registry, file_id};
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

  TEST_CASE("Error reporting - importing non-pub type") {
    Temp_Module_Fixture const fixture;

    // Create Geometry module with non-pub struct
    fs::create_directories(fixture.temp_src / "geometry");
    fixture.create_file("geometry/types.life", "struct Internal { x: I32 }\n");

    // Create Main module that imports Internal
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/app.life",
        "import Geometry.{ Internal };\n"
        "fn test(): Internal { return Internal { x: 0 }; }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));
    diag_mgr.clear_diagnostics();  // Clear any errors from loading

    // Try to resolve Internal - should fail with error
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"Internal"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      CHECK(!result.has_value());

      // Verify error was reported
      REQUIRE(diag_mgr.has_errors());
      auto const& errors = diag_mgr.all_diagnostics();
      REQUIRE(!errors.empty());
      CHECK(errors[0].message.find("cannot import 'Internal'") != std::string::npos);
      CHECK(errors[0].message.find("not marked pub") != std::string::npos);
    }
  }

  TEST_CASE("Error reporting - type not found") {
    Temp_Module_Fixture const fixture;

    // Create Main module
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/app.life", "fn test(): I32 { return 0; }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));
    diag_mgr.clear_diagnostics();

    // Try to resolve non-existent type
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"NonExistent"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      CHECK(!result.has_value());

      // Verify error was reported
      REQUIRE(diag_mgr.has_errors());
      auto const& errors = diag_mgr.all_diagnostics();
      REQUIRE(!errors.empty());
      CHECK(errors[0].message.find("type 'NonExistent'") != std::string::npos);
      CHECK(errors[0].message.find("not found") != std::string::npos);
    }
  }

  TEST_CASE("Error reporting - accessing non-pub type cross-module") {
    Temp_Module_Fixture const fixture;

    // Create Geometry module with non-pub type
    fs::create_directories(fixture.temp_src / "geometry");
    fixture.create_file("geometry/types.life", "struct Internal { x: I32 }\n");

    // Create Main module (no import, direct access)
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/app.life", "fn test(): I32 { return 0; }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));
    diag_mgr.clear_diagnostics();

    // Try to resolve Geometry.Internal (fully qualified)
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"Geometry.Internal"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      CHECK(!result.has_value());

      // Verify error was reported
      REQUIRE(diag_mgr.has_errors());
      auto const& errors = diag_mgr.all_diagnostics();
      REQUIRE(!errors.empty());
      CHECK(errors[0].message.find("cannot access") != std::string::npos);
      CHECK(errors[0].message.find("Internal") != std::string::npos);
      CHECK(errors[0].message.find("not marked pub") != std::string::npos);
    }
  }

  TEST_CASE("Error reporting - importing non-pub function") {
    Temp_Module_Fixture const fixture;

    // Create Math module with non-pub function
    fs::create_directories(fixture.temp_src / "math");
    fixture.create_file("math/ops.life", "fn internal_calc(x: I32): I32 { return x * 2; }\n");

    // Create Main module that imports internal_calc
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/app.life",
        "import Math.{ internal_calc };\n"
        "fn test(): I32 { return internal_calc(5); }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));
    diag_mgr.clear_diagnostics();

    // Try to resolve internal_calc
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"internal_calc"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const var_name_opt = parser.parse_variable_name();
    REQUIRE(var_name_opt.has_value());

    if (var_name_opt.has_value()) {
      auto const result = ctx.resolve_var_name("Main", *var_name_opt);
      CHECK(!result.has_value());

      // Verify error was reported
      REQUIRE(diag_mgr.has_errors());
      auto const& errors = diag_mgr.all_diagnostics();
      REQUIRE(!errors.empty());
      CHECK(errors[0].message.find("cannot import function 'internal_calc'") != std::string::npos);
      CHECK(errors[0].message.find("not marked pub") != std::string::npos);
    }
  }

  TEST_CASE("Error reporting - function not found") {
    Temp_Module_Fixture const fixture;

    // Create Main module
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/app.life", "fn test(): I32 { return 0; }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));
    diag_mgr.clear_diagnostics();

    // Try to resolve non-existent function
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"missing_func"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const var_name_opt = parser.parse_variable_name();
    REQUIRE(var_name_opt.has_value());

    if (var_name_opt.has_value()) {
      auto const result = ctx.resolve_var_name("Main", *var_name_opt);
      CHECK(!result.has_value());

      // Verify error was reported
      REQUIRE(diag_mgr.has_errors());
      auto const& errors = diag_mgr.all_diagnostics();
      REQUIRE(!errors.empty());
      CHECK(errors[0].message.find("function 'missing_func'") != std::string::npos);
      CHECK(errors[0].message.find("not found") != std::string::npos);
    }
  }
}

// ============================================================================
// Compound Type Resolution Tests (Issue #3)
// Tests for Function_Type, Array_Type, and Tuple_Type resolution
// ============================================================================

TEST_SUITE("Compound Type Resolution") {
  TEST_CASE("Array type - resolves element type") {
    Temp_Module_Fixture const fixture;

    // Create module with a struct
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/types.life", "pub struct Point { x: I32, y: I32 }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse an array type with the struct as element type: [Point; 5]
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"[Point; 5]"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      // Resolve should return nullopt (array types are structural, not definitions)
      // but should NOT produce an error since Point is valid
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      CHECK(!result.has_value());     // Array types return nullopt
      CHECK(!diag_mgr.has_errors());  // But no error - it's valid
    }
  }

  TEST_CASE("Array type - reports error for unknown element type") {
    Temp_Module_Fixture const fixture;

    // Create module with NO definitions
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/types.life", "// empty module\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse an array type with unknown element type: [UnknownType; 5]
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"[UnknownType; 5]"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      CHECK(!result.has_value());

      // Verify error was reported for the unknown element type
      REQUIRE(diag_mgr.has_errors());
      auto const& errors = diag_mgr.all_diagnostics();
      REQUIRE(!errors.empty());
      CHECK(errors[0].message.find("UnknownType") != std::string::npos);
      CHECK(errors[0].message.find("not found") != std::string::npos);
    }
  }

  TEST_CASE("Tuple type - resolves all element types") {
    Temp_Module_Fixture const fixture;

    // Create module with structs
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/types.life", "pub struct Point { x: I32, y: I32 }\npub struct Color { r: I32 }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse a tuple type: (Point, Color)
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"(Point, Color)"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      // Tuple types return nullopt but should not produce errors
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      CHECK(!result.has_value());     // Tuple types return nullopt
      CHECK(!diag_mgr.has_errors());  // No error - both types are valid
    }
  }

  TEST_CASE("Tuple type - reports error for unknown element type") {
    Temp_Module_Fixture const fixture;

    // Create module with one struct
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/types.life", "pub struct Point { x: I32, y: I32 }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse a tuple type with one unknown type: (Point, UnknownType)
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"(Point, UnknownType)"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      CHECK(!result.has_value());

      // Verify error was reported for the unknown type
      REQUIRE(diag_mgr.has_errors());
      auto const& errors = diag_mgr.all_diagnostics();
      REQUIRE(!errors.empty());
      CHECK(errors[0].message.find("UnknownType") != std::string::npos);
      CHECK(errors[0].message.find("not found") != std::string::npos);
    }
  }

  TEST_CASE("Function type - resolves param and return types") {
    Temp_Module_Fixture const fixture;

    // Create module with structs
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/types.life", "pub struct Input { x: I32 }\npub struct Output { y: I32 }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse a function type: fn(Input): Output
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"fn(Input): Output"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      // Function types return nullopt but should not produce errors
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      CHECK(!result.has_value());     // Function types return nullopt
      CHECK(!diag_mgr.has_errors());  // No error - all types are valid
    }
  }

  TEST_CASE("Function type - reports error for unknown param type") {
    Temp_Module_Fixture const fixture;

    // Create module with one struct
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/types.life", "pub struct Output { y: I32 }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse a function type with unknown param type: fn(UnknownInput): Output
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"fn(UnknownInput): Output"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      CHECK(!result.has_value());

      // Verify error was reported for the unknown param type
      REQUIRE(diag_mgr.has_errors());
      auto const& errors = diag_mgr.all_diagnostics();
      REQUIRE(!errors.empty());
      CHECK(errors[0].message.find("UnknownInput") != std::string::npos);
      CHECK(errors[0].message.find("not found") != std::string::npos);
    }
  }

  TEST_CASE("Function type - reports error for unknown return type") {
    Temp_Module_Fixture const fixture;

    // Create module with one struct
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/types.life", "pub struct Input { x: I32 }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse a function type with unknown return type: fn(Input): UnknownOutput
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"fn(Input): UnknownOutput"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      CHECK(!result.has_value());

      // Verify error was reported for the unknown return type
      REQUIRE(diag_mgr.has_errors());
      auto const& errors = diag_mgr.all_diagnostics();
      REQUIRE(!errors.empty());
      CHECK(errors[0].message.find("UnknownOutput") != std::string::npos);
      CHECK(errors[0].message.find("not found") != std::string::npos);
    }
  }

  TEST_CASE("Nested compound types - array of tuples") {
    Temp_Module_Fixture const fixture;

    // Create module with a struct
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/types.life", "pub struct Point { x: I32, y: I32 }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse a nested type: [(Point, Point); 3]
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"[(Point, Point); 3]"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      CHECK(!result.has_value());     // Compound types return nullopt
      CHECK(!diag_mgr.has_errors());  // No error - Point is valid
    }
  }

  TEST_CASE("Generic type parameters - are recursively resolved") {
    Temp_Module_Fixture const fixture;

    // Create module with structs
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/types.life", "pub struct Container<T> { value: T }\npub struct Point { x: I32 }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse a generic type: Container<Point>
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"Container<Point>"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      REQUIRE(result.has_value());
      if (result.has_value()) {
        auto const& [module_path, item] = *result;
        CHECK(module_path == "Main");
        CHECK(item != nullptr);
        CHECK(!diag_mgr.has_errors());  // Both Container and Point are valid
      }
    }
  }

  TEST_CASE("Generic type with unknown param - reports error") {
    Temp_Module_Fixture const fixture;

    // Create module with generic struct
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/types.life", "pub struct Container<T> { value: T }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse a generic type with unknown param: Container<UnknownType>
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"Container<UnknownType>"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const type_name_opt = parser.parse_type_name();
    REQUIRE(type_name_opt.has_value());

    if (type_name_opt.has_value()) {
      auto const result = ctx.resolve_type_name("Main", *type_name_opt);
      // Container resolves but we should still get an error for UnknownType
      // The result may still return the Container since the container itself exists
      REQUIRE(diag_mgr.has_errors());
      auto const& errors = diag_mgr.all_diagnostics();
      REQUIRE(!errors.empty());
      CHECK(errors[0].message.find("UnknownType") != std::string::npos);
      CHECK(errors[0].message.find("not found") != std::string::npos);
    }
  }
}

// ============================================================================
// Generic Var Name Resolution Tests (Issue #4)
// Tests for resolve_var_name with type parameters
// ============================================================================

TEST_SUITE("Generic Var Name Resolution") {
  TEST_CASE("Generic function call - resolves type params") {
    Temp_Module_Fixture const fixture;

    // Create module with a generic function and types
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/types.life",
        "pub struct Point { x: I32, y: I32 }\n"
        "pub fn create<T>(): T { return T{}; }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse a generic function call: create<Point>
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"create<Point>"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const var_name_opt = parser.parse_qualified_variable_name();
    REQUIRE(var_name_opt.has_value());

    if (var_name_opt.has_value()) {
      auto const result = ctx.resolve_var_name("Main", *var_name_opt);
      REQUIRE(result.has_value());
      if (result.has_value()) {
        auto const& [module_path, item] = *result;
        CHECK(module_path == "Main");
        CHECK(item != nullptr);
        CHECK(!diag_mgr.has_errors());  // Both create and Point are valid
      }
    }
  }

  TEST_CASE("Generic function with unknown type param - reports error") {
    Temp_Module_Fixture const fixture;

    // Create module with a generic function
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file("main/types.life", "pub fn create<T>(): T { return T{}; }\n");

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse a generic function call with unknown type: create<UnknownType>
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"create<UnknownType>"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const var_name_opt = parser.parse_qualified_variable_name();
    REQUIRE(var_name_opt.has_value());

    if (var_name_opt.has_value()) {
      auto const result = ctx.resolve_var_name("Main", *var_name_opt);
      // Function resolves but we get an error for unknown type param
      REQUIRE(diag_mgr.has_errors());
      auto const& errors = diag_mgr.all_diagnostics();
      REQUIRE(!errors.empty());
      CHECK(errors[0].message.find("UnknownType") != std::string::npos);
      CHECK(errors[0].message.find("not found") != std::string::npos);
    }
  }

  TEST_CASE("Imported generic function - resolves type params") {
    Temp_Module_Fixture const fixture;

    // Create Utils module with generic function
    fs::create_directories(fixture.temp_src / "utils");
    fixture.create_file("utils/funcs.life", "pub fn identity<T>(x: T): T { return x; }\n");

    // Create Main module with import and type
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/app.life",
        "import Utils.{ identity };\n"
        "pub struct Data { value: I32 }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse imported generic function call: identity<Data>
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"identity<Data>"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const var_name_opt = parser.parse_qualified_variable_name();
    REQUIRE(var_name_opt.has_value());

    if (var_name_opt.has_value()) {
      auto const result = ctx.resolve_var_name("Main", *var_name_opt);
      REQUIRE(result.has_value());
      if (result.has_value()) {
        auto const& [module_path, item] = *result;
        CHECK(module_path == "Utils");  // Function is from Utils
        CHECK(item != nullptr);
        CHECK(!diag_mgr.has_errors());  // Both identity and Data are valid
      }
    }
  }

  TEST_CASE("Multiple type params in function call - all validated") {
    Temp_Module_Fixture const fixture;

    // Create module with a multi-param generic function
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/types.life",
        "pub struct Key { k: I32 }\n"
        "pub struct Value { v: I32 }\n"
        "pub fn pair<K, V>(k: K, v: V): (K, V) { return (k, v); }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse: pair<Key, Value>
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"pair<Key, Value>"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const var_name_opt = parser.parse_qualified_variable_name();
    REQUIRE(var_name_opt.has_value());

    if (var_name_opt.has_value()) {
      auto const result = ctx.resolve_var_name("Main", *var_name_opt);
      REQUIRE(result.has_value());
      CHECK(!diag_mgr.has_errors());  // All types valid
    }
  }

  TEST_CASE("Multiple type params - one unknown reports error") {
    Temp_Module_Fixture const fixture;

    // Create module with a multi-param generic function and one type
    fs::create_directories(fixture.temp_src / "main");
    fixture.create_file(
        "main/types.life",
        "pub struct Key { k: I32 }\n"
        "pub fn pair<K, V>(k: K, v: V): (K, V) { return (k, v); }\n"
    );

    Diagnostic_Manager diag_mgr;
    Semantic_Context ctx(diag_mgr);
    REQUIRE(ctx.load_modules(fixture.temp_src));

    // Parse: pair<Key, UnknownValue> - second param is unknown
    life_lang::Source_File_Registry registry;
    File_Id const file_id = registry.register_file("<test>", std::string{"pair<Key, UnknownValue>"});
    Diagnostic_Engine diag{registry, file_id};
    life_lang::parser::Parser parser(diag);
    auto const var_name_opt = parser.parse_qualified_variable_name();
    REQUIRE(var_name_opt.has_value());

    if (var_name_opt.has_value()) {
      auto const result = ctx.resolve_var_name("Main", *var_name_opt);
      // Function resolves but error for unknown type
      REQUIRE(diag_mgr.has_errors());
      auto const& errors = diag_mgr.all_diagnostics();
      REQUIRE(!errors.empty());
      CHECK(errors[0].message.find("UnknownValue") != std::string::npos);
    }
  }
}
