#include <fmt/core.h>

#include <iostream>
#include <string>
#include <string_view>

#include "rules.hpp"
#include "version.hpp"

int main(int argc, char* argv[]) {
  // Handle command line arguments
  if (argc > 1) {
    std::string_view const arg{argv[1]};
    if (arg == "--version" || arg == "-v") {
      fmt::print("life-lang compiler version {}\n", life_lang::k_version);
      return 0;
    }
    if (arg == "--help" || arg == "-h") {
      fmt::print("Usage: {} [OPTIONS]\n", argv[0]);
      fmt::print("Options:\n");
      fmt::print("  -v, --version    Show version information\n");
      fmt::print("  -h, --help       Show this help message\n");
      return 0;
    }
  }

  // Example valid program
  std::string const valid_input = R"(fn main(args: Std.Array<Std.String>): I32 {
    Std.print("Hello, world!");
    return 0;
})";

  fmt::print("=== Parsing valid program ===\n");
  auto const valid_result = life_lang::parser::parse_module(valid_input, "hello.life");
  if (valid_result) {
    fmt::print("Success! AST:\n{}\n\n", to_json_string(*valid_result, 2));
  } else {
    fmt::print("Parse failed (which it shouldn't):\n");
    valid_result.error().print(std::cout);
  }

  // Example invalid program to demonstrate diagnostic output
  std::string const invalid_input = R"(fn main(): I32 {
    return 0;
}

fn broken_syntax_here
})";

  fmt::print("=== Parsing invalid program (syntax error) ===\n");
  auto const invalid_result = life_lang::parser::parse_module(invalid_input, "error.life");
  if (invalid_result) {
    fmt::print("Success (which it shouldn't)! AST:\n{}\n", to_json_string(*invalid_result, 2));
  } else {
    fmt::print("\n");
    invalid_result.error().print(std::cout);
  }
}