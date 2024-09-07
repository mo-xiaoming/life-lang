#include "utils.hpp"

using life_lang::ast::Block;
using life_lang::ast::Expr;
using life_lang::ast::FunctionCallExpr;
using life_lang::ast::FunctionCallStatement;
using life_lang::ast::FunctionDeclaration;
using life_lang::ast::FunctionDefinition;
using life_lang::ast::FunctionParameter;
using life_lang::ast::Path;
using life_lang::ast::PathSegment;
using life_lang::ast::ReturnStatement;
using life_lang::ast::Statement;

PARSE_TEST(FunctionDefinition)

INSTANTIATE_TEST_SUITE_P(
    , ParseFunctionDefinitionTest,
    ::testing::Values(
        FunctionDefinitionTestParamsType{
            .name = "noArguments",
            .input = "fn hello(): Int {}",
            .expected =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters = {},
                            .returnType =
                                Path{
                                    .segments =
                                        {PathSegment{.value = "Int", .templateParameters = {},},},
                                }
                        },
                    .body = Block{.statements = {},},
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withArguments",
            .input = "fn hello(a: Int, b: Double): Int {}",
            .expected =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters =
                                {
                                    FunctionParameter{
                                        .name = "a",
                                        .type =
                                            Path{
                                                .segments =
                                                    {PathSegment{.value = "Int", .templateParameters = {},},},
                                            }
                                    },
                                    FunctionParameter{
                                        .name = "b",
                                        .type =
                                            Path{
                                                .segments =
                                                    {PathSegment{.value = "Double", .templateParameters = {},},},
                                            }
                                    },
                                },
                            .returnType =
                                Path{
                                    .segments =
                                        {PathSegment{.value = "Int", .templateParameters = {},},},
                                }
                        },
                    .body = Block{.statements = {},},
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withBody",
            .input = "fn hello(): Int {return world;}",
            .expected =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters = {},
                            .returnType =
                                Path{
                                    .segments =
                                        {PathSegment{.value = "Int", .templateParameters = {},},},
                                }
                        },
                    .body =
                        Block{
                            .statements = {Statement{ReturnStatement{
                                .expr = Expr{Path{
                                    .segments =
                                        {PathSegment{.value = "world", .templateParameters = {},},},
                                },},
                            },},}
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withArgumentsAndBody",
            .input = "fn hello(a: Int, b: Double): Int {return world;}",
            .expected =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters =
                                {
                                    FunctionParameter{
                                        .name = "a",
                                        .type = Path{
                                            .segments =
                                                {PathSegment{.value = "Int", .templateParameters = {},},},
                                        },
                                    },
                                    FunctionParameter{
                                        .name = "b",
                                        .type = Path{
                                            .segments =
                                                {PathSegment{.value = "Double", .templateParameters = {},},},
                                        },
                                    },
                                },
                            .returnType =
                                Path{
                                    .segments =
                                        {PathSegment{.value = "Int", .templateParameters = {},},},
                                }
                        },
                    .body =
                        Block{
                            .statements = {Statement{ReturnStatement{
                                .expr = Expr{Path{
                                    .segments =
                                        {PathSegment{.value = "world", .templateParameters = {},},},
                                },},
                            },},}
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withArgumentsAndBodyAndNestedBlock",
            .input = R"(fn hello(a: Int, b: Double): Int {
hello();
{
    return world;
}
}
)",
            .expected =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters =
                                {
                                    FunctionParameter{
                                        .name = "a",
                                        .type = Path{
                                            .segments =
                                                {PathSegment{.value = "Int", .templateParameters = {},},},
                                        },
                                    },
                                    FunctionParameter{
                                        .name = "b",
                                        .type = Path{
                                            .segments =
                                                {PathSegment{.value = "Double", .templateParameters = {},},},
                                        },
                                    },
                                },
                            .returnType =
                                Path{
                                    .segments =
                                        {PathSegment{.value = "Int", .templateParameters = {},},},
                                }
                        },
                    .body =
                        Block{
                            .statements =
                                {
                                    Statement{FunctionCallStatement{.expr = FunctionCallExpr{.name = Path{
                                        .segments = {PathSegment{.value = "hello", .templateParameters = {},},},
                                    }, .parameters = {},},},}, Statement{Block{.statements = {Statement{ReturnStatement{.expr = Expr{Path{
                                        .segments = {PathSegment{.value = "world", .templateParameters = {},},},
                                    },},},},},},}
                                }
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withArgumentsAndBodyAndNestedBlockAndNestedFunctionCall",
            .input = R"(fn hello(a: Int, b: Double): Int {
hello();
{
    return world(a);
}
})",
            .expected =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters =
                                {
                                    FunctionParameter{
                                        .name = "a",
                                        .type = Path{
                                            .segments =
                                                {PathSegment{.value = "Int", .templateParameters = {},},},
                                        },
                                    },
                                    FunctionParameter{
                                        .name = "b",
                                        .type = Path{
                                            .segments =
                                                {PathSegment{.value = "Double", .templateParameters = {},},},
                                        },
                                    },
                                },
                            .returnType =
                                Path{
                                    .segments =
                                        {PathSegment{.value = "Int", .templateParameters = {},},},
                                }
                        },
                    .body =
                        Block{
                            .statements =
                                {Statement{FunctionCallStatement{
                                        .expr = FunctionCallExpr{.name = Path{
                                            .segments = {PathSegment{.value = "hello", .templateParameters = {},},},
                                        }, .parameters = {},},
                                    },},
                                    Statement{Block{
                                        .statements = {Statement{ReturnStatement{
                                            .expr = Expr{FunctionCallExpr{
                                                .name =
                                                    Path{
                                                        .segments = {PathSegment{.value = "world", .templateParameters = {},},},
                                                    },
                                                .parameters = {Expr{Path{
                                                    .segments =
                                                        {PathSegment{.value = "a", .templateParameters = {},},},
                                                },},}
                                            },},
                                        },},}
                                    },},}
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "nestedFunctionDefinition",
            .input = R"(fn hello(a: Int, b: Double): Int {
fn world(): Int {
    hello();
}
return world();
}
)",
            .expected =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters =
                                {
                                    FunctionParameter{
                                        .name = "a",
                                        .type = Path{.segments = {PathSegment{.value = "Int", .templateParameters = {},},},},
                                    },
                                    FunctionParameter{
                                        .name = "b",
                                        .type = Path{.segments = {PathSegment{.value = "Double", .templateParameters = {},},},},
                                    },
                                },
                            .returnType =
                                Path{
                                    .segments =
                                        {PathSegment{.value = "Int", .templateParameters = {},},},
                                }
                        },
                    .body =
                        Block{
                            .statements =
                                {Statement{
                                        FunctionDefinition{
                                            .declaration =
                                                FunctionDeclaration{
                                                    .name = "world",
                                                    .parameters = {},
                                                    .returnType =
                                                        Path{
                                                            .segments =
                                                                {PathSegment{.value = "Int", .templateParameters = {},},},
                                                        }
                                                },
                                            .body =
                                                Block{
                                                    .statements =
                                                        {
                                                            Statement{FunctionCallStatement{.expr = FunctionCallExpr{.name = Path{
                                                                .segments = {PathSegment{.value = "hello", .templateParameters = {},},},
                                                            }, .parameters = {},},},}
                                                        }
                                                }
                                        }
                                    },
                                    Statement{ReturnStatement{
                                        .expr = Expr{FunctionCallExpr{
                                            .name =
                                                Path{.segments = {PathSegment{.value = "world", .templateParameters = {},},},},
                                            .parameters = {}
                                        },},
                                    },},}
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "helloWorld",
            .input = R"(
fn main(args: Std.Array<Std.String>): I32 {
    Std.print(args);
    return args.size();
}
)",
            .expected =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "main",
                            .parameters =
                                {
                                    FunctionParameter{
                                        .name = "args",
                                        .type =
                                            Path{
                                                .segments =
                                                    {
                                                        PathSegment{.value="Std", .templateParameters = {}},
                                                        PathSegment{.value = "Array", .templateParameters = {Path{
                                                            .segments = {
                                                                PathSegment{.value = "Std", .templateParameters = {}},
                                                                PathSegment{.value = "String", .templateParameters = {}},
                                                            },
                                                        },},},
                                                    },
                                            }
                                    },
                                },
                            .returnType =
                                Path{
                                    .segments =
                                        {PathSegment{.value = "I32", .templateParameters = {},},},
                                }
                        },
                        .body = Block{
                            .statements =
                                {
                                    Statement{FunctionCallStatement{
                                        .expr = FunctionCallExpr{
                                            .name = Path{
                                                .segments = {
                                                    PathSegment{.value = "Std", .templateParameters = {}},
                                                    PathSegment{.value = "print", .templateParameters = {}},
                                                },
                                            },
                                            .parameters = {Expr{Path{
                                                .segments = {
                                                    PathSegment{.value = "args", .templateParameters = {},},
                                                },
                                            },},},
                                        },
                                    },},
                                    Statement{ReturnStatement{
                                        .expr = Expr{FunctionCallExpr{
                                            .name = Path{
                                                .segments = {
                                                    PathSegment{.value = "args", .templateParameters = {}},
                                                    PathSegment{.value = "size", .templateParameters = {}},
                                                },
                                            },
                                            .parameters = {},
                                        },},
                                    },},
                                },
                        },
                },
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<FunctionDefinitionTestParamsType> const& paramInfo) {
        return std::string{paramInfo.param.name};
    }
);