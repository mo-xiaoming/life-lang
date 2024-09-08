#include "utils.hpp"

using life_lang::ast::MakePath;
using life_lang::ast::MakePathSegment;
using life_lang::ast::Path;
using life_lang::ast::Statement;

PARSE_TEST(Path)

INSTANTIATE_TEST_SUITE_P(
    , ParsePathTest,
    ::testing::Values(PathTestParamsType{
        .name = "all",
        .input = "A.B.World<Int<e>, Double.c>.Hi.a.b",
        .expected = MakePath(
            "A", "B",
            MakePathSegment(
                "World",
                {
                    MakePath(MakePathSegment("Int", {MakePath("e")})),
                    MakePath("Double", "c"),
                }
            ),
            "Hi", "a", "b"
        ),
        .shouldSucceed = true,
        .rest = ""
    }),

    [](testing::TestParamInfo<PathTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);