#ifndef LIB_HPP__
#define LIB_HPP__

#include <fmt/core.h>

#include <boost/fusion/include/adapt_struct.hpp>
#include <string>
#include <utility>
#include <vector>

#include "spirit_x3.hpp"  // IWYU pragma: keep

namespace client::ast {
namespace x3 = boost::spirit::x3;

struct Person : x3::position_tagged {
  std::string first_name, last_name;
};

struct Employee : x3::position_tagged {
  int age{};
  Person who;
  double salary{};
};
}  // namespace client::ast

BOOST_FUSION_ADAPT_STRUCT(client::ast::Person, first_name, last_name)
BOOST_FUSION_ADAPT_STRUCT(client::ast::Employee, age, who, salary)

template <>
struct fmt::formatter<client::ast::Person> {
  static constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(client::ast::Person const &p, FormatContext &ctx) {
    return format_to(ctx.out(), "[{}, {}]", p.first_name, p.last_name);
  }
};

template <>
struct fmt::formatter<client::ast::Employee> {
  static constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(client::ast::Employee const &e, FormatContext &ctx) {
    return format_to(ctx.out(), "[{}, {}, {}]", e.age, e.who, e.salary);
  }
};
namespace client::parser {
using IteratorType = std::string::const_iterator;
}  // namespace client::parser

namespace client {
std::pair<bool, std::vector<ast::Employee>> parse(parser::IteratorType &begin, parser::IteratorType end,
                                                  std::ostream &out);
}  // namespace client

#endif
