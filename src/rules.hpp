#ifndef RULES_HPP__
#define RULES_HPP__

#include <string>

#include "ast.hpp"  // IWYU pragma: keep

namespace life_lang::parser {
using IteratorType = std::string::const_iterator;
}  // namespace life_lang::parser

namespace life_lang::internal {
std::pair<bool, std::string> ParseIdentifier(parser::IteratorType &begin, parser::IteratorType end, std::ostream &out);
}

#endif