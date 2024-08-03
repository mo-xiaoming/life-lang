
#include <fmt/core.h>
#include <gtest/gtest.h>

#include <boost/spirit/home/x3.hpp>

namespace x3 = boost::spirit::x3;

struct ParseDoubleTestParams {
  std::string_view name;
  std::string_view input;
  double expected_value;
  bool should_succeed;
  bool should_consume_all;
};  // namespace std::ranges

class ParseDoubleTest : public ::testing::TestWithParam<ParseDoubleTestParams> {};

TEST_P(ParseDoubleTest, ParseDouble) {
  auto const& params = GetParam();
  auto const* input_start = params.input.cbegin();
  double result{};
  auto const succeed = x3::parse(input_start, params.input.cend(), x3::double_, result);
  EXPECT_EQ(params.should_succeed, succeed);
  if (params.should_succeed) {
    EXPECT_EQ(params.should_consume_all, input_start == params.input.cend());
    EXPECT_DOUBLE_EQ(params.expected_value, result);
  }
}

INSTANTIATE_TEST_SUITE_P(ParseDoubleTests, ParseDoubleTest,
                         ::testing::Values(ParseDoubleTestParams{.name = "NormalNumber",
                                                                 .input = "3.14",
                                                                 .expected_value = 3.14,
                                                                 .should_succeed = true,
                                                                 .should_consume_all = true},
                                           ParseDoubleTestParams{.name = "NumberFollowedBySpace",
                                                                 .input = "3.14 ",
                                                                 .expected_value = 3.14,
                                                                 .should_succeed = true,
                                                                 .should_consume_all = false},
                                           ParseDoubleTestParams{.name = "NumberPrecededBySpace",
                                                                 .input = " 3.14",
                                                                 .expected_value = 0.0,
                                                                 .should_succeed = false,
                                                                 .should_consume_all = false},
                                           ParseDoubleTestParams{.name = "NumberPrecededByPlusSign",
                                                                 .input = "+3.14",
                                                                 .expected_value = 3.14,
                                                                 .should_succeed = true,
                                                                 .should_consume_all = true},
                                           ParseDoubleTestParams{.name = "NegativeNumber",
                                                                 .input = "-2.71",
                                                                 .expected_value = -2.71,
                                                                 .should_succeed = true,
                                                                 .should_consume_all = true},
                                           ParseDoubleTestParams{.name = "NegativeSignFollowedBySpace",
                                                                 .input = "- 2.71",
                                                                 .expected_value = -2.71,
                                                                 .should_succeed = false,
                                                                 .should_consume_all = false},
                                           ParseDoubleTestParams{.name = "ZeroDotZero",
                                                                 .input = "0.0",
                                                                 .expected_value = 0.0,
                                                                 .should_succeed = true,
                                                                 .should_consume_all = true},
                                           ParseDoubleTestParams{.name = "NotANumber",
                                                                 .input = "abc",
                                                                 .expected_value = 0.0,
                                                                 .should_succeed = false,
                                                                 .should_consume_all = false}),
                         [](testing::TestParamInfo<ParseDoubleTestParams> const& param_info) {
                           return std::string{param_info.param.name};
                         });