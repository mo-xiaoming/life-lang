#include "lib.hpp"

namespace client::parser {
namespace x3 = boost::spirit::x3;

using SpaceType = x3::ascii::space_type;
using ErrorHandlerType = x3::error_handler<IteratorType>;
using PhraseContextType = x3::phrase_parse_context<SpaceType>::type;
using ContextType = x3::context<x3::error_handler_tag, std::reference_wrapper<ErrorHandlerType>, PhraseContextType>;

struct ErrorHandler {
  template <typename Iterator, typename Exception, typename Context>
  x3::error_handler_result on_error(Iterator & /*first*/, Iterator const & /*last*/, Exception const &x,
                                    Context const &context) {
    auto &errorHandler = x3::get<x3::error_handler_tag>(context);
    std::string const message = fmt::format("Error! Expecting: {} here:", x.which() /*.name()*/);
    errorHandler(x.where(), message);
    return x3::error_handler_result::fail;
  }
};

using x3::double_;
using x3::int_;
using x3::lexeme;
using x3::ascii::print;

// all rules needs annotation have to be delcare/defined/instantiated with BOOST_SPIRIT_DECLARE/DEFINE/INSTANTIATE
struct PersonRuleTag : x3::annotate_on_success {};
struct EmployeeRuleTag : ErrorHandler, x3::annotate_on_success {};
struct EmployeesRuleTag {};

using PersonRuleType = x3::rule<PersonRuleTag, ast::Person>;
using EmployeeRuleType = x3::rule<EmployeeRuleTag, ast::Employee>;
using EmployeesRuleType = x3::rule<EmployeesRuleTag, std::vector<ast::Employee>>;

// NOLINTNEXTLINE(bugprone-chained-comparison)
auto const QuotedStringRule = lexeme['"' >> +(print - '"') >> '"'];

PersonRuleType const PersonRule = "person rule";
// NOLINTNEXTLINE(bugprone-chained-comparison)
auto const PersonRule_def = QuotedStringRule > ',' > QuotedStringRule;
BOOST_SPIRIT_DEFINE(PersonRule)
BOOST_SPIRIT_INSTANTIATE(PersonRuleType, IteratorType, ContextType)

EmployeeRuleType const EmployeeRule = "employee rule";
// NOLINTNEXTLINE(bugprone-chained-comparison)
auto const EmployeeRule_def = '{' > int_ > ',' > PersonRule > ',' > double_ > '}';
BOOST_SPIRIT_DEFINE(EmployeeRule)
BOOST_SPIRIT_INSTANTIATE(EmployeeRuleType, IteratorType, ContextType)

EmployeesRuleType const EmployeesRule = "employees rule";
auto const EmployeesRule_def = EmployeeRule % ',';
BOOST_SPIRIT_DEFINE(EmployeesRule)
BOOST_SPIRIT_INSTANTIATE(EmployeesRuleType, IteratorType, ContextType)
}  // namespace client::parser

namespace client {
std::pair<bool, std::vector<ast::Employee>> parse(parser::IteratorType &begin, parser::IteratorType end,
                                                  std::ostream &out) {
  static parser::ErrorHandlerType errorHandler(begin, end, out);
  auto const parser = with<parser::x3::error_handler_tag>(std::ref(errorHandler))[parser::EmployeesRule];
  std::vector<ast::Employee> ast;
  return {phrase_parse(begin, end, parser, parser::SpaceType{}, ast), ast};
}
}  // namespace client