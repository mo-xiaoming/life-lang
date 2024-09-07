#include <fmt/core.h>

#include <sstream>
#include <string>

#include "rules.hpp"

int main() {
  std::string const input = R"(
fn main(args: Std.Array<Std.String>): I32 {
    Std.print(args);
    return args.size();
}
)";

  auto inputStart = input.cbegin();
  std::ostringstream errorMsg;
  auto const &got = life_lang::parser::parse(inputStart, input.cend(), errorMsg);
  if (got) {
    fmt::print("{}\n", *got);
  } else {
    fmt::print("parsing failed: {}\n", errorMsg.str());
  }
}