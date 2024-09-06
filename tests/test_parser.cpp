#include <fmt/core.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>

#include <string_view>

#include "rules.hpp"

using life_lang::ast::Block;
using life_lang::ast::DataPath;
using life_lang::ast::DataPathSegment;
using life_lang::ast::Expr;
using life_lang::ast::FunctionCallExpr;
using life_lang::ast::FunctionCallExprStatement;
using life_lang::ast::FunctionDeclaration;
using life_lang::ast::FunctionDefinition;
using life_lang::ast::FunctionParameter;
using life_lang::ast::ModulePath;
using life_lang::ast::ModulePathSegment;
using life_lang::ast::ReturnStatement;
using life_lang::ast::Statement;
using life_lang::ast::Type;
using life_lang::ast::Value;

#define PARSE_TEST(name)                                                                         \
  using name##TestParamsType = ParseTestParams<name>;                                            \
  class Parse##name##Test : public ::testing::TestWithParam<name##TestParamsType> {};            \
                                                                                                 \
  TEST_P(Parse##name##Test, Parse##name) {                                                       \
    auto const& params = GetParam();                                                             \
    auto inputStart = params.input.cbegin();                                                     \
    auto const inputEnd = params.input.cend();                                                   \
    std::ostringstream errorMsg;                                                                 \
    auto const ret = life_lang::internal::Parse##name(inputStart, inputEnd, errorMsg);           \
    EXPECT_EQ(params.shouldSucceed, bool(ret)) << (ret ? fmt::to_string(*ret) : errorMsg.str()); \
    auto const rest = std::string_view{inputStart, inputEnd};                                    \
    EXPECT_EQ(params.rest, rest) << fmt::format(">{}< != >{}<", params.rest, rest);              \
    EXPECT_EQ(params.expectedValue, *ret);                                                       \
  }

template <typename AstType>
struct ParseTestParams {
  std::string_view name;
  std::string input;
  AstType expectedValue;
  bool shouldSucceed{};
  std::string_view rest;
};  // namespace std::ranges

PARSE_TEST(ModulePathSegment)

INSTANTIATE_TEST_SUITE_P(
    , ParseModulePathSegmentTest,
    ::testing::Values(
        ModulePathSegmentTestParamsType{
            .name = "H",
            .input = "H",
            .expectedValue = ModulePathSegment{.value = "H"},
            .shouldSucceed = true,
            .rest = ""
        },
        ModulePathSegmentTestParamsType{
            .name = "Hello",
            .input = "Hello",
            .expectedValue = ModulePathSegment{.value = "Hello"},
            .shouldSucceed = true,
            .rest = ""
        },
        ModulePathSegmentTestParamsType{
            .name = "HELLO",
            .input = "HELLO",
            .expectedValue = ModulePathSegment{.value = "HELLO"},
            .shouldSucceed = true,
            .rest = ""
        },
        ModulePathSegmentTestParamsType{
            .name = "H340",
            .input = "H340",
            .expectedValue = ModulePathSegment{.value = "H340"},
            .shouldSucceed = true,
            .rest = ""
        },
        ModulePathSegmentTestParamsType{
            .name = "HelloWorld",
            .input = "HelloWorld",
            .expectedValue = ModulePathSegment{.value = "HelloWorld"},
            .shouldSucceed = true,
            .rest = ""
        },
        ModulePathSegmentTestParamsType{
            .name = "unsupportedStartsWithNumber",
            .input = "0abc",
            .expectedValue = ModulePathSegment{.value = ""},
            .shouldSucceed = false,
            .rest = "0abc"
        },
        ModulePathSegmentTestParamsType{
            .name = "unsupportedStartsWithLowercase",
            .input = "hello",
            .expectedValue = ModulePathSegment{.value = ""},
            .shouldSucceed = false,
            .rest = "hello"
        },
        ModulePathSegmentTestParamsType{
            .name = "unsupportedStartsWithUnderscore",
            .input = "_Hello",
            .expectedValue = ModulePathSegment{.value = ""},
            .shouldSucceed = false,
            .rest = "_Hello"
        }
    ),
    [](testing::TestParamInfo<ModulePathSegmentTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);

PARSE_TEST(ModulePath)

INSTANTIATE_TEST_SUITE_P(
    , ParseModulePathTest,
    ::testing::Values(
        ModulePathTestParamsType{
            .name = "singleSegment",
            .input = "Hello",
            .expectedValue = ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("Hello")}},
            .shouldSucceed = true,
            .rest = {}
        },
        ModulePathTestParamsType{
            .name = "multiSegments",
            .input = "HELLO.World",
            .expectedValue =
                ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("HELLO"), ModulePathSegment("World")}},
            .shouldSucceed = true,
            .rest = {}
        },
        ModulePathTestParamsType{
            .name = "relativeSegment",
            .input = ".H340",
            .expectedValue = ModulePath{.isAbsolute = false, .segments = {ModulePathSegment("H340")}},
            .shouldSucceed = true,
            .rest = {}
        },
        ModulePathTestParamsType{
            .name = "relativeMultiSegments",
            .input = ".H340.HelloWorld",
            .expectedValue =
                ModulePath{
                    .isAbsolute = false, .segments = {ModulePathSegment("H340"), ModulePathSegment("HelloWorld")}
                },
            .shouldSucceed = true,
            .rest = {}
        },
        ModulePathTestParamsType{
            .name = "unsupportedDoubleDots",
            .input = "..HelloWorld",
            .expectedValue = ModulePath{},
            .shouldSucceed = false,
            .rest = "..HelloWorld"
        },
        ModulePathTestParamsType{
            .name = "unsupportedRelativeSegments",
            .input = ".Hello.0abc",
            .expectedValue = ModulePath{.isAbsolute = false, .segments = {ModulePathSegment("Hello")}},
            .shouldSucceed = true,
            .rest = ".0abc"
        },
        ModulePathTestParamsType{
            .name = "unsupportedSegments",
            .input = "Hello.0abc",
            .expectedValue = ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("Hello")}},
            .shouldSucceed = true,
            .rest = ".0abc"
        },
        ModulePathTestParamsType{
            .name = "unsupportedRelativeLowercase",
            .input = ".h340",
            .expectedValue = ModulePath{},
            .shouldSucceed = false,
            .rest = ".h340"
        }
    ),
    [](testing::TestParamInfo<ModulePathTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);

