#include "utils.hpp"

using life_lang::ast::Expr;
using life_lang::ast::FunctionCallExpr;
using life_lang::ast::Path;
using life_lang::ast::PathSegment;
using life_lang::ast::Statement;

PARSE_TEST(Expr)

INSTANTIATE_TEST_SUITE_P(
    , ParseExprTest,
    ::testing::Values(
        ExprTestParamsType{
            .name = "functionCall",
            .input = "hello()",
            .expected = Expr{
                FunctionCallExpr{
                    .name =
                        Path{
                            .segments = {
                                PathSegment{.value = "hello", .templateParameters = {}},
                            },
                        },
                    .parameters = {}
                },
            },
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithDataPath",
            .input = "hello.a.b()",
            .expected = Expr{
                FunctionCallExpr{
                    .name =
                        Path{
                            .segments = {
                                PathSegment{.value = "hello", .templateParameters = {}},
                                PathSegment{.value = "a", .templateParameters = {}},
                                PathSegment{.value = "b", .templateParameters = {}},
                            },
                        },
                    .parameters = {}
                },
            },
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithTypePath",
            .input = "A.B.hello()",
            .expected = Expr{
                FunctionCallExpr{
                    .name =
                        Path{
                            .segments = {
                                PathSegment{.value = "A", .templateParameters = {}},
                                PathSegment{.value = "B", .templateParameters = {}},
                                PathSegment{.value = "hello", .templateParameters = {}},
                            },
                        },
                    .parameters = {}
                },
            },
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithTypeAndDataPath",
            .input = "A.B.hello.a.b()",
            .expected = Expr{
                    FunctionCallExpr{
                    .name =
                        Path{
                            .segments = {
                                PathSegment{.value = "A", .templateParameters = {}},
                                PathSegment{.value = "B", .templateParameters = {}},
                                PathSegment{.value = "hello", .templateParameters = {}},
                                PathSegment{.value = "a", .templateParameters = {}},
                                PathSegment{.value = "b", .templateParameters = {}},
                            },
                        },
                    .parameters = {}
                },
            },
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithEverything",
            .input = "A.B<Int, Double>.hello.a.b()",
            .expected = Expr{
                FunctionCallExpr{
                    .name =
                        Path{
                            .segments = {
                                PathSegment{.value = "A", .templateParameters = {}},
                                PathSegment{
                                    .value = "B",
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
                                PathSegment{.value = "hello", .templateParameters = {}},
                                PathSegment{.value = "a", .templateParameters = {}},
                                PathSegment{.value = "b", .templateParameters = {}},
                                },
                        },
                        .parameters = {}
                    }
            },
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType {
            .name = "functionCallWithArguments",
            .input = "hello(a, b, c)",
            .expected = Expr{
                FunctionCallExpr{
                    .name =
                        Path{
                            .segments =
                                {
                                    PathSegment{.value = "hello", .templateParameters = {}},
                                },
                        },
                    .parameters =
                        {
                            Expr{Path{
                                .segments =
                                    {
                                        PathSegment{.value = "a", .templateParameters = {}},
                                    },
                            }},
                            Expr{Path{
                                .segments =
                                    {
                                        PathSegment{.value = "b", .templateParameters = {}},
                                    },
                            }},
                            Expr{Path{
                                .segments =
                                    {
                                        PathSegment{.value = "c", .templateParameters = {}},
                                    },
                            }},
                        },
                },
            },
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType {
        .name = "functionCallWithArgumentsHavePaths", .input = "hello(a, b.c.world, c.world)",
        .expected = Expr{
                FunctionCallExpr{
                    .name =
                        Path{
                            .segments =
                                {
                                    PathSegment{.value = "hello", .templateParameters = {}},
                                },
                        },
                    .parameters =
                        {
                            Expr{Path{
                                .segments =
                                    {
                                        PathSegment{.value = "a", .templateParameters = {}},
                                    },
                            }},
                            Expr{Path{
                                .segments =
                                    {
                                        PathSegment{.value = "b", .templateParameters = {}},
                                        PathSegment{.value = "c", .templateParameters = {}},
                                        PathSegment{.value = "world", .templateParameters = {}},
                                    },
                            }},
                            Expr{Path{
                                .segments =
                                    {
                                        PathSegment{.value = "c", .templateParameters = {}},
                                        PathSegment{.value = "world", .templateParameters = {}},
                                    },
                            }},

                        },
                },
            },
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType {
            .name = "functionCallWithArgumentIsFunctionCall",
            .input = "hello(A.B.a.d(), c.world(a))",
            .expected = Expr{
                    FunctionCallExpr{
                        .name = Path{
                            .segments = {
                                PathSegment{.value = "hello", .templateParameters = {}},
                            },
                        },
                        .parameters =
                        {
                            Expr{FunctionCallExpr{
                                .name = Path{
                                    .segments = {
                                        PathSegment{.value = "A", .templateParameters = {}},
                                        PathSegment{.value = "B", .templateParameters = {}},
                                        PathSegment{.value = "a", .templateParameters = {}},
                                        PathSegment{.value = "d", .templateParameters = {}},
                                    },
                                },
                                .parameters = {},
                            }},
                            Expr{FunctionCallExpr{
                                .name = Path{
                                    .segments = {
                                        PathSegment{.value = "c", .templateParameters = {}},
                                        PathSegment{.value = "world", .templateParameters = {}},
                                    },
                                },
                                .parameters = {
                                    Expr{
                                        Path{
                                            .segments = {
                                                PathSegment{.value = "a", .templateParameters = {}},
                                            },
                                        },
                                    },
                                },
                            }}
                        },
                    },
            },
            .shouldSucceed = true,
            .rest = ""
        }),
        [](testing::TestParamInfo<ExprTestParamsType> const& paramInfo) {
            return std::string{paramInfo.param.name};
        }
);