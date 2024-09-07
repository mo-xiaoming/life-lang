#include "utils.hpp"

using life_lang::ast::FunctionDeclaration;
using life_lang::ast::FunctionParameter;
using life_lang::ast::Path;
using life_lang::ast::PathSegment;

PARSE_TEST(FunctionDeclaration)

INSTANTIATE_TEST_SUITE_P(
    , ParseFunctionDeclarationTest,
    ::testing::Values(
        FunctionDeclarationTestParamsType{
            .name = "noArgument",
            .input = "fn foo(): Int",
            .expected = {FunctionDeclaration{
                .name = "foo",
                .parameters = {},
                .returnType = Path{.segments = {PathSegment{.value = "Int", .templateParameters = {},},},},
            }},
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDeclarationTestParamsType{
            .name = "oneArgument",
            .input = "fn foo(hello:T): Int",
            .expected =
                {
                    FunctionDeclaration{
                        .name = "foo",
                        .parameters =
                            {
                                FunctionParameter{
                                    .name = "hello",
                                    .type = Path{.segments = {PathSegment{.value = "T", .templateParameters = {},},},},
                                },
                            },
                        .returnType = Path{.segments = {PathSegment{.value = "Int", .templateParameters = {},},},},
                    },
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDeclarationTestParamsType{
            .name = "multipleArguments",
            .input = "fn foo(hello:T, world:U): Int",
            .expected =
                {
                    FunctionDeclaration{
                        .name = "foo",
                        .parameters =
                            {
                                FunctionParameter{
                                    .name = "hello",
                                    .type = Path{.segments = {PathSegment{.value = "T", .templateParameters = {},},},},
                                },
                                FunctionParameter{
                                    .name = "world",
                                    .type = Path{.segments = {PathSegment{.value = "U", .templateParameters = {},},},},
                                },
                            },
                        .returnType = Path{.segments = {PathSegment{.value = "Int", .templateParameters = {},},},},
                    },
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDeclarationTestParamsType{
            .name = "oneTemplateArgument",
            .input = "fn foo(hello: A.B.Hello<Std.Array, B.C<Int, Double>>): A.B.C<Int>",
            .expected =
                {
                    FunctionDeclaration{
                        .name = "foo",
                        .parameters =
                            {
                                FunctionParameter{
                                    .name = "hello",
                                    .type =
                                        Path{
                                            .segments =
                                                {
                                                    PathSegment{
                                                     .value = "A",
                                                     .templateParameters = {},
                                                 },
                                                 PathSegment{
                                                     .value = "B",
                                                     .templateParameters = {},
                                                 },
                                                 PathSegment{
                                                     .value = "Hello",
                                                     .templateParameters =
                                                        {Path{
                                                              .segments =
                                                                  {PathSegment{
                                                                       .value = "Std", .templateParameters = {},
                                                                   },
                                                                   PathSegment{
                                                                       .value = "Array", .templateParameters = {},
                                                                   },},
                                                          },
                                                          Path{
                                                                .segments = {
                                                                    PathSegment{.value = "B", .templateParameters = {}},
                                                                    PathSegment{
                                                                        .value = "C",
                                                                        .templateParameters = {
                                                                            Path{.segments = {PathSegment{.value = "Int", .templateParameters = {},},},},
                                                                            Path{.segments = {PathSegment{.value = "Double", .templateParameters = {},},},},
                                                                        },
                                                                    },
                                                                },
                                                            },
                                                        },
                                                 },
                                            },
                                        },
                                },
                            },
                        .returnType =
                            Path{
                                .segments =
                                    {PathSegment{.value = "A", .templateParameters = {},},
                                     PathSegment{.value = "B", .templateParameters = {},},
                                     PathSegment{
                                         .value = "C",
                                         .templateParameters =
                                             {Path{.segments = {PathSegment{.value = "Int", .templateParameters = {},},},},}
                                     },},
                            },
                    },
                },
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<FunctionDeclarationTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);