PARSE_TEST(DataPathSegment)

INSTANTIATE_TEST_SUITE_P(
    , ParseDataPathSegmentTest,
    ::testing::Values(
        DataPathSegmentTestParamsType{
            .name = "h", .input = "h", .expectedValue = DataPathSegment{.value = "h"}, .shouldSucceed = true, .rest = ""
        },
        DataPathSegmentTestParamsType{
            .name = "hello",
            .input = "hello",
            .expectedValue = DataPathSegment{.value = "hello"},
            .shouldSucceed = true,
            .rest = ""
        },
        DataPathSegmentTestParamsType{
            .name = "h340",
            .input = "h340",
            .expectedValue = DataPathSegment{.value = "h340"},
            .shouldSucceed = true,
            .rest = ""
        },
        DataPathSegmentTestParamsType{
            .name = "hello_world",
            .input = "hello_world",
            .expectedValue = DataPathSegment{.value = "hello_world"},
            .shouldSucceed = true,
            .rest = ""
        },
        DataPathSegmentTestParamsType{
            .name = "endsWithUnderscore",
            .input = "hello_",
            .expectedValue = DataPathSegment{.value = "hello_"},
            .shouldSucceed = true,
            .rest = ""
        },
        DataPathSegmentTestParamsType{
            .name = "unsupportedStartsWithNumber",
            .input = "0abc",
            .expectedValue = DataPathSegment{.value = ""},
            .shouldSucceed = false,
            .rest = "0abc"
        },
        DataPathSegmentTestParamsType{
            .name = "unsupportedStartsWithUppercase",
            .input = "Hello",
            .expectedValue = DataPathSegment{.value = ""},
            .shouldSucceed = false,
            .rest = "Hello"
        },
        DataPathSegmentTestParamsType{
            .name = "unsupportedStartsWithUnderscore",
            .input = "_hello",
            .expectedValue = DataPathSegment{.value = ""},
            .shouldSucceed = false,
            .rest = "_hello"
        },
        DataPathSegmentTestParamsType{
            .name = "unsupportedEndsWithUnderscore",
            .input = "hello_Abc",
            .expectedValue = DataPathSegment{.value = ""},
            .shouldSucceed = false,
            .rest = "hello_Abc"
        }
    ),
    [](testing::TestParamInfo<DataPathSegmentTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);

PARSE_TEST(DataPath)

INSTANTIATE_TEST_SUITE_P(
    , ParseDataPathTest,
    ::testing::Values(
        DataPathTestParamsType{
            .name = "singleSegment",
            .input = "hello",
            .expectedValue = DataPath{.segments = {DataPathSegment("hello")}},
            .shouldSucceed = true,
            .rest = {}
        },
        DataPathTestParamsType{
            .name = "multiSegments",
            .input = "hello.world",
            .expectedValue = DataPath{.segments = {DataPathSegment("hello"), DataPathSegment("world")}},
            .shouldSucceed = true,
            .rest = {}
        },
        DataPathTestParamsType{
            .name = "withNumberSegment",
            .input = "h340",
            .expectedValue = DataPath{.segments = {DataPathSegment("h340")}},
            .shouldSucceed = true,
            .rest = {}
        },
        DataPathTestParamsType{
            .name = "withNumberMultiSegments",
            .input = "h340.hello_world",
            .expectedValue = DataPath{.segments = {DataPathSegment("h340"), DataPathSegment("hello_world")}},
            .shouldSucceed = true,
            .rest = {}
        },
        DataPathTestParamsType{
            .name = "secondSegmentIsInvalid",
            .input = "hello.0abc",
            .expectedValue = DataPath{.segments = {DataPathSegment("hello")}},
            .shouldSucceed = true,
            .rest = ".0abc"
        },
        DataPathTestParamsType{
            .name = "unsupportedStartingDots",
            .input = ".hello",
            .expectedValue = DataPath{},
            .shouldSucceed = false,
            .rest = ".hello"
        },
        DataPathTestParamsType{
            .name = "unsupportedRelativeLowercase",
            .input = ".h340",
            .expectedValue = DataPath{},
            .shouldSucceed = false,
            .rest = ".h340"
        }
    ),
    [](testing::TestParamInfo<DataPathTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);

PARSE_TEST(Type)

INSTANTIATE_TEST_SUITE_P(
    , ParseTypeTest,
    ::testing::Values(
        TypeTestParamsType{
            .name = "noModuleNoTemplate",
            .input = "Hello",
            .expectedValue =
                Type{
                    .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                    .name = "Hello",
                    .templateParameters = {}
                },
            .shouldSucceed = true,
            .rest = {}
        },
        TypeTestParamsType{
            .name = "withModuleNoTemplate",
            .input = "A.B.Hello",
            .expectedValue =
                Type{
                    .modulePath =
                        ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("A"), ModulePathSegment("B")}},
                    .name = "Hello",
                    .templateParameters = {}
                },
            .shouldSucceed = true,
            .rest = {}
        },
        TypeTestParamsType{
            .name = "withRelativeModuleNoTemplate",
            .input = ".A.B.Hello",
            .expectedValue =
                Type{
                    .modulePath =
                        ModulePath{.isAbsolute = false, .segments = {ModulePathSegment("A"), ModulePathSegment("B")}},
                    .name = "Hello",
                    .templateParameters = {}
                },
            .shouldSucceed = true,
            .rest = {}
        },
        TypeTestParamsType{
            .name = "noModuleWithTemplate",
            .input = "Hello<Int>",
            .expectedValue =
                Type{
                    .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                    .name = "Hello",
                    .templateParameters = {Type{
                        .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                        .name = "Int",
                        .templateParameters = {}
                    }}
                },
            .shouldSucceed = true,
            .rest = {}
        },
        TypeTestParamsType{
            .name = "withModuleWithTemplate",
            .input = "A.B.Hello<Int>",
            .expectedValue =
                Type{
                    .modulePath =
                        ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("A"), ModulePathSegment("B")}},
                    .name = "Hello",
                    .templateParameters = {Type{
                        .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                        .name = "Int",
                        .templateParameters = {}
                    }}
                },
            .shouldSucceed = true,
            .rest = {}
        },
        TypeTestParamsType{
            .name = "withRelativeModuleWithTemplate",
            .input = ".A.B.Hello<Int>",
            .expectedValue =
                Type{
                    .modulePath =
                        ModulePath{.isAbsolute = false, .segments = {ModulePathSegment("A"), ModulePathSegment("B")}},
                    .name = "Hello",
                    .templateParameters = {Type{
                        .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                        .name = "Int",
                        .templateParameters = {}
                    }}
                },
            .shouldSucceed = true,
            .rest = {}
        },
        TypeTestParamsType{
            .name = "withModuleWithTemplateHasModulePath",
            .input = "A.Hello<.C.Int, Std.Array<Math.Double>>",
            .expectedValue =
                Type{
                    .modulePath = ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("A")}},
                    .name = "Hello",
                    .templateParameters =
                        {Type{
                             .modulePath = ModulePath{.isAbsolute = false, .segments = {ModulePathSegment("C")}},
                             .name = "Int",
                             .templateParameters = {}
                         },
                         Type{
                             .modulePath = ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("Std")}},
                             .name = "Array",
                             .templateParameters = {Type{
                                 .modulePath = ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("Math")}},
                                 .name = "Double",
                                 .templateParameters = {}
                             }}
                         }}
                },
            .shouldSucceed = true,
            .rest = {}
        }
    ),
    [](testing::TestParamInfo<TypeTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);

