#include <fmt/core.h>
#include <fmt/format.h>
#include <gtest/gtest.h>

#include <boost/fusion/include/io.hpp>
#include <rules.hpp>
#include <string_view>

#define PARSE_TEST(name)                                                                          \
  using name##TestParamsType = ParseTestParams<name>;                                             \
  class Parse##name##Test : public ::testing::TestWithParam<name##TestParamsType> {};             \
                                                                                                  \
  TEST_P(Parse##name##Test, Parse##name) {                                                        \
    auto const& params = GetParam();                                                              \
    auto inputStart = params.input.cbegin();                                                      \
    auto const inputEnd = params.input.cend();                                                    \
    std::ostringstream errorMsg;                                                                  \
    auto const got = life_lang::internal::Parse##name(inputStart, inputEnd, errorMsg);            \
    EXPECT_EQ(params.shouldSucceed, bool(got)) << (got ? ToJsonString(*got, 2) : errorMsg.str()); \
    auto const rest = std::string_view{inputStart, inputEnd};                                     \
    EXPECT_EQ(params.rest, rest);                                                                 \
    EXPECT_EQ(ToJsonString(params.expected, 2), ToJsonString(*got, 2));                           \
  }

template <typename AstType>
struct ParseTestParams {
  std::string_view name;
  std::string input;
  AstType expected;
  bool shouldSucceed{};
  std::string_view rest;
};  // namespace std::ranges

template <typename T>
void PrintTo(ParseTestParams<T> const& params, std::ostream* os) {
  *os << fmt::format(
      R"({{.input = "{}", .expected = {}, .shouldSucceed = {}, .rest = "{}"}})", params.input,
      ToJsonString(params.expected, -1), params.shouldSucceed, params.rest
  );
}

namespace life_lang::ast {
void PrintTo(JsonStringConvertible auto const& ast, std::ostream* os) { *os << ToJsonString(ast, 2); }
}  // namespace life_lang::ast