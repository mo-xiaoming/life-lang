#include "utils.hpp"

using life_lang::ast::Expr;
using life_lang::ast::FunctionCallExpr;
using life_lang::ast::Path;
using life_lang::ast::PathSegment;
using life_lang::ast::ReturnStatement;
using life_lang::ast::Statement;

PARSE_TEST(ReturnStatement)

INSTANTIATE_TEST_SUITE_P(
    , ParseReturnStatementTest,
    ::testing::Values(
        ReturnStatementTestParamsType{
            .name = "noType",
            .input = "return hello;",
            .expected =
                ReturnStatement{
                    .expr =
                        Expr{
                            Path{
                                .segments =
                                    {
                                        PathSegment{.value = "hello", .templateParameters = {}},
                                    }
                            },
                        },
                },
            .shouldSucceed = true,
            .rest = ""
        },
        ReturnStatementTestParamsType{
            .name = "noTypeMultiSegments",
            .input = "return hello.a.b;",
            .expected =
                ReturnStatement{
                    .expr =
                        Expr{
                            Path{
                                .segments =
                                    {
                                        PathSegment{.value = "hello", .templateParameters = {}},
                                        PathSegment{.value = "a", .templateParameters = {}},
                                        PathSegment{.value = "b", .templateParameters = {}},
                                    }
                            },
                        },
                },
            .shouldSucceed = true,
            .rest = ""
        },
        ReturnStatementTestParamsType{
            .name = "withType",
            .input = "return A.B.Hello<Int>.a;",
            .expected =
                ReturnStatement{
                    .expr =
                        Expr{
                            Path{
                                .segments =
                                    {
                                        PathSegment{.value = "A", .templateParameters = {}},
                                        PathSegment{.value = "B", .templateParameters = {}},
                                        PathSegment{
                                            .value = "Hello",
                                            .templateParameters =
                                                {
                                                    Path{
                                                        .segments =
                                                            {
                                                                PathSegment{.value = "Int", .templateParameters = {}},
                                                            }
                                                    },
                                                },
                                        },
                                        PathSegment{.value = "a", .templateParameters = {}},
                                    },
                            },
                        },
                },
            .shouldSucceed = true,
            .rest = ""
        },
        ReturnStatementTestParamsType{
            .name = "withTypeMultiSegments",
            .input = "return A.B.Hello<Int>.a.b.c;",
            .expected =
                ReturnStatement{
                    .expr =
                        Expr{
                            Path{
                                .segments =
                                    {
                                        PathSegment{.value = "A", .templateParameters = {}},
                                        PathSegment{.value = "B", .templateParameters = {}},
                                        PathSegment{
                                            .value = "Hello",
                                            .templateParameters =
                                                {
                                                    Path{
                                                        .segments =
                                                            {
                                                                PathSegment{.value = "Int", .templateParameters = {}},
                                                            }
                                                    },
                                                },
                                        },
                                        PathSegment{.value = "a", .templateParameters = {}},
                                        PathSegment{.value = "b", .templateParameters = {}},
                                        PathSegment{.value = "c", .templateParameters = {}},
                                    }
                            },
                        },
                },
            .shouldSucceed = true,
            .rest = ""
        },
        ReturnStatementTestParamsType{
            .name = "functionResult",
            .input = "return A.B.Hello<Int>.a.c(b);",
            .expected =
                {
                    ReturnStatement{
                        .expr =
                            Expr{
                                FunctionCallExpr{
                                    .name =
                                        Path{
                                            .segments =
                                                {
                                                    PathSegment{.value = "A", .templateParameters = {}},
                                                    PathSegment{.value = "B", .templateParameters = {}},
                                                    PathSegment{
                                                        .value = "Hello",
                                                        .templateParameters =
                                                            {
                                                                Path{
                                                                    .segments =
                                                                        {
                                                                            PathSegment{
                                                                                .value = "Int", .templateParameters = {}
                                                                            },
                                                                        }
                                                                },
                                                            },
                                                    },
                                                    PathSegment{.value = "a", .templateParameters = {}},
                                                    PathSegment{.value = "c", .templateParameters = {}},
                                                }
                                        },
                                    .parameters =
                                        {
                                            Expr{
                                                Path{
                                                    .segments =
                                                        {
                                                            PathSegment{.value = "b", .templateParameters = {}},
                                                        }
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
    [](testing::TestParamInfo<ReturnStatementTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);