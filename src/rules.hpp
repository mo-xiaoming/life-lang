#ifndef RULES_HPP__
#define RULES_HPP__

#include "ast.hpp"

namespace client::parser {
using IteratorType = std::string::const_iterator;
}  // namespace client::parser

namespace client {
std::pair<bool, std::vector<ast::Employee>> parse(parser::IteratorType begin, parser::IteratorType end,
                                                  std::ostream &out);
}  // namespace client
#endif