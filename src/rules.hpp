#ifndef RULES_HPP__
#define RULES_HPP__

#include <iosfwd>
#include <string>

#include "ast.hpp"

namespace life_lang::parser {
using IteratorType = std::string::const_iterator;
}  // namespace life_lang::parser

namespace life_lang::internal {
std::pair<bool, Identifier> ParseIdentifier(parser::IteratorType &begin, parser::IteratorType end, std::ostream &out);
std::pair<bool, Path> ParsePath(parser::IteratorType &begin, parser::IteratorType end, std::ostream &out);
std::pair<bool, Type> ParseType(parser::IteratorType &begin, parser::IteratorType end, std::ostream &out);
}  // namespace life_lang::internal

#endif