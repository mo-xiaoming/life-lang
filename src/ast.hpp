#ifndef AST_HPP__
#define AST_HPP__

#include <fmt/core.h>

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/type_index.hpp>
#include <concepts>
#include <nlohmann/json.hpp>
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
  static inline constexpr std::string_view Name = "Path";
  std::vector<PathSegment> segments;
};

struct PathSegment {
  static inline constexpr std::string_view Name = "PathSegment";
  std::string value;
  std::vector<Path> templateParameters;
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

struct String {
  static inline constexpr std::string_view Name = "String";
  std::string value;
};

inline String MakeString(std::string&& value) { return String{.value = std::move(value)}; }

struct Integer {
  static inline constexpr std::string_view Name = "Integer";
  std::string value;
};

inline Integer MakeInteger(std::string value) noexcept { return Integer{.value = std::move(value)}; }

struct FunctionParameter {
  static inline constexpr std::string_view Name = "FunctionParameter";
  std::string name;
  Path type;
};

inline FunctionParameter MakeFunctionParameter(std::string&& name, Path&& type) {
  return FunctionParameter{.name = std::move(name), .type = std::move(type)};
}

struct FunctionDeclaration {
  static inline constexpr std::string_view Name = "FunctionDeclaration";
  std::string name;
  std::vector<FunctionParameter> parameters;
  Path returnType;
};

inline FunctionDeclaration MakeFunctionDeclaration(
    std::string&& name, std::vector<FunctionParameter>&& parameters, Path&& returnType
) {
  return FunctionDeclaration{
      .name = std::move(name), .parameters = std::move(parameters), .returnType = std::move(returnType)
  };
}

struct FunctionCallExpr;
using Expr = boost::spirit::x3::variant<Path, boost::spirit::x3::forward_ast<FunctionCallExpr>, String, Integer>;

inline Expr MakeExpr(Path&& path) { return Expr{std::move(path)}; }
inline Expr MakeExpr(String&& str) { return Expr{std::move(str)}; }
inline Expr MakeExpr(Integer&& integer) noexcept { return Expr{std::move(integer)}; }

struct FunctionCallExpr {
  static inline constexpr std::string_view Name = "FunctionCallExpr";
  Path name;
  std::vector<Expr> parameters;
};
inline FunctionCallExpr MakeFunctionCallExpr(Path&& name, std::vector<Expr>&& parameters) {
  return FunctionCallExpr{.name = std::move(name), .parameters = std::move(parameters)};
}
inline Expr MakeExpr(FunctionCallExpr&& call) { return Expr{std::move(call)}; }

struct FunctionCallStatement {
  static inline constexpr std::string_view Name = "FunctionCallStatement";
  FunctionCallExpr expr;
};

inline FunctionCallStatement MakeFunctionCallStatement(FunctionCallExpr&& expr) {
  return FunctionCallStatement{.expr = std::move(expr)};
}

struct ReturnStatement {
  static inline constexpr std::string_view Name = "ReturnStatement";
  Expr expr;
};

inline ReturnStatement MakeReturnStatement(Expr&& expr) { return ReturnStatement{.expr = std::move(expr)}; }

struct FunctionDefinition;
struct Block;
using Statement = boost::spirit::x3::variant<
    boost::spirit::x3::forward_ast<FunctionDefinition>, FunctionCallStatement, ReturnStatement,
    boost::spirit::x3::forward_ast<Block>>;

inline Statement MakeStatement(FunctionCallStatement&& call) { return Statement{std::move(call)}; }
inline Statement MakeStatement(ReturnStatement&& ret) { return Statement{std::move(ret)}; }

struct Block {
  static inline constexpr std::string_view Name = "Block";
  std::vector<Statement> statements;
};

inline Block MakeBlock(std::vector<Statement>&& statements) { return Block{.statements = std::move(statements)}; }
inline Statement MakeStatement(Block&& block) { return Statement{std::move(block)}; }

struct FunctionDefinition {
  static inline constexpr std::string_view Name = "FunctionDefinition";
  FunctionDeclaration declaration;
  Block body;
};

inline FunctionDefinition MakeFunctionDefinition(FunctionDeclaration&& decl, Block&& body) {
  return FunctionDefinition{.declaration = std::move(decl), .body = std::move(body)};
}
inline Statement MakeStatement(FunctionDefinition&& def) { return Statement{std::move(def)}; }

void to_json(nlohmann::json& json, PathSegment const& segment);
inline void to_json(nlohmann::json& json, Path const& path) { json[Path::Name] = {{"segments", path.segments}}; }
inline void to_json(nlohmann::json& json, PathSegment const& segment) {
  json[PathSegment::Name] = {{"value", segment.value}, {"templateParameters", segment.templateParameters}};
};
inline void to_json(nlohmann::json& json, String const& str) { json[String::Name] = {{"value", str.value}}; }
inline void to_json(nlohmann::json& json, Integer const& integer) { json[Integer::Name] = {{"value", integer.value}}; }
inline void to_json(nlohmann::json& json, FunctionParameter const& param) {
  json[FunctionParameter::Name] = {{"name", param.name}, {"type", param.type}};
}
inline void to_json(nlohmann::json& json, FunctionDeclaration const& decl) {
  json[FunctionDeclaration::Name] = {
      {"name", decl.name}, {"parameters", decl.parameters}, {"returnType", decl.returnType}
  };
}
void to_json(nlohmann::json& json, FunctionCallExpr const& call);
inline void to_json(nlohmann::json& json, Expr const& expr) {
  boost::apply_visitor([&json](auto const& value) { json = value; }, expr);
}
inline void to_json(nlohmann::json& json, FunctionCallExpr const& call) {
  json[FunctionCallExpr::Name] = {{"name", call.name}, {"parameters", call.parameters}};
}
inline void to_json(nlohmann::json& json, FunctionCallStatement const& call) {
  json[FunctionCallStatement::Name] = {{"expr", call.expr}};
}
inline void to_json(nlohmann::json& json, ReturnStatement const& ret) {
  json[ReturnStatement::Name] = {{"expr", ret.expr}};
}
void to_json(nlohmann::json& json, FunctionDefinition const& def);
void to_json(nlohmann::json& json, Block const& block);
inline void to_json(nlohmann::json& json, Statement const& stmt) {
  boost::apply_visitor([&json](auto const& value) { json = value; }, stmt);
}
inline void to_json(nlohmann::json& json, Block const& block) {
  json[Block::Name] = {{"statements", block.statements}};
}
inline void to_json(nlohmann::json& json, FunctionDefinition const& def) {
  json[FunctionDefinition::Name] = {{"declaration", def.declaration}, {"body", def.body}};
}

template <typename T>
concept JsonStringConvertible = requires(nlohmann::json j, T t) {
  { life_lang::ast::to_json(j, t) };
};

std::string ToJsonString(JsonStringConvertible auto const& t, int indent) {
  nlohmann::json json;
  life_lang::ast::to_json(json, t);
  return json.dump(indent);
}
}  // namespace life_lang::ast

BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::PathSegment, value, templateParameters)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::Path, segments)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::String, value)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::Integer, value)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionParameter, name, type)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionDeclaration, name, parameters, returnType)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::ReturnStatement, expr)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionCallExpr, name, parameters)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionCallStatement, expr)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::Block, statements)
BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionDefinition, declaration, body)

#endif