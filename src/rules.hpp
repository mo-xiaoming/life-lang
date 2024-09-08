#ifndef RULES_HPP__
#define RULES_HPP__

#include <iosfwd>
#include <string>

#include "ast.hpp"

namespace life_lang::parser {
using IteratorType = std::string::const_iterator;
}  // namespace life_lang::parser

namespace life_lang::internal {
template <typename Ast>
struct ParseResult {
  ParseResult(bool success, Ast ast) : m_success(success), m_ast(std::move(ast)) {}
  [[nodiscard]] constexpr explicit operator bool() const noexcept { return m_success; }
  [[nodiscard]] Ast const &operator*() const noexcept { return m_ast; }

 private:
  bool m_success;
  Ast m_ast;
};

#define PARSE_FN_DECLARATION(name)                                             \
  ParseResult<life_lang::ast::name> Parse##name(                               \
      parser::IteratorType &begin, parser::IteratorType end, std::ostream &out \
  );

PARSE_FN_DECLARATION(PathSegment)
PARSE_FN_DECLARATION(Path)
PARSE_FN_DECLARATION(String)
PARSE_FN_DECLARATION(FunctionParameter)
PARSE_FN_DECLARATION(FunctionDeclaration)
PARSE_FN_DECLARATION(Expr)
PARSE_FN_DECLARATION(FunctionCallExpr)
PARSE_FN_DECLARATION(FunctionCallStatement)
PARSE_FN_DECLARATION(ReturnStatement)
PARSE_FN_DECLARATION(Statement)
PARSE_FN_DECLARATION(Block)
PARSE_FN_DECLARATION(FunctionDefinition)
#undef PARSE_FN_DECLARATION
}  // namespace life_lang::internal

namespace life_lang::parser {
internal::ParseResult<ast::FunctionDefinition> parse(IteratorType &begin, IteratorType end, std::ostream &out);
}

#endif
