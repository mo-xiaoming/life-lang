#ifndef RULES_HPP__
#define RULES_HPP__

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>

#include "ast.hpp"

namespace client::parser {
namespace x3 = boost::spirit::x3;
namespace ascii = x3::ascii;

using ascii::char_;
using x3::double_;
using x3::int_;
using x3::lexeme;

// struct AnnotatePosition {
//   template <typename T, typename Iterator, typename Context>
//   inline void on_success(Iterator const &first, Iterator const &last, T &ast, Context const &context) {
//     auto &positionCache = x3::get<PositionCacheTag>(context);
//     positionCache.annotate(ast, first, last);
//   }
// };

struct ErrorHandler {
  template <typename Iterator, typename Exception, typename Context>
  x3::error_handler_result on_error(Iterator & /*first*/, Iterator const & /*last*/, Exception const &x,
                                    Context const &context) {
    auto &errorHandler = x3::get<x3::error_handler_tag>(context);
    std::string const message = fmt::format("Error! Expecting: {} here:", x.which());
    errorHandler(x.where(), message);
    return x3::error_handler_result::fail;
  }
};

// all rules needs annotation have to be delcare/defined/instantiated with BOOST_SPIRIT_DECLARE/DEFINE/INSTANTIATE
struct PersonRuleTag : x3::annotate_on_success {};
struct EmployeeRuleTag : ErrorHandler, x3::annotate_on_success {};
struct EmployeesRuleTag {};

using PersonRuleType = x3::rule<PersonRuleTag, ast::Person>;
BOOST_SPIRIT_DECLARE(PersonRuleType);

using EmployeeRuleType = x3::rule<EmployeeRuleTag, ast::Employee>;
BOOST_SPIRIT_DECLARE(EmployeeRuleType);

using EmployeesRuleType = x3::rule<EmployeesRuleTag, std::vector<ast::Employee>>;
BOOST_SPIRIT_DECLARE(EmployeesRuleType);
}  // namespace client::parser

namespace client {
parser::EmployeesRuleType GetGrammar();
}  // namespace client
#endif