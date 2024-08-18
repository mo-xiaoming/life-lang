#include "rules.hpp"

#include <boost/spirit/home/x3.hpp>

#include "config.hpp"  // IWYU pragma: keep

namespace client::parser {

auto const QuotedStringRule = lexeme['"' >> +(char_ - '"') >> '"'];

PersonRuleType const PersonRule = "person rule";
auto const PersonRule_def = QuotedStringRule > ',' > QuotedStringRule;
BOOST_SPIRIT_DEFINE(PersonRule)
BOOST_SPIRIT_INSTANTIATE(PersonRuleType, IteratorType, ContextType)

EmployeeRuleType const EmployeeRule = "employee rule";
auto const EmployeeRule_def = '{' > int_ > ',' > PersonRule > ',' > double_ > '}';
BOOST_SPIRIT_DEFINE(EmployeeRule)
BOOST_SPIRIT_INSTANTIATE(EmployeeRuleType, IteratorType, ContextType)

EmployeesRuleType const EmployeesRule = "employees rule";
auto const EmployeesRule_def = EmployeeRule % ',';
BOOST_SPIRIT_DEFINE(EmployeesRule)
BOOST_SPIRIT_INSTANTIATE(EmployeesRuleType, IteratorType, ContextType)
}  // namespace client::parser

namespace client {
parser::EmployeesRuleType GetGrammar() { return parser::EmployeesRule; }
}  // namespace client