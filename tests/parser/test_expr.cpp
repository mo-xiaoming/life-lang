#include "utils.hpp"

using life_lang::ast::Expr;
using life_lang::ast::MakeFunctionCallExpr;
using life_lang::ast::MakePath;
using life_lang::ast::MakePathSegment;
using life_lang::ast::MakeStatement;

PARSE_TEST(Expr)

INSTANTIATE_TEST_SUITE_P(
    , ParseExprTest,
    ::testing::Values(
        ExprTestParamsType{
            .name = "functionCall",
            .input = "hello()",
            .expected = MakeExpr(MakeFunctionCallExpr(MakePath("hello"), {})),
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithDataPath",
            .input = "hello.a.b()",
            .expected = MakeExpr(MakeFunctionCallExpr(MakePath("hello", "a", "b"), {})),
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithTypePath",
            .input = "A.B.hello()",
            .expected = MakeExpr(MakeFunctionCallExpr(MakePath("A", "B", "hello"), {})),
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithTypeAndDataPath",
            .input = "A.B.hello.a.b()",
            .expected = MakeExpr(MakeFunctionCallExpr(MakePath("A", "B", "hello", "a", "b"), {})),
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithEverything",
            .input = "A.B<Int, Double>.hello.a.b()",
            .expected = MakeExpr(MakeFunctionCallExpr(
                MakePath(
                    "A",
                    MakePathSegment(
                        "B",
                        {
                            MakePath("Int"),
                            MakePath("Double"),
                        }
                    ),
                    "hello", "a", "b"
                ),
                {}
            )),
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithArguments",
            .input = "hello(a, b, c)",
            .expected = MakeExpr(MakeFunctionCallExpr(
                MakePath("hello"),
                {
                    MakeExpr(MakePath("a")),
                    MakeExpr(MakePath("b")),
                    MakeExpr(MakePath("c")),
                }
            )),
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithArgumentsHavePaths",
            .input = "hello(a, b.c.world, c.world)",
            .expected = MakeExpr(MakeFunctionCallExpr(
                MakePath("hello"),
                {
                    MakeExpr(MakePath("a")),
                    MakeExpr(MakePath("b", "c", "world")),
                    MakeExpr(MakePath("c", "world")),
                }
            )),
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithArgumentIsFunctionCall",
            .input = "hello(A.B.a.d(), c.world(a))",
            .expected = MakeExpr(MakeFunctionCallExpr(
                MakePath("hello"),
                {
                    MakeExpr(MakeFunctionCallExpr(MakePath("A", "B", "a", "d"), {})),
                    MakeExpr(MakeFunctionCallExpr(MakePath("c", "world"), {MakeExpr(MakePath("a"))})),
                }
            )),
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<ExprTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);