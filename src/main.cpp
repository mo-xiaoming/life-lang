#include <fmt/core.h>

#include <iostream>
#include <string>

#include "lib.hpp"

int main() {
  std::string const input = R"(
        { 35, "John", "Doe", 35000.0 },
        { 25, "Jane", "Doe", 25000.0 }
    )";

  auto inputStart = input.cbegin();
  auto const &[success, ast] = client::parse(inputStart, input.cend(), std::cerr);
  if (success) {
    fmt::print("parsing succeeded\n");
    for (auto const &emp : ast) {
      fmt::print("got: {}\n", emp);
    }
  } else {
    fmt::print("parsing failed\n");
  }
}