#ifndef AST_HPP__
#define AST_HPP__

#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <concepts>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace life_lang::ast {

// ============================================================================
// Forward Declarations
// ============================================================================

struct Path_Segment;
struct Function_Call_Expr;
struct Function_Definition;
struct Block;

// ============================================================================
// Type & Path System
// ============================================================================

struct Path : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Path";
  std::vector<Path_Segment> segments;
};

struct Path_Segment : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Path_Segment";
  std::string value;
  std::vector<Path> template_parameters;
};

// ============================================================================
// Literal Types
// ============================================================================

struct String : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "String";
  std::string value;
};

struct Integer : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Integer";
  std::string value;
};

// ============================================================================
// Expression Types
// ============================================================================

struct Expr : boost::spirit::x3::variant<Path, boost::spirit::x3::forward_ast<Function_Call_Expr>, String, Integer>,
              boost::spirit::x3::position_tagged {
  using Base_Type =
      boost::spirit::x3::variant<Path, boost::spirit::x3::forward_ast<Function_Call_Expr>, String, Integer>;
  using Base_Type::Base_Type;
  using Base_Type::operator=;
};

struct Function_Call_Expr : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Function_Call_Expr";
  Path name;
  std::vector<Expr> parameters;
};

// ============================================================================
// Statement Types
// ============================================================================

struct Function_Call_Statement : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Function_Call_Statement";
  Function_Call_Expr expr;
};

struct Return_Statement : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Return_Statement";
  Expr expr;
};

struct Statement : boost::spirit::x3::variant<
                       boost::spirit::x3::forward_ast<Function_Definition>, Function_Call_Statement, Return_Statement,
                       boost::spirit::x3::forward_ast<Block>>,
                   boost::spirit::x3::position_tagged {
  using Base_Type = boost::spirit::x3::variant<
      boost::spirit::x3::forward_ast<Function_Definition>, Function_Call_Statement, Return_Statement,
      boost::spirit::x3::forward_ast<Block>>;
  using Base_Type::Base_Type;
  using Base_Type::operator=;
};

struct Block : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Block";
  std::vector<Statement> statements;
};

// ============================================================================
// Function Types
// ============================================================================

struct Function_Parameter : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Function_Parameter";
  std::string name;
  Path type;
};

struct Function_Declaration : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Function_Declaration";
  std::string name;
  std::vector<Function_Parameter> parameters;
  Path return_type;
};

struct Function_Definition : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Function_Definition";
  Function_Declaration declaration;
  Block body;
};

// ============================================================================
// Module Types
// ============================================================================

struct Module : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Module";
  std::vector<Statement> statements;
};

// ============================================================================
// Helper Functions for AST Construction
// ============================================================================

inline Path_Segment make_path_segment(std::string&& a_value, std::vector<Path>&& a_template_parameters) {
  return Path_Segment{{}, std::move(a_value), std::move(a_template_parameters)};
}

inline Path_Segment make_path_segment(std::string&& a_value) { return make_path_segment(std::move(a_value), {}); }

template <typename... Args>
  requires((std::same_as<Args, Path_Segment> || std::constructible_from<std::string, Args>) && ...)
inline Path make_path(Args&&... a_args) {
  std::vector<Path_Segment> segments;
  segments.reserve(sizeof...(a_args));
  (
      [&] {
        if constexpr (std::same_as<std::remove_cvref_t<Args>, Path_Segment>) {
          segments.push_back(std::forward<Args>(a_args));
        } else {
          segments.push_back(make_path_segment(std::string(std::forward<Args>(a_args))));
        }
      }(),
      ...
  );
  return Path{{}, std::move(segments)};
}

inline String make_string(std::string&& a_value) { return String{{}, std::move(a_value)}; }

inline Integer make_integer(std::string a_value) noexcept { return Integer{{}, std::move(a_value)}; }

inline Expr make_expr(Path&& a_path) { return Expr{std::move(a_path)}; }
inline Expr make_expr(String&& a_str) { return Expr{std::move(a_str)}; }
inline Expr make_expr(Integer&& a_integer) noexcept { return Expr{std::move(a_integer)}; }
inline Expr make_expr(Function_Call_Expr&& a_call) { return Expr{std::move(a_call)}; }

inline Function_Call_Expr make_function_call_expr(Path&& a_name, std::vector<Expr>&& a_parameters) {
  return Function_Call_Expr{{}, std::move(a_name), std::move(a_parameters)};
}

inline Function_Call_Statement make_function_call_statement(Function_Call_Expr&& a_expr) {
  return Function_Call_Statement{{}, std::move(a_expr)};
}

inline Return_Statement make_return_statement(Expr&& a_expr) { return Return_Statement{{}, std::move(a_expr)}; }

