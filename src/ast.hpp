#ifndef AST_HPP__
#define AST_HPP__

#include <fmt/core.h>
#include <fmt/format.h>

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <optional>
#include <string>
#include <vector>

namespace life_lang::ast {

struct ModulePathSegment {
  std::string value;
  friend bool operator==(ModulePathSegment const& lhs, ModulePathSegment const& rhs) = default;
  friend auto operator<<(std::ostream& os, ModulePathSegment const& segment) -> std::ostream& {
    return os << fmt::to_string(segment);
  }
};
struct ModulePath {
  bool isAbsolute{};
  std::vector<ModulePathSegment> segments;
  friend bool operator==(ModulePath const& lhs, ModulePath const& rhs) = default;
  friend auto operator<<(std::ostream& os, ModulePath const& path) -> std::ostream& {
    return os << fmt::to_string(path);
  }
};

struct DataPathSegment {
  std::string value;
  friend bool operator==(DataPathSegment const& lhs, DataPathSegment const& rhs) = default;
  friend auto operator<<(std::ostream& os, DataPathSegment const& segment) -> std::ostream& {
    return os << fmt::to_string(segment);
  }
};

struct DataPath {
  std::vector<DataPathSegment> segments;
  friend bool operator==(DataPath const& lhs, DataPath const& rhs) = default;
  friend auto operator<<(std::ostream& os, DataPath const& path) -> std::ostream& { return os << fmt::to_string(path); }
};

struct Type {
  ModulePath modulePath;
  std::string name;
  std::vector<Type> templateParameters;
  friend bool operator==(Type const& lhs, Type const& rhs) = default;
  friend auto operator<<(std::ostream& os, Type const& type) -> std::ostream& { return os << fmt::to_string(type); }
};

struct FunctionParameter {
  std::string name;
  Type type;
  friend bool operator==(FunctionParameter const& lhs, FunctionParameter const& rhs) = default;
  friend auto operator<<(std::ostream& os, FunctionParameter const& arg) -> std::ostream& {
    return os << fmt::to_string(arg);
  }
};

struct FunctionDeclaration {
  std::string name;
  std::vector<FunctionParameter> parameters;
  Type returnType;
  friend bool operator==(FunctionDeclaration const& lhs, FunctionDeclaration const& rhs) = default;
  friend auto operator<<(std::ostream& os, FunctionDeclaration const& decl) -> std::ostream& {
    return os << fmt::to_string(decl);
  }
};

struct Value {
  std::optional<Type> type;
  DataPath dataPath;
  friend bool operator==(Value const& lhs, Value const& rhs) = default;
  friend auto operator<<(std::ostream& os, Value const& value) -> std::ostream& { return os << fmt::to_string(value); }
};

struct FunctionCallExpr;

using Expr = boost::spirit::x3::variant<Value, boost::spirit::x3::forward_ast<FunctionCallExpr>>;
namespace internal {
struct ExprCmp {
  [[nodiscard]] bool operator()(Value const& lhs, Value const& rhs) const {
    if (lhs.type != rhs.type) {
      return false;
    }
    return lhs.dataPath == rhs.dataPath;
  }
  template <typename T>
  [[nodiscard]] bool operator()(
      boost::spirit::x3::forward_ast<T> const& lhs, boost::spirit::x3::forward_ast<T> const& rhs
  ) const {
    return lhs.get() == rhs.get();
  }
  [[nodiscard]] bool operator()(auto const& /*lhs*/, auto const& /*rhs*/) const { return false; }
};
}  // namespace internal
inline bool operator==(Expr const& lhs, Expr const& rhs) { return boost::apply_visitor(internal::ExprCmp{}, lhs, rhs); }
inline std::ostream& operator<<(std::ostream& os, Expr const& expr) { return os << fmt::to_string(expr); }

struct FunctionCallExpr {
  Value name;
  std::vector<Expr> parameters;
  friend bool operator==(FunctionCallExpr const& lhs, FunctionCallExpr const& rhs) = default;
  friend auto operator<<(std::ostream& os, FunctionCallExpr const& call) -> std::ostream& {
    return os << fmt::to_string(call);
  }
};

struct FunctionCallExprStatement {
  FunctionCallExpr expr;
  friend bool operator==(FunctionCallExprStatement const& lhs, FunctionCallExprStatement const& rhs) = default;
  friend auto operator<<(std::ostream& os, FunctionCallExprStatement const& call) -> std::ostream& {
    return os << fmt::to_string(call);
  }
};

struct ReturnStatement {
  Expr expr;
  friend bool operator==(ReturnStatement const& lhs, ReturnStatement const& rhs) = default;
  friend auto operator<<(std::ostream& os, ReturnStatement const& returnStatement) -> std::ostream& {
    return os << fmt::to_string(returnStatement);
  }
};

struct FunctionDefinition;
struct Block;
using Statement = boost::spirit::x3::variant<
    boost::spirit::x3::forward_ast<FunctionDefinition>, FunctionCallExprStatement, ReturnStatement,
    boost::spirit::x3::forward_ast<Block>>;
namespace internal {
struct StatementCmp {
  template <typename T>
  [[nodiscard]] bool operator()(
      boost::spirit::x3::forward_ast<T> const& lhs, boost::spirit::x3::forward_ast<T> const& rhs
  ) const {
    return lhs.get() == rhs.get();
  }
  [[nodiscard]] bool operator()(FunctionCallExprStatement const& lhs, FunctionCallExprStatement const& rhs) const {
    return lhs == rhs;
  }
  [[nodiscard]] bool operator()(ReturnStatement const& lhs, ReturnStatement const& rhs) const { return lhs == rhs; }
  [[nodiscard]] bool operator()(auto const& /*lhs*/, auto const& /*rhs*/) const { return false; }
};
}  // namespace internal
inline bool operator==(Statement const& lhs, Statement const& rhs) {
  return boost::apply_visitor(internal::StatementCmp{}, lhs, rhs);
}
inline std::ostream& operator<<(std::ostream& os, Statement const& statement) {
  return os << fmt::to_string(statement);
}

struct Block {
  std::vector<Statement> statements;
  friend bool operator==(Block const& lhs, Block const& rhs) = default;
  friend auto operator<<(std::ostream& os, Block const& block) -> std::ostream& { return os << fmt::to_string(block); }
};

struct FunctionDefinition {
  FunctionDeclaration declaration;
  Block body;
  friend bool operator==(FunctionDefinition const& lhs, FunctionDefinition const& rhs) = default;
  friend auto operator<<(std::ostream& os, FunctionDefinition const& def) -> std::ostream& {
    return os << fmt::to_string(def);
  }
};
}  // namespace life_lang::ast

BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::ModulePathSegment, value)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::ModulePath, isAbsolute, segments)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::DataPathSegment, value)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::DataPath, segments)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::Type, modulePath, name, templateParameters)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionParameter, name, type)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionDeclaration, name, parameters, returnType)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::Value, type, dataPath)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::ReturnStatement, expr)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionCallExpr, name, parameters)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionCallExprStatement, expr)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::Block, statements)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionDefinition, declaration, body)

namespace fmt {
template <>
struct formatter<life_lang::ast::ModulePathSegment> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::ModulePathSegment const& segment, FormatContext& ctx) const {
    return format_to(ctx.out(), "{}", segment.value);
  }
};

template <>
struct formatter<life_lang::ast::ModulePath> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::ModulePath const& path, FormatContext& ctx) const {
    return format_to(ctx.out(), "{}{}", path.isAbsolute ? "" : ".", fmt::join(path.segments, "."));
  }
};

template <>
struct formatter<life_lang::ast::DataPathSegment> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::DataPathSegment const& segment, FormatContext& ctx) const {
    return format_to(ctx.out(), "{}", segment.value);
  }
};

template <>
struct formatter<life_lang::ast::DataPath> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::DataPath const& path, FormatContext& ctx) const {
    return format_to(ctx.out(), "{}", fmt::join(path.segments, "."));
  }
};

template <>
struct formatter<life_lang::ast::Type> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::Type const& type, FormatContext& ctx) const {
    if (type.templateParameters.empty()) {
      if (type.modulePath.segments.empty()) {
        return format_to(ctx.out(), ".{}", type.name);
      }
      return format_to(ctx.out(), "{}.{}", type.modulePath, type.name);
    }
    if (type.modulePath.segments.empty()) {
      return format_to(ctx.out(), ".{}<{}>", type.name, fmt::join(type.templateParameters, ", "));
    }
    return format_to(ctx.out(), "{}.{}<{}>", type.modulePath, type.name, fmt::join(type.templateParameters, ", "));
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
struct formatter<life_lang::ast::Value> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::Value const& value, FormatContext& ctx) const {
    if (value.type.has_value()) {
      return format_to(ctx.out(), "{}.{}", *value.type, value.dataPath);
    }
    return format_to(ctx.out(), "{}", value.dataPath);
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
struct formatter<life_lang::ast::FunctionCallExprStatement> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::FunctionCallExprStatement const& call, FormatContext& ctx) const {
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
    [[nodiscard]] auto operator()(life_lang::ast::Value const& value) const {
      return format_to(m_ctx->out(), "{}", value);
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
    [[nodiscard]] auto operator()(life_lang::ast::FunctionCallExprStatement const& call) const {
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