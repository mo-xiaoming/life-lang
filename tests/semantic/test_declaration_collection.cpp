// Test semantic analyzer's declaration collection functionality

#include "semantic_analyzer.hpp"

#include "diagnostics.hpp"
#include "rules.hpp"

#include <catch2/catch_test_macros.hpp>

using life_lang::Diagnostic_Engine;
using life_lang::Semantic_Analyzer;
using life_lang::Symbol_Kind;
using life_lang::parser::parse_module;

namespace {

auto analyze_module(std::string const& a_source) -> Semantic_Analyzer {
  Diagnostic_Engine diagnostics{"<test>", a_source};

  auto const parse_result = parse_module(a_source);
  REQUIRE(parse_result.has_value());

  Semantic_Analyzer analyzer{diagnostics};
  [[maybe_unused]] auto const success = analyzer.analyze(*parse_result);

  return analyzer;
}

}  // namespace

TEST_CASE("Collect function declarations", "[semantic][declaration]") {
  char const* const source = R"(
    fn add(x: I32, y: I32): I32 {
      return x + y;
    }
    
    fn main(): () {
      let result = add(1, 2);
    }
  )";

  auto analyzer = analyze_module(source);
  auto const& symbols = analyzer.symbol_table();

  SECTION("Functions are registered") {
    auto const* const add_sym = symbols.lookup("add");
    REQUIRE(add_sym != nullptr);
    CHECK(add_sym->kind == Symbol_Kind::Function);
    CHECK(add_sym->name == "add");

    auto const* const main_sym = symbols.lookup("main");
    REQUIRE(main_sym != nullptr);
    CHECK(main_sym->kind == Symbol_Kind::Function);
    CHECK(main_sym->name == "main");
  }
}

TEST_CASE("Collect struct declarations", "[semantic][declaration]") {
  char const* const source = R"(
    struct Point {
      x: I32,
      y: I32
    }
    
    struct User {
      name: String,
      age: I32
    }
  )";

  auto analyzer = analyze_module(source);
  auto const& symbols = analyzer.symbol_table();

  SECTION("Structs are registered") {
    auto const* const point_sym = symbols.lookup_type("Point");
    REQUIRE(point_sym != nullptr);
    CHECK(point_sym->kind == Symbol_Kind::Type);
    CHECK(point_sym->name == "Point");

    auto const* const user_sym = symbols.lookup_type("User");
    REQUIRE(user_sym != nullptr);
    CHECK(user_sym->kind == Symbol_Kind::Type);
    CHECK(user_sym->name == "User");
  }
}

TEST_CASE("Collect enum declarations", "[semantic][declaration]") {
  char const* const source = R"(
    enum Option<T> {
      Some(T),
      None
    }
    
    enum Result<T, E> {
      Ok(T),
      Err(E)
    }
  )";

  auto analyzer = analyze_module(source);
  auto const& symbols = analyzer.symbol_table();

  SECTION("Enums are registered") {
    auto const* const option_sym = symbols.lookup_type("Option");
    REQUIRE(option_sym != nullptr);
    CHECK(option_sym->kind == Symbol_Kind::Type);
    CHECK(option_sym->name == "Option");
    CHECK(option_sym->generic_params.size() == 1);
    CHECK(option_sym->generic_params[0] == "T");

    auto const* const result_sym = symbols.lookup_type("Result");
    REQUIRE(result_sym != nullptr);
    CHECK(result_sym->kind == Symbol_Kind::Type);
    CHECK(result_sym->name == "Result");
    CHECK(result_sym->generic_params.size() == 2);
    CHECK(result_sym->generic_params[0] == "T");
    CHECK(result_sym->generic_params[1] == "E");
  }
}

TEST_CASE("Collect trait declarations", "[semantic][declaration]") {
  char const* const source = R"(
    trait Display {
      fn fmt(self): String;
    }
    
    trait Iterator<T> {
      fn next(mut self): Option<T>;
    }
  )";

  auto analyzer = analyze_module(source);
  auto const& symbols = analyzer.symbol_table();

  SECTION("Traits are registered") {
    auto const* const display_sym = symbols.lookup_type("Display");
    REQUIRE(display_sym != nullptr);
    CHECK(display_sym->kind == Symbol_Kind::Trait);
    CHECK(display_sym->name == "Display");

    auto const* const iterator_sym = symbols.lookup_type("Iterator");
    REQUIRE(iterator_sym != nullptr);
    CHECK(iterator_sym->kind == Symbol_Kind::Trait);
    CHECK(iterator_sym->name == "Iterator");
    CHECK(iterator_sym->generic_params.size() == 1);
    CHECK(iterator_sym->generic_params[0] == "T");
  }
}

TEST_CASE("Collect type alias declarations", "[semantic][declaration]") {
  char const* const source = R"(
    type String_Pair = (String, String);
    type Int_Vec = Vec<I32>;
  )";

  auto analyzer = analyze_module(source);
  auto const& symbols = analyzer.symbol_table();

  SECTION("Type aliases are registered") {
    auto const* const pair_sym = symbols.lookup_type("String_Pair");
    REQUIRE(pair_sym != nullptr);
    CHECK(pair_sym->kind == Symbol_Kind::Type);
    CHECK(pair_sym->name == "String_Pair");

    auto const* const vec_sym = symbols.lookup_type("Int_Vec");
    REQUIRE(vec_sym != nullptr);
    CHECK(vec_sym->kind == Symbol_Kind::Type);
    CHECK(vec_sym->name == "Int_Vec");
  }
}

TEST_CASE("Detect duplicate function names", "[semantic][declaration][error]") {
  char const* const source = R"(
    fn foo(): I32 { return 1; }
    fn foo(): String { return "dup"; }
  )";

  Diagnostic_Engine diagnostics{"<test>", source};

  auto const parse_result = parse_module(source);
  REQUIRE(parse_result.has_value());

  Semantic_Analyzer analyzer{diagnostics};
  auto const success = analyzer.analyze(*parse_result);

  CHECK_FALSE(success);
}

TEST_CASE("Detect duplicate type names", "[semantic][declaration][error]") {
  char const* const source = R"(
    struct Point { x: I32, y: I32 }
    enum Point { X, Y }
  )";

  Diagnostic_Engine diagnostics{"<test>", source};

  auto const parse_result = parse_module(source);
  REQUIRE(parse_result.has_value());

  Semantic_Analyzer analyzer{diagnostics};
  auto const success = analyzer.analyze(*parse_result);

  CHECK_FALSE(success);
}

TEST_CASE("Validate function naming convention", "[semantic][declaration][convention]") {
  char const* const source = R"(
    fn InvalidName(): () { }
  )";

  Diagnostic_Engine diagnostics{"<test>", source};

  auto const parse_result = parse_module(source);
  REQUIRE(parse_result.has_value());

  Semantic_Analyzer analyzer{diagnostics};
  auto const success = analyzer.analyze(*parse_result);

  CHECK_FALSE(success);
}

TEST_CASE("Validate type naming convention", "[semantic][declaration][convention]") {
  char const* const source = R"(
    struct invalid_name { value: I32 }
  )";

  Diagnostic_Engine diagnostics{"<test>", source};

  auto const parse_result = parse_module(source);
  REQUIRE(parse_result.has_value());

  Semantic_Analyzer analyzer{diagnostics};
  auto const success = analyzer.analyze(*parse_result);

  CHECK_FALSE(success);
}