inline Statement make_statement(Function_Call_Statement&& a_call) { return Statement{std::move(a_call)}; }
inline Statement make_statement(Return_Statement&& a_ret) { return Statement{std::move(a_ret)}; }
inline Statement make_statement(Block&& a_block) { return Statement{std::move(a_block)}; }
inline Statement make_statement(Function_Definition&& a_def) { return Statement{std::move(a_def)}; }

inline Block make_block(std::vector<Statement>&& a_statements) { return Block{{}, std::move(a_statements)}; }

inline Function_Parameter make_function_parameter(std::string&& a_name, Path&& a_type) {
  return Function_Parameter{{}, std::move(a_name), std::move(a_type)};
}

inline Function_Declaration make_function_declaration(
    std::string&& a_name, std::vector<Function_Parameter>&& a_parameters, Path&& a_return_type
) {
  return Function_Declaration{{}, std::move(a_name), std::move(a_parameters), std::move(a_return_type)};
}

inline Function_Definition make_function_definition(Function_Declaration&& a_decl, Block&& a_body) {
  return Function_Definition{{}, std::move(a_decl), std::move(a_body)};
}

inline Module make_module(std::vector<Statement>&& a_statements) { return Module{{}, std::move(a_statements)}; }

// ============================================================================
// JSON Serialization
// ============================================================================

void to_json(nlohmann::json& a_json, Path_Segment const& a_segment);
inline void to_json(nlohmann::json& a_json, Path const& a_path) {
  a_json[Path::k_name] = {{"segments", a_path.segments}};
}
inline void to_json(nlohmann::json& a_json, Path_Segment const& a_segment) {
  a_json[Path_Segment::k_name] = {{"value", a_segment.value}, {"templateParameters", a_segment.template_parameters}};
};
inline void to_json(nlohmann::json& a_json, String const& a_str) { a_json[String::k_name] = {{"value", a_str.value}}; }
inline void to_json(nlohmann::json& a_json, Integer const& a_integer) {
  a_json[Integer::k_name] = {{"value", a_integer.value}};
}
inline void to_json(nlohmann::json& a_json, Function_Parameter const& a_param) {
  a_json[Function_Parameter::k_name] = {{"name", a_param.name}, {"type", a_param.type}};
}
inline void to_json(nlohmann::json& a_json, Function_Declaration const& a_decl) {
  a_json[Function_Declaration::k_name] = {
      {"name", a_decl.name}, {"parameters", a_decl.parameters}, {"returnType", a_decl.return_type}
  };
}
void to_json(nlohmann::json& a_json, Function_Call_Expr const& a_call);
inline void to_json(nlohmann::json& a_json, Expr const& a_expr) {
  boost::apply_visitor([&a_json](auto const& a_value) { a_json = a_value; }, a_expr);
}
inline void to_json(nlohmann::json& a_json, Function_Call_Expr const& a_call) {
  a_json[Function_Call_Expr::k_name] = {{"name", a_call.name}, {"parameters", a_call.parameters}};
}
inline void to_json(nlohmann::json& a_json, Function_Call_Statement const& a_call) {
  a_json[Function_Call_Statement::k_name] = {{"expr", a_call.expr}};
}
inline void to_json(nlohmann::json& a_json, Return_Statement const& a_ret) {
  a_json[Return_Statement::k_name] = {{"expr", a_ret.expr}};
}
void to_json(nlohmann::json& a_json, Function_Definition const& a_def);
void to_json(nlohmann::json& a_json, Block const& a_block);
inline void to_json(nlohmann::json& a_json, Statement const& a_stmt) {
  boost::apply_visitor([&a_json](auto const& a_value) { a_json = a_value; }, a_stmt);
}
inline void to_json(nlohmann::json& a_json, Block const& a_block) {
  a_json[Block::k_name] = {{"statements", a_block.statements}};
}
inline void to_json(nlohmann::json& a_json, Function_Definition const& a_def) {
  a_json[Function_Definition::k_name] = {{"declaration", a_def.declaration}, {"body", a_def.body}};
}
inline void to_json(nlohmann::json& a_json, Module const& a_module) {
  a_json[Module::k_name] = {{"statements", a_module.statements}};
}

// ============================================================================
// Utility
// ============================================================================

template <typename T>
concept JsonStringConvertible = requires(nlohmann::json& a_j, T const& a_t) {
  { life_lang::ast::to_json(a_j, a_t) };
};

std::string to_json_string(JsonStringConvertible auto const& a_t, int a_indent) {
  nlohmann::json json;
  life_lang::ast::to_json(json, a_t);
  return json.dump(a_indent);
}

}  // namespace life_lang::ast

#endif
