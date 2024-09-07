#include "utils.hpp"

using life_lang::ast::Expr;
using life_lang::ast::FunctionParameter;
using life_lang::ast::Path;
using life_lang::ast::PathSegment;
using life_lang::ast::Statement;

PARSE_TEST(FunctionParameter)

INSTANTIATE_TEST_SUITE_P(
    , ParseFunctionParameterTest,
    ::testing::Values(
        FunctionParameterTestParamsType{
            .name = "noNamespace",
            .input = "hello:T",
            .expected =
                FunctionParameter{
                    .name = "hello",
                    .type =
                        Path{
                            .segments =
                                {
                                    PathSegment{.value = "T", .templateParameters = {}},
                                },
                        }
                },
            .shouldSucceed = true,
            .rest = "",
        },
        FunctionParameterTestParamsType{
            .name = "multipleTemplateArgument",
            .input = "hello: A.B.Hello<Std.Array, A.B.C<Int, Double>>",
            .expected =
                FunctionParameter{
                    .name = "hello",
                    .type =
                        Path{
                            .segments =
                                {
                                    PathSegment{.value = "A", .templateParameters = {}},
                                    PathSegment{.value = "B", .templateParameters = {}},
                                    PathSegment{
                                        .value = "Hello",
                                        .templateParameters =
                                            {
                                                Path{
                                                    .segments =
                                                        {
                                                            PathSegment{.value = "Std", .templateParameters = {}},
                                                            PathSegment{.value = "Array", .templateParameters = {}},
                                                        }
                                                },
                                                Path{
                                                    .segments =
                                                        {
                                                            PathSegment{.value = "A", .templateParameters = {}},
                                                            PathSegment{.value = "B", .templateParameters = {}},
                                                            PathSegment{
                                                                .value = "C",
                                                                .templateParameters =
                                                                    {
                                                                        Path{
                                                                            .segments =
                                                                                {
                                                                                    PathSegment{
                                                                                        .value = "Int",
                                                                                        .templateParameters = {}
                                                                                    },
                                                                                }
                                                                        },
                                                                        Path{
                                                                            .segments =
                                                                                {
                                                                                    PathSegment{
                                                                                        .value = "Double",
                                                                                        .templateParameters = {}
                                                                                    },
                                                                                }
                                                                        },
                                                                    },
                                                            },
                                                        },
                                                },
                                            },
                                    },
                                },
                        }
                },
            .shouldSucceed = true,
            .rest = "",
        }
    ),
    [](testing::TestParamInfo<FunctionParameterTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);