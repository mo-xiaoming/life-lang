#include <fmt/core.h>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <iostream>
#include <string>

#include "ast.hpp"
#include "config.hpp"
#include "rules.hpp"

int main() {
  // using client::parser::PositionCacheTag;
  namespace x3 = boost::spirit::x3;

  std::string const input = R"(
        { 35, "John", "Doe", 35000.0 },
        { 25, "Jane", "Doe", 25000.0 }
    )";

  // client::parser::PositionCacheType positions(input.cbegin(), input.cend());

  client::parser::ErrorHandlerType errorHandler(input.cbegin(), input.cend(), std::cerr);
  auto const parser = with<x3::error_handler_tag>(std::ref(errorHandler))[client::GetGrammar()];

  std::vector<client::ast::Employee> ast;
  if (phrase_parse(input.cbegin(), input.cend(), parser, client::parser::SpaceType{}, ast)) {
    fmt::print("parsing succeeded\n");
    for (auto const &emp : ast) {
      fmt::print("got: {}\n", emp);
    }
  } else {
    fmt::print("parsing failed\n");
  }

  // auto pos = positions.position_of(ast[1]);
  // fmt::print("2nd employee: {}\n", std::string(pos.begin(), pos.end()));
}