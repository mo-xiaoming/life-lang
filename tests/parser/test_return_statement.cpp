#include "utils.hpp"

using life_lang::ast::MakeExpr;
using life_lang::ast::MakeFunctionCallExpr;
using life_lang::ast::MakePath;
using life_lang::ast::MakePathSegment;
using life_lang::ast::MakeReturnStatement;
using life_lang::ast::ReturnStatement;

PARSE_TEST(ReturnStatement)

INSTANTIATE_TEST_SUITE_P(
    , ParseReturnStatementTest,
    ::testing::Values(
        ReturnStatementTestParamsType{
            .name = "noType",
            .input = "return hello;",
            .expected = MakeReturnStatement(MakeExpr(MakePath("hello"))),
            .shouldSucceed = true,
            .rest = ""
        },
        ReturnStatementTestParamsType{
            .name = "noTypeMultiSegments",
            .input = "return hello.a.b;",
            .expected = MakeReturnStatement(MakeExpr(MakePath("hello", "a", "b"))),
            .shouldSucceed = true,
            .rest = ""
        },
        ReturnStatementTestParamsType{
            .name = "withType",
            .input = "return A.B.Hello<Int>.a;",
            .expected =
                MakeReturnStatement(MakeExpr(MakePath("A", "B", MakePathSegment("Hello", {MakePath("Int")}), "a"))),
            .shouldSucceed = true,
            .rest = ""
        },
        ReturnStatementTestParamsType{
            .name = "withTypeMultiSegments",
            .input = "return A.B.Hello<Int>.a.b.c;",
            .expected = MakeReturnStatement(
                MakeExpr(MakePath("A", "B", MakePathSegment("Hello", {MakePath("Int")}), "a", "b", "c"))
            ),
            .shouldSucceed = true,
            .rest = ""
        },
        ReturnStatementTestParamsType{
            .name = "functionResult",
            .input = "return A.B.Hello<Int>.a.c(b);",
            .expected = MakeReturnStatement(MakeExpr(MakeFunctionCallExpr(
                MakePath("A", "B", MakePathSegment("Hello", {MakePath("Int")}), "a", "c"), {MakeExpr(MakePath("b"))}
            ))),
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<ReturnStatementTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);