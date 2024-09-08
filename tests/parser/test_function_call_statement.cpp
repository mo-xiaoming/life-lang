#include "utils.hpp"

using life_lang::ast::FunctionCallStatement;
using life_lang::ast::MakeExpr;
using life_lang::ast::MakeFunctionCallExpr;
using life_lang::ast::MakePath;
using life_lang::ast::MakePathSegment;

PARSE_TEST(FunctionCallStatement)

INSTANTIATE_TEST_SUITE_P(
    , ParseFunctionCallStatementTest,
    ::testing::Values(
        FunctionCallStatementTestParamsType{
            .name = "noArguments",
            .input = "hello();",
            .expected = MakeFunctionCallStatement(MakeFunctionCallExpr(MakePath("hello"), {})),
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionCallStatementTestParamsType{
            .name = "withEverything",
            .input = "A.B<Double>.hello.world(a, b.c);",
            .expected = MakeFunctionCallStatement(MakeFunctionCallExpr(
                MakePath("A", MakePathSegment("B", {MakePath("Double")}), "hello", "world"),
                {
                    MakeExpr(MakePath("a")),
                    MakeExpr(MakePath("b", "c")),
                }
            )),
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<FunctionCallStatementTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);