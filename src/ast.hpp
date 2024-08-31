#ifndef AST_HPP__
#define AST_HPP__

#include <fmt/core.h>
#include <fmt/format.h>

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <string>
#include <vector>

namespace life_lang::ast {
struct Identifier {
  std::string value;
  friend bool operator==(Identifier const&, Identifier const&) = default;
  friend auto operator<<(std::ostream& os, Identifier const& id) -> std::ostream& { return os << fmt::to_string(id); }
};
}  // namespace life_lang::ast

BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::Identifier, value)

namespace fmt {
template <>
struct formatter<life_lang::ast::Identifier> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::Identifier const& id, FormatContext& ctx) {
    return format_to(ctx.out(), "Identifier{{value: {}}}", id.value);
  }
};
}  // namespace fmt

namespace life_lang::ast {
struct Path {
  bool isAbsolute{};
  std::vector<Identifier> segments;
  friend bool operator==(Path const&, Path const&) = default;
  friend auto operator<<(std::ostream& os, Path const& path) -> std::ostream& { return os << fmt::to_string(path); }
};
}  // namespace life_lang::ast

BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::Path, isAbsolute, segments)

namespace fmt {
template <>
struct formatter<life_lang::ast::Path> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::Path const& path, FormatContext& ctx) {
    format_to(ctx.out(), "Path{{isAbsolute: {}, segments: [", path.isAbsolute);
    for (auto const& segment : path.segments) {
      format_to(ctx.out(), "{}, ", segment);
    }
    return format_to(ctx.out(), "]}}");
  }
};
}  // namespace fmt

namespace life_lang::ast {
struct Type;
using TemplateArgument = Type;
using TemplateArgumentList = std::vector<TemplateArgument>;

struct Type {
  Path path;
  TemplateArgumentList templateArguments;
  friend bool operator==(Type const&, Type const&) = default;
  friend auto operator<<(std::ostream& os, Type const& type) -> std::ostream& { return os << fmt::to_string(type); }
};
}  // namespace life_lang::ast

BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::Type, path, templateArguments)

namespace fmt {
template <>
struct formatter<life_lang::ast::Type> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::Type const& type, FormatContext& ctx) {
    format_to(ctx.out(), "Type{{path: {}, templateArguments: [", type.path);
    for (auto const& arg : type.templateArguments) {
      format_to(ctx.out(), "{}, ", arg);
    }
    return format_to(ctx.out(), "]}}");
  }
};
}  // namespace fmt

namespace life_lang::ast {
struct FunctionParameter {
  Identifier name;
  Type type;
  friend bool operator==(FunctionParameter const&, FunctionParameter const&) = default;
  friend auto operator<<(std::ostream& os, FunctionParameter const& arg) -> std::ostream& {
    return os << fmt::to_string(arg);
  }
};
}  // namespace life_lang::ast

BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionParameter, name, type)

namespace fmt {
template <>
struct formatter<life_lang::ast::FunctionParameter> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::FunctionParameter const& arg, FormatContext& ctx) {
    return format_to(ctx.out(), "FunctionParameter{{name: {}, type: {}}}", arg.name, arg.type);
  }
};
}  // namespace fmt

namespace life_lang::ast {
using FunctionParameterList = std::vector<life_lang::ast::FunctionParameter>;
}

namespace fmt {
template <>
struct formatter<life_lang::ast::FunctionParameterList> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::FunctionParameterList const& args, FormatContext& ctx) {
    format_to(ctx.out(), "FunctionParameterList{{");
    for (auto const& arg : args) {
      format_to(ctx.out(), "{}, ", arg);
    }
    return format_to(ctx.out(), "}}");
  }
};
}  // namespace fmt

namespace life_lang::ast {
struct FunctionDeclaration {
  Identifier name;
  FunctionParameterList parameters;
  Type returnType;
  friend bool operator==(FunctionDeclaration const&, FunctionDeclaration const&) = default;
  friend auto operator<<(std::ostream& os, FunctionDeclaration const& decl) -> std::ostream& {
    return os << fmt::to_string(decl);
  }
};
}  // namespace life_lang::ast

BOOST_FUSION_ADAPT_STRUCT(life_lang::ast::FunctionDeclaration, name, parameters, returnType)

namespace fmt {
template <>
struct formatter<life_lang::ast::FunctionDeclaration> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(life_lang::ast::FunctionDeclaration const& decl, FormatContext& ctx) {
    return format_to(
        ctx.out(), "FunctionDeclaration{{name: {}, parameters: {}, returnType: {}}}", decl.name, decl.parameters,
        decl.returnType
    );
  }
};
}  // namespace fmt
#endif