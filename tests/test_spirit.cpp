#include <fmt/core.h>
#include <gtest/gtest.h>

#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/zip.hpp>
#include <boost/hana.hpp>
#include <boost/spirit/home/x3.hpp>
#include <lib.hpp>
#include <string_view>
#include <utility>
#include <vector>

// keep it here till we have meaningful things to test in lib
// otherwise, coverage will fail for nothing to cover
TEST(CoverageTest, Foo) { EXPECT_EQ("hello", life_lang::foo("hello")); }

namespace spirit = boost::spirit::x3;
namespace fusion = boost::fusion;
using namespace std::string_view_literals;

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
  auto const succeed = spirit::parse(input_start, params.input.cend(), spirit::double_, result);
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

struct ParseTwoDoublesTestParams {
  std::string_view name;
  std::string_view input;
  std::pair<double, double> expected_value;
  bool should_succeed;
  bool should_consume_all;
};  // namespace std::ranges

class ParseTwoDoublesTest : public ::testing::TestWithParam<ParseTwoDoublesTestParams> {};

auto ExpectTwoDoublesEq(std::pair<double, double> const& lhs, std::pair<double, double> const& rhs) {
  auto const zipped = fusion::zip(lhs, rhs);
  fusion::for_each(zipped, [](auto const& tup) { EXPECT_DOUBLE_EQ(fusion::at_c<0>(tup), fusion::at_c<1>(tup)); });
};

TEST_P(ParseTwoDoublesTest, ParseTwoDoubles) {
  auto const& params = GetParam();
  auto const* input_start = params.input.cbegin();
  std::pair<double, double> result{};
  auto const succeed =
      spirit::phrase_parse(input_start, params.input.cend(), spirit::double_ >> spirit::double_, spirit::space, result);
  EXPECT_EQ(params.should_succeed, succeed);
  if (params.should_succeed) {
    EXPECT_EQ(params.should_consume_all, input_start == params.input.cend());
    ExpectTwoDoublesEq(params.expected_value, result);
  }
}

INSTANTIATE_TEST_SUITE_P(ParseTwoDoublesTests, ParseTwoDoublesTest,
                         ::testing::Values(ParseTwoDoublesTestParams{.name = "TwoDoubles",
                                                                     .input = "3.14 42.0",
                                                                     .expected_value = std::pair{3.14, 42.0},
                                                                     .should_succeed = true,
                                                                     .should_consume_all = true},
                                           ParseTwoDoublesTestParams{.name = "TwoDoublesTwoSpacesInBetween",
                                                                     .input = "3.14  42.0",
                                                                     .expected_value = std::pair{3.14, 42.0},
                                                                     .should_succeed = true,
                                                                     .should_consume_all = true}),
                         [](testing::TestParamInfo<ParseTwoDoublesTestParams> const& param_info) {
                           return std::string{param_info.param.name};
                         });

TEST(ParsePairOfDoublesTest, ParsePairOfDoubles) {
  auto const input = "(3.14, 42.0)"sv;
  auto const* input_start = input.cbegin();
  std::vector<double> result{};
  auto const succeed = spirit::phrase_parse(
      input_start, input.cend(), '(' >> spirit::double_ >> *(',' >> spirit::double_) >> ')', spirit::space, result);
  EXPECT_TRUE(succeed);
  EXPECT_EQ(input.cend(), input_start);
  auto const expected_result = std::vector{3.14, 42.0};
  EXPECT_EQ(expected_result, result);
}

TEST(ActionTest, Action) {
  auto const input = "{42}"sv;
  auto const* input_start = input.cbegin();
  auto const print_action = [](auto& ctx) { fmt::print("{}\n", spirit::_attr(ctx)); };
  auto const succeed = spirit::parse(input_start, input.cend(), '{' >> spirit::int_[print_action] >> '}');
  EXPECT_TRUE(succeed);
  EXPECT_EQ(input.cend(), input_start);
}