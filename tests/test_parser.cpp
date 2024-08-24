#include <gtest/gtest.h>

#include <string_view>
#include <utility>

#include "rules.hpp"

template <typename AstType>
struct ParseTestParams {
  std::string_view name;
  std::string input;
  AstType expectedValue;
  bool shouldSucceed{};
  bool shouldConsumeAll{};
};  // namespace std::ranges

class ParseIdentifierTest : public ::testing::TestWithParam<ParseTestParams<std::string>> {};

TEST_P(ParseIdentifierTest, ParseIdentifier) {
  auto const& params = GetParam();
  auto inputStart = params.input.cbegin();
  auto const inputEnd = params.input.cend();
  std::ostringstream oss;
  auto const ret = life_lang::internal::ParseIdentifier(inputStart, inputEnd, oss);
  EXPECT_EQ(params.shouldSucceed, ret.first);
  if (params.shouldSucceed) {
    EXPECT_EQ(params.shouldConsumeAll, inputStart == inputEnd);
    EXPECT_EQ(params.expectedValue, ret.second);
  }
}

INSTANTIATE_TEST_SUITE_P(
    ParseIdentifierTest, ParseIdentifierTest,
    ::testing::Values(ParseTestParams<std::string>{.name = "allLowerAlphabets",
                                                   .input = "hello",
                                                   .expectedValue = "hello",
                                                   .shouldSucceed = true,
                                                   .shouldConsumeAll = true},
                      ParseTestParams<std::string>{.name = "allUpperAlphabets",
                                                   .input = "HELLO",
                                                   .expectedValue = "HELLO",
                                                   .shouldSucceed = true,
                                                   .shouldConsumeAll = true},
                      ParseTestParams<std::string>{.name = "mixedAlphaNumeric",
                                                   .input = "h340",
                                                   .expectedValue = "h340",
                                                   .shouldSucceed = true,
                                                   .shouldConsumeAll = true},
                      ParseTestParams<std::string>{.name = "allUpperAlphabetsStartsWithUnderscore",
                                                   .input = "_hello",
                                                   .expectedValue = "_hello",
                                                   .shouldSucceed = true,
                                                   .shouldConsumeAll = true},
                      ParseTestParams<std::string>{.name = "allLowerAlphabetsStartsWithUnderscore",
                                                   .input = "_HELLO",
                                                   .expectedValue = "_HELLO",
                                                   .shouldSucceed = true,
                                                   .shouldConsumeAll = true},
                      ParseTestParams<std::string>{.name = "allUpperAlphabetsStartsWithMultipleUnderscore",
                                                   .input = "__hello",
                                                   .expectedValue = "__hello",
                                                   .shouldSucceed = true,
                                                   .shouldConsumeAll = true},
                      ParseTestParams<std::string>{.name = "allLowerAlphabetsStartsWithMultipleUnderscore",
                                                   .input = "__HELLO",
                                                   .expectedValue = "__HELLO",
                                                   .shouldSucceed = true,
                                                   .shouldConsumeAll = true},
                      ParseTestParams<std::string>{.name = "underScoreInBetween",
                                                   .input = "_he__ll_o",
                                                   .expectedValue = "_he__ll_o",
                                                   .shouldSucceed = true,
                                                   .shouldConsumeAll = true},
                      ParseTestParams<std::string>{.name = "underScoreAtEnd",
                                                   .input = "_hello_",
                                                   .expectedValue = "_hello_",
                                                   .shouldSucceed = true,
                                                   .shouldConsumeAll = true},
                      ParseTestParams<std::string>{.name = "underScoreOnly",
                                                   .input = "_",
                                                   .expectedValue = "_",
                                                   .shouldSucceed = true,
                                                   .shouldConsumeAll = true}),
    [](testing::TestParamInfo<ParseTestParams<std::string>> const& paramInfo) {
      return std::string{paramInfo.param.name};
    });