PARSE_TEST(FunctionParameter)

INSTANTIATE_TEST_SUITE_P(
    , ParseFunctionParameterTest,
    ::testing::Values(
        FunctionParameterTestParamsType{
            .name = "noNamespace",
            .input = "hello:T",
            .expectedValue =
                FunctionParameter{
                    .name = "hello",
                    .type =
                        Type{
                            .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                            .name = "T",
                            .templateParameters = {}
                        }
                },
            .shouldSucceed = true,
            .rest = "",
        },
        FunctionParameterTestParamsType{
            .name = "multipleTemplateArgument",
            .input = "hello: A.B.Hello<Std.Array, A.B.C<Int, Double>>",
            .expectedValue =
                FunctionParameter{
                    .name = "hello",
                    .type =
                        Type{
                            .modulePath =
                                ModulePath{
                                    .isAbsolute = true, .segments = {ModulePathSegment("A"), ModulePathSegment("B")}
                                },
                            .name = "Hello",
                            .templateParameters =
                                {Type{
                                     .modulePath =
                                         ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("Std")}},
                                     .name = "Array",
                                     .templateParameters = {}
                                 },
                                 Type{
                                     .modulePath =
                                         ModulePath{
                                             .isAbsolute = true,
                                             .segments = {ModulePathSegment("A"), ModulePathSegment("B")}
                                         },
                                     .name = "C",
                                     .templateParameters =
                                         {Type{
                                              .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                              .name = "Int",
                                              .templateParameters = {}
                                          },
                                          Type{
                                              .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                              .name = "Double",
                                              .templateParameters = {}
                                          }}
                                 }}
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<FunctionParameterTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);

PARSE_TEST(FunctionDeclaration)

INSTANTIATE_TEST_SUITE_P(
    , ParseFunctionDeclarationTest,
    ::testing::Values(
        FunctionDeclarationTestParamsType{
            .name = "noArgument",
            .input = "fn foo(): Int",
            .expectedValue = {FunctionDeclaration{
                .name = "foo",
                .parameters = {},
                .returnType =
                    Type{
                        .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                        .name = "Int",
                        .templateParameters = {}
                    }
            }},
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDeclarationTestParamsType{
            .name = "oneArgument",
            .input = "fn foo(hello:T): Int",
            .expectedValue =
                {
                    FunctionDeclaration{
                        .name = "foo",
                        .parameters =
                            {
                                FunctionParameter{
                                    .name = "hello",
                                    .type =
                                        Type{
                                            .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                            .name = "T",
                                            .templateParameters = {}
                                        }
                                },
                            },
                        .returnType =
                            Type{
                                .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                .name = "Int",
                                .templateParameters = {}
                            },
                    },
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDeclarationTestParamsType{
            .name = "multipleArguments",
            .input = "fn foo(hello:T, world:U): Int",
            .expectedValue =
                {
                    FunctionDeclaration{
                        .name = "foo",
                        .parameters =
                            {
                                FunctionParameter{
                                    .name = "hello",
                                    .type =
                                        Type{
                                            .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                            .name = "T",
                                            .templateParameters = {}
                                        }
                                },
                                FunctionParameter{
                                    .name = "world",
                                    .type =
                                        Type{
                                            .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                            .name = "U",
                                            .templateParameters = {}
                                        }
                                },
                            },
                        .returnType =
                            Type{
                                .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                .name = "Int",
                                .templateParameters = {}
                            },
                    },
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDeclarationTestParamsType{
            .name = "oneTemplateArgument",
            .input = "fn foo(hello: A.B.Hello<Std.Array, .B.C<Int, Double>>): A.B.C<Int>",
            .expectedValue =
                {
                    FunctionDeclaration{
                        .name = "foo",
                        .parameters =
                            {
                                FunctionParameter{
                                    .name = "hello",
                                    .type =
                                        Type{
                                            .modulePath =
                                                ModulePath{
                                                    .isAbsolute = true,
                                                    .segments = {ModulePathSegment("A"), ModulePathSegment("B")}
                                                },
                                            .name = "Hello",
                                            .templateParameters =
                                                {Type{
                                                     .modulePath =
                                                         ModulePath{
                                                             .isAbsolute = true, .segments = {ModulePathSegment("Std")}
                                                         },
                                                     .name = "Array",
                                                     .templateParameters = {}
                                                 },
                                                 Type{
                                                     .modulePath =
                                                         ModulePath{
                                                             .isAbsolute = false, .segments = {ModulePathSegment("B")}
                                                         },
                                                     .name = "C",
                                                     .templateParameters =
                                                         {
                                                             Type{
                                                                 .modulePath =
                                                                     ModulePath{.isAbsolute = false, .segments = {}},
                                                                 .name = "Int",
                                                                 .templateParameters = {}
                                                             },
                                                             Type{
                                                                 .modulePath =
                                                                     ModulePath{.isAbsolute = false, .segments = {}},
                                                                 .name = "Double",
                                                                 .templateParameters = {}
                                                             }
                                                         }
                                                 }}
                                        }
                                },
                            },
                        .returnType =
                            Type{
                                .modulePath =
                                    ModulePath{
                                        .isAbsolute = true, .segments = {ModulePathSegment("A"), ModulePathSegment("B")}
                                    },
                                .name = "C",
                                .templateParameters = {Type{
                                    .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                    .name = "Int",
                                    .templateParameters = {}
                                }}
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

PARSE_TEST(Value)

INSTANTIATE_TEST_SUITE_P(
    , ParseValueTest,
    ::testing::Values(
        ValueTestParamsType{
            .name = "noType",
            .input = "hello",
            .expectedValue = Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("hello")}}},
            .shouldSucceed = true,
            .rest = ""
        },
        ValueTestParamsType{
            .name = "noTypeMultiSegments",
            .input = "hello.a.b",
            .expectedValue =
                Value{
                    .type = std::nullopt,
                    .dataPath =
                        DataPath{.segments = {DataPathSegment("hello"), DataPathSegment("a"), DataPathSegment("b")}}
                },
            .shouldSucceed = true,
            .rest = ""
        },
        ValueTestParamsType{
            .name = "withType",
            .input = "A.B.Hello<Int>.a",
            .expectedValue =
                Value{
                    .type =
                        Type{
                            .modulePath =
                                ModulePath{
                                    .isAbsolute = true, .segments = {ModulePathSegment("A"), ModulePathSegment("B")}
                                },
                            .name = "Hello",
                            .templateParameters = {Type{
                                .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                .name = "Int",
                                .templateParameters = {}
                            }}
                        },
                    .dataPath = DataPath{.segments = {DataPathSegment("a")}}
                },
            .shouldSucceed = true,
            .rest = ""
        },
        ValueTestParamsType{
            .name = "withTypeMultiSegments",
            .input = "A.B.Hello<Int>.a.b.c",
            .expectedValue =
                Value{
                    .type =
                        Type{
                            .modulePath =
                                ModulePath{
                                    .isAbsolute = true, .segments = {ModulePathSegment("A"), ModulePathSegment("B")}
                                },
                            .name = "Hello",
                            .templateParameters = {Type{
                                .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                .name = "Int",
                                .templateParameters = {}
                            }}
                        },
                    .dataPath = DataPath{.segments = {DataPathSegment("a"), DataPathSegment("b"), DataPathSegment("c")}}
                },
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<ValueTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);

PARSE_TEST(ReturnStatement)

INSTANTIATE_TEST_SUITE_P(
    , ParseReturnStatementTest,
    ::testing::
        Values(
            ReturnStatementTestParamsType{
                .name = "noType",
                .input = "return hello;",
                .expectedValue =
                    ReturnStatement{
                        .expr = Expr{Value{
                            .type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("hello")}}
                        }}
                    },
                .shouldSucceed = true,
                .rest = ""
            },
            ReturnStatementTestParamsType{
                .name = "noTypeMultiSegments",
                .input = "return hello.a.b;",
                .expectedValue =
                    ReturnStatement{
                        .expr = Expr{Value{
                            .type = std::nullopt,
                            .dataPath =
                                DataPath{
                                    .segments = {DataPathSegment("hello"), DataPathSegment("a"), DataPathSegment("b")}
                                }
                        }}
                    },
                .shouldSucceed = true,
                .rest = ""
            },
            ReturnStatementTestParamsType{
                .name = "withType",
                .input = "return A.B.Hello<Int>.a;",
                .expectedValue =
                    ReturnStatement{
                        .expr =
                            Expr{
                                Value{
                                    .type =
                                        Type{
                                            .modulePath =
                                                ModulePath{
                                                    .isAbsolute = true,
                                                    .segments = {ModulePathSegment("A"), ModulePathSegment("B")}
                                                },
                                            .name = "Hello",
                                            .templateParameters =
                                                {
                                                    Type{
                                                        .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                                        .name = "Int",
                                                        .templateParameters = {}
                                                    }
                                                }
                                        },
                                    .dataPath = DataPath{.segments = {DataPathSegment("a")}}
                                }
                            }
                    },
                .shouldSucceed = true,
                .rest = ""
            },
            ReturnStatementTestParamsType{
                .name = "withTypeMultiSegments", .input = "return A.B.Hello<Int>.a.b.c;", .expectedValue = ReturnStatement{.expr = Expr{Value{.type = Type{.modulePath = ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("A"), ModulePathSegment("B")}}, .name = "Hello", .templateParameters = {Type{.modulePath = ModulePath{.isAbsolute = false, .segments = {}}, .name = "Int", .templateParameters = {}}}}, .dataPath = DataPath{.segments = {DataPathSegment("a"), DataPathSegment("b"), DataPathSegment("c")}}}}}, .shouldSucceed = true, .rest = ""
            },
            ReturnStatementTestParamsType{
                .name = "functionResult",
                .input = "return A.B.Hello<Int>.a.c(b);",
                .expectedValue =
                    {ReturnStatement{.expr = Expr{FunctionCallExpr{.name = Value{.type = Type{.modulePath = ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("A"), ModulePathSegment("B")}}, .name = "Hello", .templateParameters = {Type{.modulePath = ModulePath{.isAbsolute = false, .segments = {}}, .name = "Int", .templateParameters = {}}}}, .dataPath = DataPath{.segments = {DataPathSegment("a"), DataPathSegment("c")}}}, .parameters = {Expr{Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("b")}}}}}}}}
                    },
                .shouldSucceed = true,
                .rest = ""
            }
        ),
    [](testing::TestParamInfo<ReturnStatementTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);

PARSE_TEST(Expr)

INSTANTIATE_TEST_SUITE_P(
    , ParseExprTest,
    ::testing::Values(
        ExprTestParamsType{
            .name = "functionCall",
            .input = "hello()",
            .expectedValue = Expr{FunctionCallExpr{
                .name = Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("hello")}}},
                .parameters = {}
            }},
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithDataPath",
            .input = "hello.a.b()",
            .expectedValue = Expr{FunctionCallExpr{
                .name =
                    Value{
                        .type = std::nullopt,
                        .dataPath =
                            DataPath{.segments = {DataPathSegment("hello"), DataPathSegment("a"), DataPathSegment("b")}}
                    },
                .parameters = {}
            }},
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithTypePath",
            .input = "A.B.hello()",
            .expectedValue = Expr{FunctionCallExpr{
                .name =
                    Value{
                        .type =
                            Type{
                                .modulePath = ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("A")}},
                                .name = "B",
                                .templateParameters = {}
                            },
                        .dataPath = DataPath{.segments = {DataPathSegment("hello")}}
                    },
                .parameters = {}
            }},
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithTypeAndDataPath",
            .input = "A.B.hello.a.b()",
            .expectedValue = Expr{FunctionCallExpr{
                .name =
                    Value{
                        .type =
                            Type{
                                .modulePath = ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("A")}},
                                .name = "B",
                                .templateParameters = {}
                            },
                        .dataPath =
                            DataPath{.segments = {DataPathSegment("hello"), DataPathSegment("a"), DataPathSegment("b")}}
                    },
                .parameters = {}
            }},
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithEverything",
            .input = ".A.B<Int, Double>.hello.a.b()",
            .expectedValue =
                Expr{FunctionCallExpr{.name = Value{.type = Type{.modulePath = ModulePath{.isAbsolute = false, .segments = {ModulePathSegment("A")}}, .name = "B", .templateParameters = {Type{.modulePath = ModulePath{.isAbsolute = false, .segments = {}}, .name = "Int", .templateParameters = {}}, Type{.modulePath = ModulePath{.isAbsolute = false, .segments = {}}, .name = "Double", .templateParameters = {}}}}, .dataPath = DataPath{.segments = {DataPathSegment("hello"), DataPathSegment("a"), DataPathSegment("b")}}}, .parameters = {}}
                },
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithArguments",
            .input = "hello(a, b, c)",
            .expectedValue = Expr{FunctionCallExpr{
                .name = Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("hello")}}},
                .parameters =
                    {Expr(Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("a")}}}),
                     Expr(Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("b")}}}),
                     Expr(Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("c")}}})}
            }},
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithArgumentsHavePaths",
            .input = "hello(a, b.c.world, c.world)",
            .expectedValue = Expr{FunctionCallExpr{
                .name = Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("hello")}}},
                .parameters =
                    {Expr(Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("a")}}}),
                     Expr(Value{
                         .type = std::nullopt,
                         .dataPath =
                             DataPath{.segments = {DataPathSegment("b"), DataPathSegment("c"), DataPathSegment("world")}}
                     }),
                     Expr(Value{
                         .type = std::nullopt,
                         .dataPath = DataPath{.segments = {DataPathSegment("c"), DataPathSegment("world")}}
                     })}
            }},
            .shouldSucceed = true,
            .rest = ""
        },
        ExprTestParamsType{
            .name = "functionCallWithArgumentIsFunctionCall",
            .input = "hello(A.B.a.d(), c.world(a))",
            .expectedValue =
                Expr{
                    FunctionCallExpr{
                        .name = Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("hello")}}},
                        .parameters =
                            {Expr{
                                 FunctionCallExpr{
                                     .name = Value{.type = Type{.modulePath = ModulePath{.isAbsolute = true, .segments = {ModulePathSegment("A")}}, .name = "B", .templateParameters = {}}, .dataPath = DataPath{.segments = {DataPathSegment("a"), DataPathSegment("d")}}},
                                     .parameters = {}
                                 }
                             },
                             Expr{FunctionCallExpr{
                                 .name =
                                     Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("c"), DataPathSegment("world")}}},
                                 .parameters = {Expr{Value{
                                     .type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("a")}}
                                 }}}
                             }}}
                    }
                },
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<ExprTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);

