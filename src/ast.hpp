#ifndef AST_HPP__
#define AST_HPP__

#include <fmt/core.h>
#include <fmt/format.h>

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <concepts>
#include <string>
#include <utility>
#include <vector>

namespace life_lang::ast {

namespace internal {
struct VariantCmp {
  template <typename T>
  [[nodiscard]] bool operator()(
      boost::spirit::x3::forward_ast<T> const& lhs, boost::spirit::x3::forward_ast<T> const& rhs
  ) const {
    return lhs.get() == rhs.get();
  }
  template <typename T>
  [[nodiscard]] bool operator()(T const& lhs, T const& rhs) const {
    return lhs == rhs;
  }
  [[nodiscard]] bool operator()(auto const& /*lhs*/, auto const& /*rhs*/) const { return false; }
};
}  // namespace internal

struct PathSegment;
struct Path {
  std::vector<PathSegment> segments;
  friend bool operator==(Path const& lhs, Path const& rhs) = default;
  friend auto operator<<(std::ostream& os, Path const& path) -> std::ostream& { return os << fmt::to_string(path); }
};

struct PathSegment {
  std::string value;
  std::vector<Path> templateParameters;
  friend bool operator==(PathSegment const& lhs, PathSegment const& rhs) = default;
  friend auto operator<<(std::ostream& os, PathSegment const& segment) -> std::ostream& {
    return os << fmt::to_string(segment);
  }
};

template <typename... Args>
  requires((std::same_as<Args, PathSegment> || std::constructible_from<std::string, Args>) && ...)
inline Path MakePath(Args&&... args) {
  std::vector<PathSegment> segments;
  segments.reserve(sizeof...(args));
  (segments.emplace_back(std::forward<Args>(args)), ...);
  return Path{.segments = std::move(segments)};
}
inline PathSegment MakePathSegment(std::string&& value, std::vector<Path>&& templateParameters) {
  return PathSegment{.value = std::move(value), .templateParameters = std::move(templateParameters)};
}
inline PathSegment MakePathSegment(std::string&& value) { return MakePathSegment(std::move(value), {}); }

struct FunctionParameter {
  std::string name;
  Path type;
  friend bool operator==(FunctionParameter const& lhs, FunctionParameter const& rhs) = default;
  friend auto operator<<(std::ostream& os, FunctionParameter const& arg) -> std::ostream& {
    return os << fmt::to_string(arg);
  }
};

inline FunctionParameter MakeFunctionParameter(std::string&& name, Path&& type) {
  return FunctionParameter{.name = std::move(name), .type = std::move(type)};
}

struct FunctionDeclaration {
  std::string name;
  std::vector<FunctionParameter> parameters;
  Path returnType;
  friend bool operator==(FunctionDeclaration const& lhs, FunctionDeclaration const& rhs) = default;
  friend auto operator<<(std::ostream& os, FunctionDeclaration const& decl) -> std::ostream& {
    return os << fmt::to_string(decl);
  }
};

inline FunctionDeclaration MakeFunctionDeclaration(
    std::string&& name, std::vector<FunctionParameter>&& parameters, Path&& returnType
) {
  return FunctionDeclaration{
      .name = std::move(name), .parameters = std::move(parameters), .returnType = std::move(returnType)
  };
}

struct FunctionCallExpr;
using Expr = boost::spirit::x3::variant<Path, boost::spirit::x3::forward_ast<FunctionCallExpr>>;
inline bool operator==(Expr const& lhs, Expr const& rhs) {
  return boost::apply_visitor(internal::VariantCmp{}, lhs, rhs);
}
inline std::ostream& operator<<(std::ostream& os, Expr const& expr) { return os << fmt::to_string(expr); }

inline Expr MakeExpr(Path&& path) { return Expr{std::move(path)}; }

struct FunctionCallExpr {
  Path name;
  std::vector<Expr> parameters;
  friend bool operator==(FunctionCallExpr const& lhs, FunctionCallExpr const& rhs) = default;
  friend auto operator<<(std::ostream& os, FunctionCallExpr const& call) -> std::ostream& {
    return os << fmt::to_string(call);
  }
};
inline FunctionCallExpr MakeFunctionCallExpr(Path&& name, std::vector<Expr>&& parameters) {
  return FunctionCallExpr{.name = std::move(name), .parameters = std::move(parameters)};
}
inline Expr MakeExpr(FunctionCallExpr&& call) { return Expr{std::move(call)}; }

struct FunctionCallStatement {
  FunctionCallExpr expr;
  friend bool operator==(FunctionCallStatement const& lhs, FunctionCallStatement const& rhs) = default;
  friend auto operator<<(std::ostream& os, FunctionCallStatement const& call) -> std::ostream& {
    return os << fmt::to_string(call);
  }
};

inline FunctionCallStatement MakeFunctionCallStatement(FunctionCallExpr&& expr) {
  return FunctionCallStatement{.expr = std::move(expr)};
}

struct ReturnStatement {
  Expr expr;
  friend bool operator==(ReturnStatement const& lhs, ReturnStatement const& rhs) = default;
  friend auto operator<<(std::ostream& os, ReturnStatement const& returnStatement) -> std::ostream& {
    return os << fmt::to_string(returnStatement);
  }
};

inline ReturnStatement MakeReturnStatement(Expr&& expr) { return ReturnStatement{.expr = std::move(expr)}; }

struct FunctionDefinition;
struct Block;
using Statement = boost::spirit::x3::variant<
    boost::spirit::x3::forward_ast<FunctionDefinition>, FunctionCallStatement, ReturnStatement,
    boost::spirit::x3::forward_ast<Block>>;
inline bool operator==(Statement const& lhs, Statement const& rhs) {
  return boost::apply_visitor(internal::VariantCmp{}, lhs, rhs);
}
inline std::ostream& operator<<(std::ostream& os, Statement const& statement) {
  return os << fmt::to_string(statement);
}

inline Statement MakeStatement(FunctionCallStatement&& call) { return Statement{std::move(call)}; }
inline Statement MakeStatement(ReturnStatement&& ret) { return Statement{std::move(ret)}; }

struct Block {
  std::vector<Statement> statements;
  friend bool operator==(Block const& lhs, Block const& rhs) = default;
  friend auto operator<<(std::ostream& os, Block const& block) -> std::ostream& { return os << fmt::to_string(block); }
};

inline Block MakeBlock(std::vector<Statement>&& statements) { return Block{.statements = std::move(statements)}; }
inline Statement MakeStatement(Block&& block) { return Statement{std::move(block)}; }

struct FunctionDefinition {
  FunctionDeclaration declaration;
  Block body;
  friend bool operator==(FunctionDefinition const& lhs, FunctionDefinition const& rhs) {
    if (lhs.declaration == rhs.declaration) {
      return lhs.body == rhs.body;
    }
    return false;
  }
  friend auto operator<<(std::ostream& os, FunctionDefinition const& def) -> std::ostream& {
    return os << fmt::to_string(def);
  }
};

inline FunctionDefinition MakeFunctionDefinition(FunctionDeclaration&& decl, Block&& body) {
  return FunctionDefinition{.declaration = std::move(decl), .body = std::move(body)};
}
inline Statement MakeStatement(FunctionDefinition&& def) { return Statement{std::move(def)}; }
}  // namespace life_lang::ast

BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::PathSegment, value, templateParameters)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::Path, segments)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionParameter, name, type)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionDeclaration, name, parameters, returnType)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::ReturnStatement, expr)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionCallExpr, name, parameters)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionCallStatement, expr)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::Block, statements)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionDefinition, declaration, body)

namespace fmt {

template <>
struct formatter<life_lang::ast::PathSegment> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::PathSegment const& segment, FormatContext& ctx) const {
    if (segment.templateParameters.size() > 0) {
      return format_to(ctx.out(), "{}<{}>", segment.value, fmt::join(segment.templateParameters, ", "));
    }
    return format_to(ctx.out(), "{}", segment.value);
  }
};

template <>
struct formatter<life_lang::ast::Path> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::Path const& path, FormatContext& ctx) const {
    return format_to(ctx.out(), "{}", fmt::join(path.segments, "."));
  }
};

template <>
struct formatter<life_lang::ast::FunctionParameter> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::FunctionParameter const& arg, FormatContext& ctx) const {
    return format_to(ctx.out(), "{}: {}", arg.name, arg.type);
  }
};

template <>
struct formatter<life_lang::ast::FunctionDeclaration> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::FunctionDeclaration const& decl, FormatContext& ctx) const {
    return format_to(ctx.out(), "fn {}({}): {}", decl.name, fmt::join(decl.parameters, ", "), decl.returnType);
  }
};

