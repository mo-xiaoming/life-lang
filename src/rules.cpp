#include "rules.hpp"

#include <fmt/core.h>

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>

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

using x3::lexeme;
using x3::raw;
using x3::ascii::alnum;
using x3::ascii::alpha;
using x3::ascii::char_;
using x3::ascii::digit;
using x3::ascii::lit;
using x3::ascii::lower;

auto &GetSymbolTable() {
  static x3::symbols<int> symtab;
  return symtab;
}

namespace {
auto const MakeKeyword = [](std::string const &kw) {
  GetSymbolTable().add(kw);
  return lexeme[x3::lit(kw) >> !alnum];
};

auto const KwFn = MakeKeyword("fn");
auto const KwLet = MakeKeyword("let");
auto const ReservedRule = lexeme[GetSymbolTable() >> !(alnum | char_('_'))];

// auto const CName = raw[lexeme[upper >> *(alnum)] >> !alnum];
auto const SnakeCase = raw[lexeme[lower >> *(lower | digit | char_('_')) >> !(alnum | char_('_'))]];
}  // namespace

struct PathTag : ErrorHandler, x3::position_tagged {};
x3::rule<PathTag, ast::Path> const PathRule = "path rule";

struct PathSegmentTag : ErrorHandler, x3::position_tagged {};
x3::rule<PathSegmentTag, ast::PathSegment> const PathSegmentRule = "path segment rule";
auto const PathSegmentRule_def = raw[lexeme[alpha >> *(alnum | char_('_'))]] >>
                                 -(lit('<') > (PathRule % ',') > lit('>'));
BOOST_SPIRIT_DEFINE(PathSegmentRule)
BOOST_SPIRIT_INSTANTIATE(decltype(PathSegmentRule), IteratorType, ContextType)

auto const PathRule_def = PathSegmentRule % lit('.');
BOOST_SPIRIT_DEFINE(PathRule)
BOOST_SPIRIT_INSTANTIATE(decltype(PathRule), IteratorType, ContextType)

auto const EscapedChar = lexeme[lit('\\') > (char_("\"\\ntr") | (char_('x') > x3::repeat(2)[x3::xdigit]))];
struct StringTag : ErrorHandler, x3::position_tagged {};
x3::rule<StringTag, ast::String> const StringRule = "string rule";
auto const StringRule_def = raw[lexeme[lit('"') > *(EscapedChar | (char_ - lit('"') - lit('\\'))) > lit('"')]];
BOOST_SPIRIT_DEFINE(StringRule)
BOOST_SPIRIT_INSTANTIATE(decltype(StringRule), IteratorType, ContextType)

struct IntegerTag : ErrorHandler, x3::position_tagged {};
x3::rule<IntegerTag, ast::Integer> const IntegerRule = "integer rule";
auto const IntegerRule_def =
    raw[lexeme[(lit('0') >> !(digit | char_('_'))) | (char_('1', '9') >> *(char_('0', '9') | char_('_')))]]
       [([](auto &ctx) {
         auto const &attr = x3::_attr(ctx);
         std::string str{attr.begin(), attr.end()};
         if (str.back() == '_') {
           x3::_pass(ctx) = false;
         } else {
           std::erase(str, '_');
           x3::_val(ctx) = ast::MakeInteger(str);
         }
       })];
BOOST_SPIRIT_DEFINE(IntegerRule)
BOOST_SPIRIT_INSTANTIATE(decltype(IntegerRule), IteratorType, ContextType)

struct FunctionParameterTag : ErrorHandler, x3::position_tagged {};
x3::rule<FunctionParameterTag, ast::FunctionParameter> const FunctionParameterRule = "function parameter rule";
auto const FunctionParameterRule_def = SnakeCase > lit(':') > PathRule;
BOOST_SPIRIT_DEFINE(FunctionParameterRule)
BOOST_SPIRIT_INSTANTIATE(decltype(FunctionParameterRule), IteratorType, ContextType)

struct FunctionDeclarationTag : ErrorHandler, x3::position_tagged {};
x3::rule<FunctionDeclarationTag, ast::FunctionDeclaration> const FunctionDeclarationRule = "function declaration rule";
auto const FunctionDeclarationRule_def = lit("fn") > SnakeCase > lit('(') > -(FunctionParameterRule % lit(',')) >
                                         lit(')') > lit(':') > PathRule;
BOOST_SPIRIT_DEFINE(FunctionDeclarationRule)
BOOST_SPIRIT_INSTANTIATE(decltype(FunctionDeclarationRule), IteratorType, ContextType)

struct ExprTag : ErrorHandler, x3::position_tagged {};
x3::rule<ExprTag, ast::Expr> const ExprRule = "expr rule";

