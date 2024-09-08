#include "utils.hpp"

using life_lang::ast::MakePath;
using life_lang::ast::MakePathSegment;
using life_lang::ast::PathSegment;

PARSE_TEST(PathSegment)

INSTANTIATE_TEST_SUITE_P(
    , ParsePathSegmentTest,
    ::testing::Values(
        PathSegmentTestParamsType{
            .name = "starsWithDigit",
            .input = "0abc",
            .expected = MakePathSegment(""),
            .shouldSucceed = false,
            .rest = "0abc"
        },
        PathSegmentTestParamsType{
            .name = "spaceAfterInt",
            .input = "Int  {",
            .expected = MakePathSegment("Int"),
            .shouldSucceed = true,
            .rest = "{"
        },
        PathSegmentTestParamsType{
            .name = "h340", .input = "h340", .expected = MakePathSegment("h340"), .shouldSucceed = true, .rest = ""
        },
        PathSegmentTestParamsType{
            .name = "hello_world",
            .input = "hello_world",
            .expected = MakePathSegment("hello_world"),
            .shouldSucceed = true,
            .rest = ""
        },
        PathSegmentTestParamsType{
            .name = "templateParameter",
            .input = "Hello<Int>",
            .expected = MakePathSegment("Hello", {MakePath("Int")}),
            .shouldSucceed = true,
            .rest = ""
        },
        PathSegmentTestParamsType{
            .name = "templateParameterWithMultipleSegments",
            .input = "Hello<Int, Double>",
            .expected = MakePathSegment("Hello", {MakePath("Int"), MakePath("Double")}),
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<PathSegmentTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);