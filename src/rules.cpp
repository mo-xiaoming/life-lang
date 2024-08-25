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
  x3::error_handler_result on_error(
      Iterator & /*first*/, Iterator const & /*last*/, Exception const &x, Context const &context
  ) {
    auto &errorHandler = x3::get<x3::error_handler_tag>(context);
    std::string const message = fmt::format("Error! Expecting: {} here:", x.which());
    errorHandler(x.where(), message);
    return x3::error_handler_result::fail;
  }
};

using x3::eps;
using x3::lexeme;
using x3::ascii::alnum;
using x3::ascii::alpha;
using x3::ascii::char_;
using x3::ascii::lit;

auto &GetSymbolTable() {
  static x3::symbols<int> symtab;
  return symtab;
}

auto const mkkw = [](std::string const &kw) {
  GetSymbolTable().add(kw);
  return lexeme[x3::lit(kw) >> !alnum];
};

auto const KwFn = mkkw("fn");
auto const KwLet = mkkw("let");
auto const ReservedRule = lexeme[GetSymbolTable() >> !(alnum | char_('_'))];

struct IdentifierTag : ErrorHandler, x3::annotate_on_success {};
x3::rule<IdentifierTag, ast::Identifier> const IdentifierRule = "identifier rule";
auto const IdentifierRule_def =
    x3::raw[x3::lexeme[(alpha | char_('_')) >> *(alnum | char_('_'))] - ReservedRule]
           [([](auto &ctx) { x3::_val(ctx).value = std::string{x3::_attr(ctx).begin(), x3::_attr(ctx).end()}; })];
BOOST_SPIRIT_DEFINE(IdentifierRule)
BOOST_SPIRIT_INSTANTIATE(decltype(IdentifierRule), IteratorType, ContextType)

struct PathTag : ErrorHandler, x3::annotate_on_success {};
x3::rule<PathTag, ast::Path> const PathRule = "path rule";
auto const PathRule_def = eps[([](auto &ctx) { x3::_val(ctx).isAbsolute = false; })] >>
                          -lit("::")[([](auto &ctx) { x3::_val(ctx).isAbsolute = true; })] >>
                          (IdentifierRule % "::")[([](auto &ctx) { x3::_val(ctx).segments = x3::_attr(ctx); })];
BOOST_SPIRIT_DEFINE(PathRule)
BOOST_SPIRIT_INSTANTIATE(decltype(PathRule), IteratorType, ContextType)

struct TypeTag : ErrorHandler, x3::annotate_on_success {};
x3::rule<TypeTag, ast::Type> const TypeRule = "type rule";

struct TemplateArgumentTag : ErrorHandler, x3::annotate_on_success {};
x3::rule<TemplateArgumentTag, ast::TemplateArgument> const TemplateArgumentRule = "template argument rule";

struct TemplateArgumentListTag : ErrorHandler, x3::annotate_on_success {};
x3::rule<TemplateArgumentListTag, ast::TemplateArgumentList> const TemplateArgumentListRule =
    "template argument list rule";

auto const TemplateArgumentRule_def = TypeRule;
BOOST_SPIRIT_DEFINE(TemplateArgumentRule)
BOOST_SPIRIT_INSTANTIATE(decltype(TemplateArgumentRule), IteratorType, ContextType)

auto const TemplateArgumentListRule_def = lit('<') > (TemplateArgumentRule % ',') > lit('>');
BOOST_SPIRIT_DEFINE(TemplateArgumentListRule)
BOOST_SPIRIT_INSTANTIATE(decltype(TemplateArgumentListRule), IteratorType, ContextType)

auto const TypeRule_def = PathRule >> -TemplateArgumentListRule;
BOOST_SPIRIT_DEFINE(TypeRule)
BOOST_SPIRIT_INSTANTIATE(decltype(TypeRule), IteratorType, ContextType)

struct ArgumentTag : ErrorHandler, x3::annotate_on_success {};
x3::rule<ArgumentTag, ast::Argument> const ArgumentRule = "argument rule";
auto const ArgumentRule_def = IdentifierRule > lit(':') > TypeRule;
BOOST_SPIRIT_DEFINE(ArgumentRule)
BOOST_SPIRIT_INSTANTIATE(decltype(ArgumentRule), IteratorType, ContextType)

struct ArgumentListTag : ErrorHandler, x3::annotate_on_success {};
x3::rule<ArgumentListTag, ast::ArgumentList> const ArgumentListRule = "argument list rule";
auto const ArgumentListRule_def = lit('(') > -(ArgumentRule % ',') > lit(')');
BOOST_SPIRIT_DEFINE(ArgumentListRule)
BOOST_SPIRIT_INSTANTIATE(decltype(ArgumentListRule), IteratorType, ContextType)
}  // namespace life_lang::parser

namespace life_lang::internal {

template <typename Rule, typename AST>
std::pair<bool, AST> Parse(Rule const &rule, parser::IteratorType &begin, parser::IteratorType end, std::ostream &out) {
  static parser::ErrorHandlerType errorHandler(begin, end, out);
  auto const parser = with<parser::x3::error_handler_tag>(std::ref(errorHandler))[rule];
  AST ast;
  return {phrase_parse(begin, end, parser, parser::SpaceType{}, ast), ast};
}

#define PARSE_FN_DEFINITION(name)                                                                          \
  std::pair<bool, life_lang::ast::name> Parse##name(                                                       \
      parser::IteratorType &begin, parser::IteratorType end, std::ostream &out                             \
  ) {                                                                                                      \
    return Parse<decltype(parser::name##Rule), life_lang::ast::name>(parser::name##Rule, begin, end, out); \
  }

PARSE_FN_DEFINITION(Identifier)
PARSE_FN_DEFINITION(Path)
PARSE_FN_DEFINITION(Type)
PARSE_FN_DEFINITION(Argument)
PARSE_FN_DEFINITION(ArgumentList)
#undef PARSE_FN_DEFINITION
}  // namespace life_lang::internal
