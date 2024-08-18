#include <fmt/core.h>

#include <boost/fusion/include/adapt_struct.hpp>
#include <iostream>
#include <string>

#include "rules.hpp"
#include "spirit_x3.hpp"  // IWYU pragma: keep

int main() {
  std::string const input = R"(
        { 35, "John", "Doe", 35000.0 },
        { 25, "Jane", "Doe", 25000.0 }
    )";

  auto const &[success, ast] = client::parse(input.cbegin(), input.cend(), std::cerr);
  if (success) {
    fmt::print("parsing succeeded\n");
    for (auto const &emp : ast) {
      fmt::print("got: {}\n", emp);
    }
  } else {
    fmt::print("parsing failed\n");
  }
}