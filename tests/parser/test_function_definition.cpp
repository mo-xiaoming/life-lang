#include "utils.hpp"

using life_lang::ast::FunctionDefinition;
using life_lang::ast::MakeBlock;
using life_lang::ast::MakeFunctionDeclaration;
using life_lang::ast::MakeFunctionDefinition;
using life_lang::ast::MakeFunctionParameter;
using life_lang::ast::MakeInteger;
using life_lang::ast::MakePath;
using life_lang::ast::MakePathSegment;
using life_lang::ast::MakeString;

PARSE_TEST(FunctionDefinition)

INSTANTIATE_TEST_SUITE_P(
    , ParseFunctionDefinitionTest,
    ::testing::Values(
        FunctionDefinitionTestParamsType{
            .name = "noArguments",
            .input = "fn hello(): Int {}",
            .expected = MakeFunctionDefinition(MakeFunctionDeclaration("hello", {}, MakePath("Int")), MakeBlock({})),
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withArguments",
            .input = "fn hello(a: Int, b: Double): Int {}",
            .expected = MakeFunctionDefinition(
                MakeFunctionDeclaration(
                    "hello",
                    {
                        MakeFunctionParameter("a", MakePath("Int")),
                        MakeFunctionParameter("b", MakePath("Double")),
                    },
                    MakePath("Int")
                ),
                MakeBlock({})
            ),
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withBody",
            .input = "fn hello(): Int {return world;}",
            .expected = MakeFunctionDefinition(
                MakeFunctionDeclaration("hello", {}, MakePath("Int")),
                MakeBlock({MakeStatement(MakeReturnStatement(MakeExpr(MakePath("world"))))})
            ),
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withArgumentsAndBody",
            .input = "fn hello(a: Int, b: Double): Int {return world;}",
            .expected = MakeFunctionDefinition(
                MakeFunctionDeclaration(
                    "hello",
                    {MakeFunctionParameter("a", MakePath("Int")), MakeFunctionParameter("b", MakePath("Double"))},
                    MakePath("Int")
                ),
                MakeBlock({MakeStatement(MakeReturnStatement(MakeExpr(MakePath("world"))))})
            ),
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
            .expected = MakeFunctionDefinition(
                MakeFunctionDeclaration(
                    "hello",
                    {
                        MakeFunctionParameter("a", MakePath("Int")),
                        MakeFunctionParameter("b", MakePath("Double")),
                    },
                    MakePath("Int")
                ),
                MakeBlock({
                    MakeStatement(MakeFunctionCallStatement(MakeFunctionCallExpr(MakePath("hello"), {}))),
                    MakeStatement(MakeBlock({
                        MakeStatement(MakeReturnStatement(MakeExpr(MakePath("world")))),
                    })),
                })
            ),
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withArgumentsAndBodyAndNestedBlockAndNestedFunctionCall",
            .input = R"(fn hello(a: Int, b: Double): Int {
hello();
{
    return world();
}
}
)",
            .expected = MakeFunctionDefinition(
                MakeFunctionDeclaration(
                    "hello",
                    {
                        MakeFunctionParameter("a", MakePath("Int")),
                        MakeFunctionParameter("b", MakePath("Double")),
                    },
                    MakePath("Int")
                ),
                MakeBlock(
                    {MakeStatement(MakeFunctionCallStatement(MakeFunctionCallExpr(MakePath("hello"), {}))),
                     MakeStatement(MakeBlock({
                         MakeStatement(MakeReturnStatement(MakeExpr(MakeFunctionCallExpr(MakePath("world"), {})))),
                     }))}
                )
            ),
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
            .expected = MakeFunctionDefinition(
                MakeFunctionDeclaration(
                    "hello",
                    {
                        MakeFunctionParameter("a", MakePath("Int")),
                        MakeFunctionParameter("b", MakePath("Double")),
                    },
                    MakePath("Int")
                ),
                MakeBlock({
                    MakeStatement(MakeFunctionDefinition(
                        MakeFunctionDeclaration("world", {}, MakePath("Int")),
                        MakeBlock({MakeStatement(MakeFunctionCallStatement(MakeFunctionCallExpr(MakePath("hello"), {})))
                        })
                    )),
                    MakeStatement(MakeReturnStatement(MakeExpr(MakeFunctionCallExpr(MakePath("world"), {})))),
                })
            ),
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "complexFunctionDefinition",
            .input = R"(fn complex(a: Int, b: Double): Int {
fn nested(): Double {
    return b;
}
return a;
}
)",
            .expected = MakeFunctionDefinition(
                MakeFunctionDeclaration(
                    "complex",
                    {
                        MakeFunctionParameter("a", MakePath("Int")),
                        MakeFunctionParameter("b", MakePath("Double")),
                    },
                    MakePath("Int")
                ),
                MakeBlock({
                    MakeStatement(MakeFunctionDefinition(
                        MakeFunctionDeclaration("nested", {}, MakePath("Double")),
                        MakeBlock({
                            MakeStatement(MakeReturnStatement(MakeExpr(MakePath("b")))),
                        })
                    )),
                    MakeStatement(MakeReturnStatement(MakeExpr(MakePath(MakePathSegment("a"))))),
                })
            ),
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "helloWorld",
            .input = R"(
fn main(args: Std.Array<Std.String>): I32 {
    Std.print("Hello, world!");
    return 0;
}
)",
            .expected = MakeFunctionDefinition(
                MakeFunctionDeclaration(
                    "main",
                    {
                        MakeFunctionParameter(
                            "args", MakePath("Std", MakePathSegment("Array", {MakePath("Std", "String")}))
                        ),
                    },
                    MakePath("I32")
                ),
                MakeBlock(
                    {MakeStatement(MakeFunctionCallStatement(
                         MakeFunctionCallExpr(MakePath("Std", "print"), {MakeExpr(MakeString("\"Hello, world!\""))})
                     )),
                     MakeStatement(MakeReturnStatement(MakeExpr(MakeInteger("0"))))}
                )
            ),
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<FunctionDefinitionTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);