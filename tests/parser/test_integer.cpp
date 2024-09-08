#include "utils.hpp"

using life_lang::ast::Integer;
using life_lang::ast::MakeInteger;

PARSE_TEST(Integer)

INSTANTIATE_TEST_SUITE_P(
    , ParseIntegerTest,
    ::testing::Values(
        IntegerTestParamsType{
            .name = "zero", .input = "0", .expected = MakeInteger("0"), .shouldSucceed = true, .rest = ""
        },
        IntegerTestParamsType{
            .name = "oneTwoThree", .input = "123", .expected = MakeInteger("123"), .shouldSucceed = true, .rest = ""
        },
        IntegerTestParamsType{
            .name = "startsWithZero",
            .input = "0123",
            .expected = MakeInteger(""),
            .shouldSucceed = false,
            .rest = "0123"
        },
        IntegerTestParamsType{
            .name = "delimited", .input = "12_34_5", .expected = MakeInteger("12345"), .shouldSucceed = true, .rest = ""
        },
        IntegerTestParamsType{
            .name = "zeroStartsWithUnderscore",
            .input = "_0",
            .expected = MakeInteger(""),
            .shouldSucceed = false,
            .rest = "_0"
        },
        IntegerTestParamsType{
            .name = "zeroEndsWithUnderscore",
            .input = "0_",
            .expected = MakeInteger(""),
            .shouldSucceed = false,
            .rest = "0_"
        },
        IntegerTestParamsType{
            .name = "startsWithUnderscore",
            .input = "_12",
            .expected = MakeInteger(""),
            .shouldSucceed = false,
            .rest = "_12"
        },
        IntegerTestParamsType{
            .name = "endsWithUnderscore",
            .input = "12_",
            .expected = MakeInteger(""),
            .shouldSucceed = false,
            .rest = "12_"
        }
    ),
    [](testing::TestParamInfo<IntegerTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);