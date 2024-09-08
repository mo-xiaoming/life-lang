#include "utils.hpp"

using life_lang::ast::FunctionDeclaration;
using life_lang::ast::MakeFunctionParameter;
using life_lang::ast::MakePath;
using life_lang::ast::MakePathSegment;

PARSE_TEST(FunctionDeclaration)

INSTANTIATE_TEST_SUITE_P(
    , ParseFunctionDeclarationTest,
    ::testing::Values(
        FunctionDeclarationTestParamsType{
            .name = "noArgument",
            .input = "fn foo(): Int",
            .expected = MakeFunctionDeclaration("foo", {}, MakePath("Int")),
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDeclarationTestParamsType{
            .name = "oneArgument",
            .input = "fn foo(hello:T): Int",
            .expected = MakeFunctionDeclaration(
                "foo",
                {
                    MakeFunctionParameter("hello", MakePath("T")),
                },
                MakePath("Int")
            ),
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDeclarationTestParamsType{
            .name = "multipleArguments",
            .input = "fn foo(hello:T, world:U): Int",
            .expected = MakeFunctionDeclaration(
                "foo",
                {
                    MakeFunctionParameter("hello", MakePath("T")),
                    MakeFunctionParameter("world", MakePath("U")),
                },
                MakePath("Int")
            ),
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDeclarationTestParamsType{
            .name = "oneTemplateArgument",
            .input = "fn foo(hello: A.B.Hello<Std.Array, B.C<Int, Double>>): A.B.C<Int>",
            .expected = MakeFunctionDeclaration(
                "foo",
                {
                    MakeFunctionParameter(
                        "hello", MakePath(
                                     "A", "B",
                                     MakePathSegment(
                                         "Hello",
                                         {
                                             MakePath("Std", "Array"),
                                             MakePath(
                                                 "B", MakePathSegment(
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
                },
                MakePath("A", "B", MakePathSegment("C", {MakePath("Int")}))
            ),
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<FunctionDeclarationTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);