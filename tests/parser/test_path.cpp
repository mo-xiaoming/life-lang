#include "utils.hpp"

using life_lang::ast::Expr;
using life_lang::ast::Path;
using life_lang::ast::PathSegment;
using life_lang::ast::Statement;

PARSE_TEST(Path)

INSTANTIATE_TEST_SUITE_P(
    , ParsePathTest,
    ::testing::Values(PathTestParamsType{
        .name = "all",
        .input = "A.B.World<Int<e>, Double.c>.Hi.a.b",
        .expected =
            {
                .segments =
                    {
                        PathSegment{.value = "A", .templateParameters = {}},
                        PathSegment{.value = "B", .templateParameters = {}},
                        PathSegment{
                            .value = "World",
                            .templateParameters =
                                {
                                    Path{
                                        .segments =
                                            {
                                                PathSegment{
                                                    .value = "Int",
                                                    .templateParameters =
                                                        {
                                                            Path{
                                                                .segments =
                                                                    {
                                                                        PathSegment{
                                                                            .value = "e", .templateParameters = {}
                                                                        },
                                                                    },
                                                            },
                                                        }
                                                },
                                            }
                                    },
                                    Path{
                                        .segments =
                                            {
                                                PathSegment{.value = "Double", .templateParameters = {}},
                                                PathSegment{.value = "c", .templateParameters = {}},
                                            }
                                    },
                                },
                        },
                        PathSegment{.value = "Hi", .templateParameters = {}},
                        PathSegment{.value = "a", .templateParameters = {}},
                        PathSegment{.value = "b", .templateParameters = {}},
                    },
            },

        .shouldSucceed = true,
        .rest = ""
    }),
    [](testing::TestParamInfo<PathTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);