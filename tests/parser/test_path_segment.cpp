#include "utils.hpp"

using life_lang::ast::Expr;
using life_lang::ast::Path;
using life_lang::ast::PathSegment;
using life_lang::ast::Statement;

PARSE_TEST(PathSegment)

INSTANTIATE_TEST_SUITE_P(
    , ParsePathSegmentTest,
    ::testing::Values(
        PathSegmentTestParamsType{
            .name = "spaceAfterInt",
            .input = "Int  {",
            .expected = PathSegment{.value = "Int", .templateParameters = {}},
            .shouldSucceed = true,
            .rest = "{"
        },
        PathSegmentTestParamsType{
            .name = "h340",
            .input = "h340",
            .expected = PathSegment{.value = "h340", .templateParameters = {}},
            .shouldSucceed = true,
            .rest = ""
        },
        PathSegmentTestParamsType{
            .name = "hello_world",
            .input = "hello_world",
            .expected = PathSegment{.value = "hello_world", .templateParameters = {}},
            .shouldSucceed = true,
            .rest = ""
        },
        PathSegmentTestParamsType{
            .name = "templateParameter",
            .input = "Hello<Int>",
            .expected =
                PathSegment{
                    .value = "Hello",
                    .templateParameters =
                        {
                            Path{
                                .segments =
                                    {
                                        PathSegment{.value = "Int", .templateParameters = {}},
                                    },
                            },
                        },
                },
            .shouldSucceed = true,
            .rest = ""
        },
        PathSegmentTestParamsType{
            .name = "templateParameterWithMultipleSegments",
            .input = "Hello<Int, Double>",
            .expected =
                PathSegment{
                    .value = "Hello",
                    .templateParameters =
                        {
                            Path{
                                .segments =
                                    {
                                        PathSegment{.value = "Int", .templateParameters = {}},
                                    },
                            },
                            Path{
                                .segments =
                                    {
                                        PathSegment{.value = "Double", .templateParameters = {}},
                                    },
                            },
                        },
                },
            .shouldSucceed = true,
            .rest = ""
        },
        PathSegmentTestParamsType{
            .name = "templateParameterWithMultipleSegmentsAndSpace",
            .input = "Hello<Int, Double, Std.Array>",
            .expected =
                PathSegment{
                    .value = "Hello",
                    .templateParameters =
                        {
                            Path{
                                .segments =
                                    {
                                        PathSegment{.value = "Int", .templateParameters = {}},
                                    },
                            },
                            Path{
                                .segments =
                                    {
                                        PathSegment{.value = "Double", .templateParameters = {}},
                                    },
                            },
                            Path{
                                .segments =
                                    {
                                        PathSegment{.value = "Std", .templateParameters = {}},
                                        PathSegment{.value = "Array", .templateParameters = {}},
                                    },
                            },
                        },
                },
            .shouldSucceed = true,
            .rest = ""
        },
        PathSegmentTestParamsType{
            .name = "templateParameterWithMultipleSegmentsAndSpaceAndTemplate",
            .input = "Hello<Int, Double, Std.Array<Math.Double>>",
            .expected =
                PathSegment{
                    .value = "Hello",
                    .templateParameters =
                        {
                            Path{
                                .segments =
                                    {
                                        PathSegment{.value = "Int", .templateParameters = {}},
                                    },
                            },
                            Path{
                                .segments =
                                    {
                                        PathSegment{.value = "Double", .templateParameters = {}},
                                    },
                            },
                            Path{
                                .segments =
                                    {
                                        PathSegment{.value = "Std", .templateParameters = {}},
                                        PathSegment{
                                            .value = "Array",
                                            .templateParameters =
                                                {
                                                    Path{
                                                        .segments =
                                                            {
                                                                PathSegment{.value = "Math", .templateParameters = {}},
                                                                PathSegment{
                                                                    .value = "Double", .templateParameters = {}
                                                                },
                                                            },
                                                    },
                                                },
                                        },
                                    },
                            },
                        },
                },
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<PathSegmentTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);