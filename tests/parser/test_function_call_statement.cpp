#include "utils.hpp"

using life_lang::ast::Expr;
using life_lang::ast::FunctionCallExpr;
using life_lang::ast::FunctionCallStatement;
using life_lang::ast::Path;
using life_lang::ast::PathSegment;

PARSE_TEST(FunctionCallStatement)

INSTANTIATE_TEST_SUITE_P(
    , ParseFunctionCallStatementTest,
    ::testing::Values(
        FunctionCallStatementTestParamsType{
            .name = "noArguments",
            .input = "hello();",
            .expected =
                FunctionCallStatement{
                    .expr =
                        FunctionCallExpr{
                            .name =
                                Path{
                                    .segments = {PathSegment{.value = "hello", .templateParameters = {}}},
                                },
                            .parameters = {}
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionCallStatementTestParamsType{
            .name = "withEverything",
            .input = "A.B<Double>.hello.world(a, b.c);",
            .expected =
                {
                    FunctionCallStatement{
                        .expr =
                            FunctionCallExpr{
                                .name =
                                    Path{
                                        .segments =
                                            {
                                                PathSegment{.value = "A", .templateParameters = {}},
                                                PathSegment{
                                                    .value = "B",
                                                    .templateParameters = {Path{
                                                        .segments =
                                                            {
                                                                PathSegment{
                                                                    .value = "Double", .templateParameters = {}
                                                                },
                                                            }
                                                    }}
                                                },
                                                PathSegment{.value = "hello", .templateParameters = {}},
                                                PathSegment{.value = "world", .templateParameters = {}},
                                            },
                                    },
                                .parameters =
                                    {
                                        Expr{
                                            Path{
                                                .segments = {PathSegment{.value = "a", .templateParameters = {}},},
                                            }
                                        },
                                        Expr{
                                            Path{
                                                .segments = {PathSegment{.value = "b", .templateParameters = {}},PathSegment{.value = "c", .templateParameters = {}},},
                                            }
                                        },
                                    },
                            }
                    }
                },
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<FunctionCallStatementTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);