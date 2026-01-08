#include "internal_rules.hpp"
#include "utils.hpp"

using namespace life_lang::parser;
using namespace test_sexp;

namespace {

constexpr auto k_pub_fields_input = R"(
struct Point {
  pub x: I32,
  y: I32,
  pub z: I32
}
)";
inline auto const k_pub_fields_expected = struct_def(
    "Point",
    {
        struct_field("x", type_name("I32"), true),
        struct_field("y", type_name("I32"), false),
        struct_field("z", type_name("I32"), true),
    }
);

constexpr auto k_all_pub_fields_input = R"(
struct User {
  pub name: String,
  pub age: I32
}
)";
inline auto const k_all_pub_fields_expected = struct_def(
    "User",
    {
        struct_field("name", type_name("String"), true),
        struct_field("age", type_name("I32"), true),
    }
);

constexpr auto k_no_pub_fields_input = R"(
struct Internal {
  data: I32,
  flag: Bool
}
)";
inline auto const k_no_pub_fields_expected = struct_def(
    "Internal",
    {
        struct_field("data", type_name("I32"), false),
        struct_field("flag", type_name("Bool"), false),
    }
);

}  // namespace

TEST_CASE("parse_struct_def - pub fields") {
  struct Test_Case {
    std::string_view name;
    std::string_view input;
    std::string expected;
  };

  std::vector<Test_Case> const test_cases = {
      {.name = "pub fields", .input = k_pub_fields_input, .expected = k_pub_fields_expected},
      {.name = "all pub fields", .input = k_all_pub_fields_input, .expected = k_all_pub_fields_expected},
      {.name = "no pub fields", .input = k_no_pub_fields_input, .expected = k_no_pub_fields_expected},
  };

  for (auto const& tc: test_cases) {
    SUBCASE(std::string(tc.name).c_str()) {
      life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{tc.input});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

      life_lang::parser::Parser parser{diagnostics};
      auto const result = parser.parse_struct_def();
      REQUIRE(result.has_value());
      if (result.has_value()) {
        CHECK(to_sexp_string(*result, 0) == tc.expected);
      }
    }
  }
}