template <>
struct formatter<life_lang::ast::FunctionCallExpr> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::FunctionCallExpr const& call, FormatContext& ctx) const {
    return format_to(ctx.out(), "{}({})", call.name, fmt::join(call.parameters, ", "));
  }
};

template <>
struct formatter<life_lang::ast::FunctionCallStatement> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::FunctionCallStatement const& call, FormatContext& ctx) const {
    return format_to(ctx.out(), "{};", call.expr);
  }
};

template <>
struct formatter<life_lang::ast::ReturnStatement> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::ReturnStatement const& returnStatement, FormatContext& ctx) const {
    return format_to(ctx.out(), "return {};", returnStatement.expr);
  }
};

template <>
struct formatter<life_lang::ast::Expr> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  struct Formatter {
    explicit Formatter(FormatContext* ctx) : m_ctx(ctx) {}
    [[nodiscard]] auto operator()(life_lang::ast::Path const& path) const {
      return format_to(m_ctx->out(), "{}", path);
    }
    template <typename T>
    [[nodiscard]] auto operator()(boost::spirit::x3::forward_ast<T> const& t) const {
      return format_to(m_ctx->out(), "{}", t.get());
    }
    [[nodiscard]] auto operator()(auto const& /*t*/) const { return format_to(m_ctx->out(), "Unknown expr"); }

   private:
    FormatContext* m_ctx;
  };
  template <typename FormatContext>
  auto format(life_lang::ast::Expr const& expr, FormatContext& ctx) const {
    return boost::apply_visitor(Formatter{&ctx}, expr);
  }
};

template <>
struct formatter<life_lang::ast::Block> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::Block const& block, FormatContext& ctx) const {
    return format_to(ctx.out(), "{}", fmt::join(block.statements, "\n"));
  }
};

template <>
struct formatter<life_lang::ast::FunctionDefinition> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::FunctionDefinition const& def, FormatContext& ctx) const {
    return format_to(ctx.out(), "{} {{\n{}\n}}", def.declaration, def.body);
  }
};

template <>
struct formatter<life_lang::ast::Statement> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  struct Formatter {
    explicit Formatter(FormatContext* ctx) : m_ctx(ctx) {}
    [[nodiscard]] auto operator()(life_lang::ast::FunctionCallStatement const& call) const {
      return format_to(m_ctx->out(), "{}", call);
    }
    [[nodiscard]] auto operator()(life_lang::ast::ReturnStatement const& ret) const {
      return format_to(m_ctx->out(), "{}", ret);
    }
    template <typename T>
    [[nodiscard]] auto operator()(boost::spirit::x3::forward_ast<T> const& t) const {
      return format_to(m_ctx->out(), "{}", t.get());
    }
    [[nodiscard]] auto operator()(auto const& /*t*/) const { return format_to(m_ctx->out(), "Unknown statement"); }

   private:
    FormatContext* m_ctx;
  };

  template <typename FormatContext>
  auto format(life_lang::ast::Statement const& statement, FormatContext& ctx) const {
    return boost::apply_visitor(Formatter{&ctx}, statement);
  }
};
}  // namespace fmt
#endif