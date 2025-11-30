#include <fmt/core.h>

#include <sstream>
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

  std::string const input = R"(
fn main(args: Std.Array<Std.String>): I32 {
    Std.print("Hello, world!");
    return 0;
}
)";

  auto input_start = input.cbegin();
  std::ostringstream error_msg;
  auto const& got = life_lang::parser::parse(input_start, input.cend(), error_msg);
  if (got) {
    fmt::print("{}\n", to_json_string(*got, 4));
  } else {
    fmt::print("parsing failed:\n{}\n", error_msg.str());
  }
}