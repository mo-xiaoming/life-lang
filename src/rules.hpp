#ifndef RULES_HPP__
#define RULES_HPP__

#include <iosfwd>
#include <string>

#include "ast.hpp"

namespace life_lang::parser {
using IteratorType = std::string::const_iterator;
}  // namespace life_lang::parser

namespace life_lang::internal {
#define PARSE_FN_DECLARATION(name)                                             \
  std::pair<bool, life_lang::ast::name> Parse##name(                           \
      parser::IteratorType &begin, parser::IteratorType end, std::ostream &out \
  );

PARSE_FN_DECLARATION(Identifier)
PARSE_FN_DECLARATION(Path)
PARSE_FN_DECLARATION(Type)
PARSE_FN_DECLARATION(Argument)
PARSE_FN_DECLARATION(ArgumentList)
#undef PARSE_FN_DECLARATION
}  // namespace life_lang::internal

#endif