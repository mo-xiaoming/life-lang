#include "utils.hpp"

using life_lang::ast::Block;
using life_lang::ast::MakeBlock;
using life_lang::ast::MakeExpr;
using life_lang::ast::MakeFunctionCallExpr;
using life_lang::ast::MakeFunctionCallStatement;
using life_lang::ast::MakePath;
using life_lang::ast::MakeReturnStatement;
using life_lang::ast::MakeStatement;

PARSE_TEST(Block)

INSTANTIATE_TEST_SUITE_P(
    , ParseBlockTest,
    ::testing::Values(
        BlockTestParamsType{
            .name = "emptyBlock", .input = "{}", .expected = MakeBlock({}), .shouldSucceed = true, .rest = ""
        },
        BlockTestParamsType{
            .name = "singleStatement",
            .input = "{return hello;}",
            .expected = MakeBlock({MakeStatement(MakeReturnStatement(MakeExpr(MakePath("hello"))))}),
            .shouldSucceed = true,
            .rest = ""
        },
        BlockTestParamsType{
            .name = "multipleStatements",
            .input = "{hello.a(); return world;}",
            .expected = MakeBlock(
                {MakeStatement(MakeFunctionCallStatement(MakeFunctionCallExpr(MakePath("hello", "a"), {}))),
                 MakeStatement(MakeReturnStatement(MakeExpr(MakePath("world"))))}
            ),
            .shouldSucceed = true,
            .rest = ""
        },
        BlockTestParamsType{
            .name = "nestedBlock",
            .input = "{hello(b); {return world;}}",
            .expected = MakeBlock(
                {MakeStatement(
                     MakeFunctionCallStatement(MakeFunctionCallExpr(MakePath("hello"), {MakeExpr(MakePath("b"))}))
                 ),
                 MakeStatement(MakeBlock({MakeStatement(MakeReturnStatement(MakeExpr(MakePath("world"))))}))}
            ),
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<BlockTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);