struct FunctionCallExprTag : ErrorHandler, x3::position_tagged {};
x3::rule<FunctionCallExprTag, ast::FunctionCallExpr> const FunctionCallExprRule = "function call rule";
auto const FunctionCallExprRule_def = PathRule >> lit('(') >> -(ExprRule % ',') >> lit(')');
BOOST_SPIRIT_DEFINE(FunctionCallExprRule)
BOOST_SPIRIT_INSTANTIATE(decltype(FunctionCallExprRule), IteratorType, ContextType)

auto const ExprRule_def = FunctionCallExprRule | StringRule | PathRule | IntegerRule;
BOOST_SPIRIT_DEFINE(ExprRule)
BOOST_SPIRIT_INSTANTIATE(decltype(ExprRule), IteratorType, ContextType)

struct ReturnStatementTag : ErrorHandler, x3::position_tagged {};
x3::rule<ReturnStatementTag, ast::ReturnStatement> const ReturnStatementRule = "return statement rule";
auto const ReturnStatementRule_def = lit("return") > ExprRule > lit(';');
BOOST_SPIRIT_DEFINE(ReturnStatementRule)
BOOST_SPIRIT_INSTANTIATE(decltype(ReturnStatementRule), IteratorType, ContextType)

struct FunctionCallStatementTag : ErrorHandler, x3::position_tagged {};
x3::rule<FunctionCallStatementTag, ast::FunctionCallStatement> const FunctionCallStatementRule =
    "function call statement rule";
auto const FunctionCallStatementRule_def = FunctionCallExprRule > lit(';');
BOOST_SPIRIT_DEFINE(FunctionCallStatementRule)
BOOST_SPIRIT_INSTANTIATE(decltype(FunctionCallStatementRule), IteratorType, ContextType)

struct StatementTag : ErrorHandler, x3::position_tagged {};
x3::rule<StatementTag, ast::Statement> const StatementRule = "statement rule";

struct BlockTag : ErrorHandler, x3::position_tagged {};
x3::rule<BlockTag, ast::Block> const BlockRule = "block rule";
auto const BlockRule_def = lit('{') > *StatementRule > lit('}');
BOOST_SPIRIT_DEFINE(BlockRule)
BOOST_SPIRIT_INSTANTIATE(decltype(BlockRule), IteratorType, ContextType)

struct FunctionDefinitionTag : ErrorHandler, x3::position_tagged {};
x3::rule<FunctionDefinitionTag, ast::FunctionDefinition> const FunctionDefinitionRule = "function definition rule";
auto const FunctionDefinitionRule_def = FunctionDeclarationRule > BlockRule;
BOOST_SPIRIT_DEFINE(FunctionDefinitionRule)
BOOST_SPIRIT_INSTANTIATE(decltype(FunctionDefinitionRule), IteratorType, ContextType)

auto const StatementRule_def = FunctionDefinitionRule | FunctionCallStatementRule | BlockRule | ReturnStatementRule;
BOOST_SPIRIT_DEFINE(StatementRule)
BOOST_SPIRIT_INSTANTIATE(decltype(StatementRule), IteratorType, ContextType)
}  // namespace life_lang::parser

namespace life_lang::internal {

template <typename Rule, typename Ast>
ParseResult<Ast> Parse(Rule const &rule, parser::IteratorType &begin, parser::IteratorType end, std::ostream &out) {
  static parser::ErrorHandlerType errorHandler(begin, end, out);
  auto const parser = with<parser::x3::error_handler_tag>(std::ref(errorHandler))[rule];
  Ast ast;
  return ParseResult{phrase_parse(begin, end, parser, parser::SpaceType{}, ast), ast};
}

#define PARSE_FN_DEFINITION(name)                                                                          \
  ParseResult<life_lang::ast::name> Parse##name(                                                           \
      parser::IteratorType &begin, parser::IteratorType end, std::ostream &out                             \
  ) {                                                                                                      \
    return Parse<decltype(parser::name##Rule), life_lang::ast::name>(parser::name##Rule, begin, end, out); \
  }

PARSE_FN_DEFINITION(PathSegment)
PARSE_FN_DEFINITION(Path)
PARSE_FN_DEFINITION(String)
PARSE_FN_DEFINITION(Integer)
PARSE_FN_DEFINITION(FunctionParameter)
PARSE_FN_DEFINITION(FunctionDeclaration)
PARSE_FN_DEFINITION(Expr)
PARSE_FN_DEFINITION(FunctionCallExpr)
PARSE_FN_DEFINITION(FunctionCallStatement)
PARSE_FN_DEFINITION(ReturnStatement)
PARSE_FN_DEFINITION(Statement)
PARSE_FN_DEFINITION(Block)
PARSE_FN_DEFINITION(FunctionDefinition)
#undef PARSE_FN_DEFINITION
}  // namespace life_lang::internal

namespace life_lang::parser {
internal::ParseResult<ast::FunctionDefinition> parse(IteratorType &begin, IteratorType end, std::ostream &out) {
  return internal::ParseFunctionDefinition(begin, end, out);
}
}  // namespace life_lang::parser