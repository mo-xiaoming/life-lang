#include "utils.hpp"

using life_lang::ast::FunctionParameter;
using life_lang::ast::MakeFunctionParameter;
using life_lang::ast::MakePath;
using life_lang::ast::MakePathSegment;

PARSE_TEST(FunctionParameter)

INSTANTIATE_TEST_SUITE_P(
    , ParseFunctionParameterTest,
    ::testing::Values(
        FunctionParameterTestParamsType{
            .name = "noNamespace",
            .input = "hello:T",
            .expected = MakeFunctionParameter("hello", MakePath("T")),
            .shouldSucceed = true,
            .rest = "",
        },
        FunctionParameterTestParamsType{
            .name = "multipleTemplateArgument",
            .input = "hello: A.B.Hello<Std.Array, A.B.C<Int, Double>>",
            .expected = MakeFunctionParameter(
                "hello", MakePath(
                             "A", "B",
                             MakePathSegment(
                                 "Hello",
                                 {
                                     MakePath("Std", "Array"),
                                     MakePath(
                                         "A", "B",
                                         MakePathSegment(
                                             "C",
                                             {
                                                 MakePath("Int"),
                                                 MakePath("Double"),
                                             }
                                         )
                                     ),
                                 }
                             )
                         )
            ),
            .shouldSucceed = true,
            .rest = "",
        }
    ),
    [](testing::TestParamInfo<FunctionParameterTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);