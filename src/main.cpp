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
      fmt::print("  -                Read source from stdin\n");
      return 0;
    }
    if (arg == "-") {
      std::string const input((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());

      auto const result = life_lang::parser::parse_module(input, "<stdin>");
      if (result) {
        fmt::print("{}\n", to_json_string(*result, 2));
        return 0;
      }
      result.error().print(std::cerr);
      return 1;
    }
  }
}