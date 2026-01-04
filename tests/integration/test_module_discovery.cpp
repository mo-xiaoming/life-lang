#include <doctest/doctest.h>

#include <fstream>

#include "semantic/module_loader.hpp"

using namespace life_lang::semantic;
namespace fs = std::filesystem;

TEST_SUITE("Module Discovery Integration") {
  // Test fixture for filesystem-based tests
  struct Module_Discovery_Fixture {
    fs::path temp_project;
    fs::path temp_src;

    Module_Discovery_Fixture() {
      temp_project = fs::temp_directory_path() / "life_lang_derive_test";
      temp_src = temp_project / "src";
      fs::remove_all(temp_project);
      fs::create_directories(temp_src);
    }

    Module_Discovery_Fixture(Module_Discovery_Fixture const&) = default;
    Module_Discovery_Fixture(Module_Discovery_Fixture&&) = delete;
    Module_Discovery_Fixture& operator=(Module_Discovery_Fixture const&) = default;
    Module_Discovery_Fixture& operator=(Module_Discovery_Fixture&&) = delete;

    ~Module_Discovery_Fixture() { fs::remove_all(temp_project); }
  };

  TEST_CASE("Derive module info from filesystem path") {
    Module_Discovery_Fixture const fixture;
    auto const& temp_src = fixture.temp_src;

    SUBCASE("Top-level module") {
      auto const geometry_dir = temp_src / "geometry";
      fs::create_directories(geometry_dir);

      auto const path_components = Module_Loader::derive_module_path(temp_src, geometry_dir);
      REQUIRE(path_components.size() == 1);
      CHECK(path_components[0] == "Geometry");
    }

    SUBCASE("Nested module") {
      auto const collections_dir = temp_src / "std" / "collections";
      fs::create_directories(collections_dir);

      auto const path_components = Module_Loader::derive_module_path(temp_src, collections_dir);
      REQUIRE(path_components.size() == 2);
      CHECK(path_components[0] == "Std");
      CHECK(path_components[1] == "Collections");
    }

    SUBCASE("Snake_case directory names") {
      auto const settings_dir = temp_src / "user_profile" / "settings";
      fs::create_directories(settings_dir);

      auto const path_components = Module_Loader::derive_module_path(temp_src, settings_dir);
      REQUIRE(path_components.size() == 2);
      CHECK(path_components[0] == "User_Profile");
      CHECK(path_components[1] == "Settings");
    }

    SUBCASE("Deep nesting") {
      auto const deep_dir = temp_src / "a" / "b" / "c" / "d";
      fs::create_directories(deep_dir);

      auto const path_components = Module_Loader::derive_module_path(temp_src, deep_dir);
      REQUIRE(path_components.size() == 4);
      CHECK(path_components[0] == "A");
      CHECK(path_components[1] == "B");
      CHECK(path_components[2] == "C");
      CHECK(path_components[3] == "D");
    }

    SUBCASE("Relative paths are canonicalized") {
      auto const geometry_dir = temp_src / "geometry";
      fs::create_directories(geometry_dir);

      // Create a directory we can use for '..' navigation
      auto const subdir = temp_src / "subdir";
      fs::create_directories(subdir);

      // Use relative path with '..' components - both must exist for canonical()
      auto const relative_src = subdir / "..";  // Points to temp_src
      auto const path_components = Module_Loader::derive_module_path(relative_src, geometry_dir);
      REQUIRE(path_components.size() == 1);
      CHECK(path_components[0] == "Geometry");
    }

    SUBCASE("Symlinks under src/ are rejected") {
      // Create actual geometry directory
      auto const geometry_dir = temp_src / "geometry";
      fs::create_directories(geometry_dir);

      // Create symlink under src/ pointing to geometry
      auto const symlink_dir = temp_src / "geo_link";
      fs::create_directory_symlink(geometry_dir, symlink_dir);

      // Symlinks are rejected to prevent confusion (link name != target name)
      auto const path_components = Module_Loader::derive_module_path(temp_src, symlink_dir);
      CHECK(path_components.empty());  // Rejected: would cause module name mismatch
    }

    SUBCASE("src/ itself can be a symlink") {
      // Create actual src directory elsewhere
      auto const real_src = fixture.temp_project / "real_src";
      fs::create_directories(real_src);

      auto const geometry_dir = real_src / "geometry";
      fs::create_directories(geometry_dir);

      // Create symlink for src/ (common use case: shared code, build outputs)
      auto const src_link = fixture.temp_project / "src_link";
      fs::create_directory_symlink(real_src, src_link);

      // This is allowed - src/ symlink is fine, but contents must be real dirs
      auto const path_components = Module_Loader::derive_module_path(src_link, geometry_dir);
      REQUIRE(path_components.size() == 1);
      CHECK(path_components[0] == "Geometry");
    }
  }

  TEST_CASE("Filesystem discovery with src/ convention") {
    Module_Discovery_Fixture const fixture;
    auto const& temp_src = fixture.temp_src;

    SUBCASE("Single module at src/ root") {
      auto const geometry_dir = temp_src / "geometry";
      fs::create_directories(geometry_dir);
      std::ofstream((geometry_dir / "point.life").string()).close();

      auto const modules = Module_Loader::discover_modules(temp_src);
      REQUIRE(modules.size() == 1);
      CHECK(modules[0].name() == "Geometry");
      CHECK(modules[0].path == std::vector<std::string>{"Geometry"});
      CHECK(modules[0].files.size() == 1);
    }

    SUBCASE("Module with multiple files") {
      auto const geometry_dir = temp_src / "geometry";
      fs::create_directories(geometry_dir);
      std::ofstream((geometry_dir / "point.life").string()).close();
      std::ofstream((geometry_dir / "circle.life").string()).close();
      std::ofstream((geometry_dir / "line.life").string()).close();

      auto const modules = Module_Loader::discover_modules(temp_src);
      REQUIRE(modules.size() == 1);
      CHECK(modules[0].name() == "Geometry");
      CHECK(modules[0].path == std::vector<std::string>{"Geometry"});
      CHECK(modules[0].files.size() == 3);
    }

    SUBCASE("Nested modules") {
      auto const std_math_dir = temp_src / "std" / "math";
      fs::create_directories(std_math_dir);
      std::ofstream((std_math_dir / "trig.life").string()).close();
      std::ofstream((std_math_dir / "algebra.life").string()).close();

      auto const modules = Module_Loader::discover_modules(temp_src);
      REQUIRE(modules.size() == 1);
      CHECK(modules[0].name() == "Math");
      CHECK(modules[0].path == std::vector<std::string>{"Std", "Math"});
      CHECK(modules[0].files.size() == 2);
    }

    SUBCASE("Snake_case directory names") {
      auto const profile_dir = temp_src / "user_profile";
      fs::create_directories(profile_dir);
      std::ofstream((profile_dir / "account.life").string()).close();

      auto const modules = Module_Loader::discover_modules(temp_src);
      REQUIRE(modules.size() == 1);
      CHECK(modules[0].name() == "User_Profile");
      CHECK(modules[0].path == std::vector<std::string>{"User_Profile"});
    }

    SUBCASE("Multiple modules at different levels") {
      // src/geometry/
      auto const geometry_dir = temp_src / "geometry";
      fs::create_directories(geometry_dir);
      std::ofstream((geometry_dir / "point.life").string()).close();

      // src/std/collections/
      auto const collections_dir = temp_src / "std" / "collections";
      fs::create_directories(collections_dir);
      std::ofstream((collections_dir / "vec.life").string()).close();

      // src/utils/
      auto const utils_dir = temp_src / "utils";
      fs::create_directories(utils_dir);
      std::ofstream((utils_dir / "helpers.life").string()).close();

      auto const modules = Module_Loader::discover_modules(temp_src);
      REQUIRE(modules.size() == 3);

      // Check all expected modules exist
      bool found_geometry = false;
      bool found_std_collections = false;
      bool found_utils = false;

      for (auto const& mod: modules) {
        if (mod.name() == "Geometry" && mod.path == std::vector<std::string>{"Geometry"}) {
          found_geometry = true;
        }
        if (mod.name() == "Collections" && mod.path == std::vector<std::string>{"Std", "Collections"}) {
          found_std_collections = true;
        }
        if (mod.name() == "Utils" && mod.path == std::vector<std::string>{"Utils"}) {
          found_utils = true;
        }
      }

      CHECK(found_geometry);
      CHECK(found_std_collections);
      CHECK(found_utils);
    }

    SUBCASE("Empty src/ directory") {
      auto const modules = Module_Loader::discover_modules(temp_src);
      CHECK(modules.empty());
    }

    SUBCASE("Non-existent directory") {
      auto const modules = Module_Loader::discover_modules(fs::path("/nonexistent/path"));
      CHECK(modules.empty());
    }
  }
}