PARSE_TEST(FunctionCallExprStatement)

INSTANTIATE_TEST_SUITE_P(
    , ParseFunctionCallExprStatementTest,
    ::testing::Values(
        FunctionCallExprStatementTestParamsType{
            .name = "noArguments",
            .input = "hello();",
            .expectedValue =
                FunctionCallExprStatement{
                    .expr =
                        FunctionCallExpr{
                            .name =
                                Value{
                                    .type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("hello")}}
                                },
                            .parameters = {}
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionCallExprStatementTestParamsType{
            .name = "withEverything",
            .input = "A.B<Double>.hello.world(a, b.c);",
            .expectedValue =
                {
                    FunctionCallExprStatement{
                        .expr =
                            FunctionCallExpr{
                                .name =
                                    Value{
                                        .type =
                                            Type{
                                                .modulePath =
                                                    ModulePath{
                                                        .isAbsolute = true, .segments = {ModulePathSegment("A")}
                                                    },
                                                .name = "B",
                                                .templateParameters =
                                                    {
                                                        Type{
                                                            .modulePath =
                                                                ModulePath{.isAbsolute = false, .segments = {}},
                                                            .name = "Double",
                                                            .templateParameters = {}
                                                        }
                                                    }
                                            },
                                        .dataPath = DataPath{.segments = {DataPathSegment("hello"), DataPathSegment("world")}}
                                    },
                                .parameters =
                                    {Expr{Value{
                                         .type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("a")}}
                                     }},
                                     Expr{Value{
                                         .type = std::nullopt,
                                         .dataPath = DataPath{.segments = {DataPathSegment("b"), DataPathSegment("c")}}
                                     }}}
                            }
                    }
                },
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<FunctionCallExprStatementTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);

PARSE_TEST(Block)

INSTANTIATE_TEST_SUITE_P(
    , ParseBlockTest,
    ::testing::Values(
        BlockTestParamsType{
            .name = "emptyBlock",
            .input = "{}",
            .expectedValue = Block{.statements = {}},
            .shouldSucceed = true,
            .rest = ""
        },
        BlockTestParamsType{
            .name = "singleStatement",
            .input = "{return hello;}",
            .expectedValue =
                Block{
                    .statements = {Statement{ReturnStatement{
                        .expr = Expr{Value{
                            .type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("hello")}}
                        }}
                    }}}
                },
            .shouldSucceed = true,
            .rest = ""
        },
        BlockTestParamsType{
            .name = "multipleStatements",
            .input = "{hello.a(); return world;}",
            .expectedValue =
                Block{
                    .statements =
                        {Statement{
                             FunctionCallExprStatement{
                                 .expr =
                                     FunctionCallExpr{
                                         .name =
                                             Value{
                                                 .type = std::nullopt,
                                                 .dataPath = DataPath{.segments = {DataPathSegment("hello"), DataPathSegment("a")}}
                                             },
                                         .parameters = {}
                                     }
                             }
                         },
                         Statement{ReturnStatement{
                             .expr = Expr{Value{
                                 .type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("world")}}
                             }}
                         }}}
                },
            .shouldSucceed = true,
            .rest = ""
        },
        BlockTestParamsType{
            .name = "nestedBlock",
            .input = "{hello(b); {return world;}}",
            .expectedValue =
                Block{
                    .statements =
                        {Statement{
                             FunctionCallExprStatement{
                                 .expr = FunctionCallExpr{.name = Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("hello")}}}, .parameters = {Expr{Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("b")}}}}}}
                             }
                         },
                         Statement{
                             Block{
                                 .statements =
                                     {
                                         Statement{
                                             ReturnStatement{
                                                 .expr = Expr{Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("world")}}}}
                                             }
                                         }
                                     }
                             }
                         }}
                },
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<BlockTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);

