#include <doctest/doctest.h>

#include "semantic/module_loader.hpp"

using namespace life_lang::semantic;

TEST_SUITE("Module Loader") {
  TEST_CASE("Case conversion: dir_name_to_module_name") {
    SUBCASE("Single lowercase word") {
      CHECK(Module_Loader::dir_name_to_module_name("geometry") == "Geometry");
      CHECK(Module_Loader::dir_name_to_module_name("std") == "Std");
      CHECK(Module_Loader::dir_name_to_module_name("utils") == "Utils");
    }

    SUBCASE("Snake_case to Camel_Snake_Case") {
      CHECK(Module_Loader::dir_name_to_module_name("user_profile") == "User_Profile");
      CHECK(Module_Loader::dir_name_to_module_name("http_client") == "Http_Client");
      CHECK(Module_Loader::dir_name_to_module_name("my_module_name") == "My_Module_Name");
    }

    SUBCASE("Already capitalized (should preserve)") {
      CHECK(Module_Loader::dir_name_to_module_name("Geometry") == "Geometry");
      CHECK(Module_Loader::dir_name_to_module_name("User_Profile") == "User_Profile");
    }

    SUBCASE("Empty string") {
      CHECK(Module_Loader::dir_name_to_module_name("") == "");
    }

    SUBCASE("Single character") {
      CHECK(Module_Loader::dir_name_to_module_name("a") == "A");
      CHECK(Module_Loader::dir_name_to_module_name("x") == "X");
    }
  }
}
