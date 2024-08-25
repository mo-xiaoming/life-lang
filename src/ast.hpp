#ifndef AST_HPP__
#define AST_HPP__

#include <fmt/core.h>
#include <fmt/format.h>

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <string>
#include <variant>
#include <vector>

struct Identifier {
  std::string value;
  friend auto operator<=>(Identifier const&, Identifier const&) = default;
  friend auto operator<<(std::ostream& os, Identifier const& id) -> std::ostream& { return os << fmt::to_string(id); }
};

BOOST_FUSION_ADAPT_STRUCT(Identifier, value)

namespace fmt {
template <>
struct formatter<Identifier> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(Identifier const& id, FormatContext& ctx) {
    return format_to(ctx.out(), "Identifier{{value: {}}}", id.value);
  }
};
}  // namespace fmt

struct PathSegment {
  std::string value;
  friend auto operator<=>(PathSegment const&, PathSegment const&) = default;
  friend auto operator<<(std::ostream& os, PathSegment const& ps) -> std::ostream& { return os << fmt::to_string(ps); }
};

BOOST_FUSION_ADAPT_STRUCT(PathSegment, value)

namespace fmt {
template <>
struct formatter<PathSegment> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(PathSegment const& ps, FormatContext& ctx) {
    return format_to(ctx.out(), "PathSegment{{value: {}}}", ps.value);
  }
};
}  // namespace fmt

struct Path {
  bool isAbsolute{};
  std::vector<Identifier> segments;
  friend bool operator==(Path const&, Path const&) = default;
  friend auto operator<<(std::ostream& os, Path const& path) -> std::ostream& { return os << fmt::to_string(path); }
};

BOOST_FUSION_ADAPT_STRUCT(Path, isAbsolute, segments)

namespace fmt {
template <>
struct formatter<Path> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(Path const& path, FormatContext& ctx) {
    format_to(ctx.out(), "Path{{isAbsolute: {}, segments: [", path.isAbsolute);
    for (auto const& segment : path.segments) {
      format_to(ctx.out(), "{}, ", segment);
    }
    return format_to(ctx.out(), "]}}");
  }
};
}  // namespace fmt

struct Type;
using TemplateArgument = Type;
using TemplateArgumentList = std::vector<TemplateArgument>;

struct Type {
  Path path;
  TemplateArgumentList templateArguments;
  friend bool operator==(Type const&, Type const&) = default;
  friend auto operator<<(std::ostream& os, Type const& type) -> std::ostream& { return os << fmt::to_string(type); }
};

BOOST_FUSION_ADAPT_STRUCT(Type, path, templateArguments)

namespace fmt {
template <>
struct formatter<Type> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(Type const& type, FormatContext& ctx) {
    format_to(ctx.out(), "Type{{path: {}, templateArguments: [", type.path);
    for (auto const& arg : type.templateArguments) {
      format_to(ctx.out(), "{}, ", arg);
    }
    return format_to(ctx.out(), "]}}");
  }
};
}  // namespace fmt

#endif