PARSE_TEST(FunctionDefinition)

INSTANTIATE_TEST_SUITE_P(
    , ParseFunctionDefinitionTest,
    ::testing::Values(
        FunctionDefinitionTestParamsType{
            .name = "noArguments",
            .input = "fn hello(): Int {}",
            .expectedValue =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters = {},
                            .returnType =
                                Type{
                                    .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                    .name = "Int",
                                    .templateParameters = {}
                                }
                        },
                    .body = Block{.statements = {}}
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withArguments",
            .input = "fn hello(a: Int, b: Double): Int {}",
            .expectedValue =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters =
                                {
                                    FunctionParameter{
                                        .name = "a",
                                        .type =
                                            Type{
                                                .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                                .name = "Int",
                                                .templateParameters = {}
                                            }
                                    },
                                    FunctionParameter{
                                        .name = "b",
                                        .type =
                                            Type{
                                                .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                                .name = "Double",
                                                .templateParameters = {}
                                            }
                                    },
                                },
                            .returnType =
                                Type{
                                    .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                    .name = "Int",
                                    .templateParameters = {}
                                }
                        },
                    .body = Block{.statements = {}}
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withBody",
            .input = "fn hello(): Int {return world;}",
            .expectedValue =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters = {},
                            .returnType =
                                Type{
                                    .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                    .name = "Int",
                                    .templateParameters = {}
                                }
                        },
                    .body =
                        Block{
                            .statements = {Statement{ReturnStatement{
                                .expr = Expr{Value{
                                    .type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("world")}}
                                }}
                            }}}
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withArgumentsAndBody",
            .input = "fn hello(a: Int, b: Double): Int {return world;}",
            .expectedValue =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters =
                                {
                                    FunctionParameter{
                                        .name = "a",
                                        .type = Type{.modulePath = ModulePath{.isAbsolute = false, .segments = {}}, .name = "Int", .templateParameters = {}}
                                    },
                                    FunctionParameter{
                                        .name = "b",
                                        .type = Type{.modulePath = ModulePath{.isAbsolute = false, .segments = {}}, .name = "Double", .templateParameters = {}}
                                    },
                                },
                            .returnType =
                                Type{
                                    .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                    .name = "Int",
                                    .templateParameters = {}
                                }
                        },
                    .body =
                        Block{
                            .statements =
                                {
                                    Statement{
                                        ReturnStatement{
                                            .expr = Expr{Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("world")}}}}
                                        }
                                    }
                                }
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withArgumentsAndBodyAndNestedBlock",
            .input = R"(fn hello(a: Int, b: Double): Int {
                            hello();
                            {
                                return world;
                            }
                        }
                     )",
            .expectedValue =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters =
                                {
                                    FunctionParameter{
                                        .name = "a",
                                        .type = Type{.modulePath = ModulePath{.isAbsolute = false, .segments = {}}, .name = "Int", .templateParameters = {}}
                                    },
                                    FunctionParameter{
                                        .name = "b",
                                        .type = Type{.modulePath = ModulePath{.isAbsolute = false, .segments = {}}, .name = "Double", .templateParameters = {}}
                                    },
                                },
                            .returnType =
                                Type{
                                    .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                    .name = "Int",
                                    .templateParameters = {}
                                }
                        },
                    .body =
                        Block{
                            .statements =
                                {Statement{
                                     FunctionCallExprStatement{
                                         .expr = FunctionCallExpr{.name = Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("hello")}}}, .parameters = {}}
                                     }
                                 },
                                 Statement{
                                     Block{
                                         .statements =
                                             {
                                                 Statement{
                                                     ReturnStatement{
                                                         .expr = Expr{Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("world")}}}}
                                                     }
                                                 }
                                             }
                                     }
                                 }}
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        },
        FunctionDefinitionTestParamsType{
            .name = "withArgumentsAndBodyAndNestedBlockAndNestedFunctionCall",
            .input = R"(fn hello(a: Int, b: Double): Int {
                          hello();
                          {
                              return world(a);
                          }
                      })",
            .expectedValue =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters =
                                {
                                    FunctionParameter{
                                        .name = "a",
                                        .type = Type{.modulePath = ModulePath{.isAbsolute = false, .segments = {}}, .name = "Int", .templateParameters = {}}
                                    },
                                    FunctionParameter{
                                        .name = "b",
                                        .type = Type{.modulePath = ModulePath{.isAbsolute = false, .segments = {}}, .name = "Double", .templateParameters = {}}
                                    },
                                },
                            .returnType =
                                Type{
                                    .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                    .name = "Int",
                                    .templateParameters = {}
                                }
                        },
                    .body =
                        Block{
                            .statements =
                                {Statement{
                                     FunctionCallExprStatement{
                                         .expr = FunctionCallExpr{.name = Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("hello")}}}, .parameters = {}}
                                     }
                                 },
                                 Statement{
                                     Block{.statements = {Statement{ReturnStatement{.expr = Expr{FunctionCallExpr{.name = Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("world")}}}, .parameters = {Expr{Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("a")}}}}}}}}}}}
                                 }}
                        }
                },
            .shouldSucceed = true,
            .rest = ""
        }, FunctionDefinitionTestParamsType{
            .name = "nestedFunctionDefinition",
            .input = R"(fn hello(a: Int, b: Double): Int {
                            fn world(): Int {
                                hello();
                            }
                            return world();
                        }
            )",
            .expectedValue =
                FunctionDefinition{
                    .declaration =
                        FunctionDeclaration{
                            .name = "hello",
                            .parameters =
                                {
                                    FunctionParameter{
                                        .name = "a",
                                        .type = Type{.modulePath = ModulePath{.isAbsolute = false, .segments = {}}, .name = "Int", .templateParameters = {}}
                                    },
                                    FunctionParameter{
                                        .name = "b",
                                        .type = Type{.modulePath = ModulePath{.isAbsolute = false, .segments = {}}, .name = "Double", .templateParameters = {}}
                                    },
                                },
                            .returnType =
                                Type{
                                    .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                    .name = "Int",
                                    .templateParameters = {}
                                }
                        },
                    .body = Block{
                        .statements =
                            {Statement{
                                 FunctionDefinition{
                                     .declaration =
                                         FunctionDeclaration{
                                             .name = "world",
                                             .parameters = {},
                                             .returnType =
                                                 Type{
                                                     .modulePath = ModulePath{.isAbsolute = false, .segments = {}},
                                                     .name = "Int",
                                                     .templateParameters = {}
                                                 }
                                         },
                                     .body = Block{
                                         .statements =
                                             {Statement{
                                                  FunctionCallExprStatement{
                                                      .expr = FunctionCallExpr{.name = Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("hello")}}}, .parameters = {}}
                                                  }
                                              }}
                                     }
                                 }
                             },
                             Statement{
                                 ReturnStatement{
                                     .expr = Expr{FunctionCallExpr{.name = Value{.type = std::nullopt, .dataPath = DataPath{.segments = {DataPathSegment("world")}}}, .parameters = {}}}
                                 }
                             }
                            }
                    }
                },
            .shouldSucceed = true,
            .rest = ""
        }
    ),
    [](testing::TestParamInfo<FunctionDefinitionTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);