
#include <boost/spirit/home/x3.hpp>
#include <fmt/core.h>
#include <gtest/gtest.h>

namespace x3 = boost::spirit::x3;

TEST(TestSpririt, ParseDoubleTest) {
  std::string_view input = "3.14 ";
  auto const* input_start = input.cbegin();
  double result{};
  auto const succeed = x3::parse(input_start, input.cend(), x3::double_, result);
  EXPECT_TRUE(succeed);
  EXPECT_TRUE(input_start == input.cend());
  EXPECT_DOUBLE_EQ(result, 3.14);
}