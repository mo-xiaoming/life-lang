
#include "utils.hpp"

using life_lang::ast::MakeString;
using life_lang::ast::String;

PARSE_TEST(String)

INSTANTIATE_TEST_SUITE_P(
    , ParseStringTest,
    ::testing::Values(
        StringTestParamsType{
            .name = "all",
            .input = R"("abc\"d\n\x00yz")",
            .expected = MakeString(R"("abc\"d\n\x00yz")"),
            .shouldSucceed = true,
            .rest = ""
        },
        StringTestParamsType{
            .name = "empty", .input = R"("")", .expected = MakeString(R"("")"), .shouldSucceed = true, .rest = ""
        }
    ),
    [](testing::TestParamInfo<StringTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);