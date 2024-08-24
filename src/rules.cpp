#include "rules.hpp"

#include <fmt/core.h>

#include "spirit_x3.hpp"  // IWYU pragma: keep

namespace life_lang::parser {
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

using x3::lexeme;
using x3::ascii::alnum;
using x3::ascii::alpha;
using x3::ascii::char_;

x3::symbols<int> symtab;

auto const mkkw = [](std::string const &kw) {
  symtab.add(kw);
  return lexeme[x3::lit(kw) >> !alnum];
};

auto const kw_fn = mkkw("fn");
auto const kw_let = mkkw("let");
auto const reserved = lexeme[symtab >> !(alnum | char_('_'))];

struct IdentifierTag : ErrorHandler, x3::annotate_on_success {};
x3::rule<IdentifierTag, std::string> const IdentifierRule = "identifier rule";
auto const IdentifierRule_def = x3::lexeme[*char_('_') >> -(alpha >> *(alnum | char_('_')))] - reserved;
BOOST_SPIRIT_DEFINE(IdentifierRule)
BOOST_SPIRIT_INSTANTIATE(decltype(IdentifierRule), IteratorType, ContextType)

struct PathSegmentTag : ErrorHandler, x3::annotate_on_success {};
x3::rule<PathSegmentTag, std::string> const PathSegmentRule = "namespace rule";
auto const PathSegmentRule_def = IdentifierRule;
BOOST_SPIRIT_DEFINE(PathSegmentRule)
BOOST_SPIRIT_INSTANTIATE(decltype(PathSegmentRule), IteratorType, ContextType)
}  // namespace life_lang::parser

namespace life_lang::internal {
std::pair<bool, std::string> ParseIdentifier(parser::IteratorType &begin, parser::IteratorType end, std::ostream &out) {
  static parser::ErrorHandlerType errorHandler(begin, end, out);
  auto const parser = with<parser::x3::error_handler_tag>(std::ref(errorHandler))[parser::IdentifierRule];
  std::string ast;
  return {phrase_parse(begin, end, parser, parser::SpaceType{}, ast), ast};
}
}  // namespace life_lang::internal