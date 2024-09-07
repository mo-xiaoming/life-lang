#include "utils.hpp"

using life_lang::ast::Block;
using life_lang::ast::Expr;
using life_lang::ast::FunctionCallExpr;
using life_lang::ast::FunctionCallStatement;
using life_lang::ast::Path;
using life_lang::ast::PathSegment;
using life_lang::ast::ReturnStatement;
using life_lang::ast::Statement;

PARSE_TEST(Block)

INSTANTIATE_TEST_SUITE_P(
    , ParseBlockTest,
    ::testing::Values(
        BlockTestParamsType{
            .name = "emptyBlock",
            .input = "{}",
            .expected =
                Block{
                    .statements = {},
                },
            .shouldSucceed = true,
            .rest = ""
        },
        BlockTestParamsType{
            .name = "singleStatement",
            .input = "{return hello;}",
            .expected =
                Block{
                    .statements =
                        {
                            Statement{
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
                            },
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        },
        BlockTestParamsType{
            .name = "multipleStatements",
            .input = "{hello.a(); return world;}",
            .expected =
                Block{
                    .statements =
                        {
                            Statement{FunctionCallStatement{
                                .expr =
                                    FunctionCallExpr{
                                        .name =
                                            Path{
                                                .segments =
                                                    {
                                                        PathSegment{.value = "hello", .templateParameters = {}},
                                                        PathSegment{.value = "a", .templateParameters = {}},
                                                    },
                                            },
                                        .parameters = {}
                                    }
                            }},
                            Statement{
                                ReturnStatement{
                                    .expr =
                                        Expr{
                                            Path{
                                                .segments =
                                                    {
                                                        PathSegment{.value = "world", .templateParameters = {}},
                                                    },
                                            },
                                        },
                                },
                            },
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        },
        BlockTestParamsType{
            .name = "nestedBlock",
            .input = "{hello(b); {return world;}}",
            .expected =
                Block{
                    .statements =
                        {Statement{
                             FunctionCallStatement{
                                 .expr =
                                     FunctionCallExpr{
                                         .name =
                                             Path{
                                                 .segments = {PathSegment{.value = "hello", .templateParameters = {}}}
                                             },
                                         .parameters =
                                             {
                                                 Expr{
                                                     Path{
                                                         .segments = {PathSegment{.value = "b", .templateParameters = {}}}
                                                     }
                                                 }
                                             }
                                     }
                             }
                         },
                         Statement{
                             Block{
                                 .statements =
                                     {
                                         Statement{
                                             ReturnStatement{
                                                 .expr = Expr{Path{.segments = {PathSegment{.value = "world", .templateParameters = {}}}}}
                                             }
                                         }
                                     }
                             }
                         }},
                },
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<BlockTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);