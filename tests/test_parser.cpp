#include <fmt/core.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>

#include <string_view>
#include <utility>

#include "rules.hpp"

using life_lang::ast::Argument;
using life_lang::ast::ArgumentList;
using life_lang::ast::Identifier;
using life_lang::ast::Path;
using life_lang::ast::Type;

#define PARSE_TEST(name)                                                               \
  using name##TestParamsType = ParseTestParams<name>;                                  \
  class Parse##name##Test : public ::testing::TestWithParam<name##TestParamsType> {};  \
                                                                                       \
  TEST_P(Parse##name##Test, Parse##name) {                                             \
    auto const& params = GetParam();                                                   \
    auto inputStart = params.input.cbegin();                                           \
    auto const inputEnd = params.input.cend();                                         \
    std::ostringstream errorMsg;                                                       \
    auto const ret = life_lang::internal::Parse##name(inputStart, inputEnd, errorMsg); \
    EXPECT_EQ(params.shouldSucceed, ret.first) << errorMsg.str();                      \
    if (params.shouldSucceed) {                                                        \
      EXPECT_EQ(params.shouldConsumeAll, inputStart == inputEnd);                      \
      EXPECT_EQ(params.expectedValue, ret.second);                                     \
    }                                                                                  \
  }

template <typename AstType>
struct ParseTestParams {
  std::string_view name;
  std::string input;
  AstType expectedValue;
  bool shouldSucceed{};
  bool shouldConsumeAll{};
};  // namespace std::ranges

PARSE_TEST(Identifier)

INSTANTIATE_TEST_SUITE_P(
    , ParseIdentifierTest,
    ::testing::Values(
        IdentifierTestParamsType{
            .name = "allLowerAlphabets",
            .input = "hello",
            .expectedValue = Identifier{.value = "hello"},
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        IdentifierTestParamsType{
            .name = "allUpperAlphabets",
            .input = "HELLO",
            .expectedValue = Identifier{.value = "HELLO"},
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        IdentifierTestParamsType{
            .name = "mixedAlphaNumeric",
            .input = "h340",
            .expectedValue = Identifier{.value = "h340"},
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        IdentifierTestParamsType{
            .name = "allUpperAlphabetsStartsWithUnderscore",
            .input = "_hello",
            .expectedValue = Identifier{.value = "_hello"},
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        IdentifierTestParamsType{
            .name = "allLowerAlphabetsStartsWithUnderscore",
            .input = "_HELLO",
            .expectedValue = Identifier{.value = "_HELLO"},
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        IdentifierTestParamsType{
            .name = "allUpperAlphabetsStartsWithMultipleUnderscore",
            .input = "__hello",
            .expectedValue = Identifier{.value = "__hello"},
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        IdentifierTestParamsType{
            .name = "allLowerAlphabetsStartsWithMultipleUnderscore",
            .input = "__HELLO",
            .expectedValue = Identifier{.value = "__HELLO"},
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        IdentifierTestParamsType{
            .name = "underScoreInBetween",
            .input = "_he__ll_o",
            .expectedValue = Identifier{.value = "_he__ll_o"},
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        IdentifierTestParamsType{
            .name = "underScoreAtEnd",
            .input = "_hello_",
            .expectedValue = Identifier{.value = "_hello_"},
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        IdentifierTestParamsType{
            .name = "underScoreOnly",
            .input = "_",
            .expectedValue = Identifier{.value = "_"},
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        IdentifierTestParamsType{
            .name = "unsupportedStartsWithNumber",
            .input = "0abc",
            .expectedValue = Identifier{.value = ""},
            .shouldSucceed = false,
            .shouldConsumeAll = false
        }
    ),
    [](testing::TestParamInfo<IdentifierTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);

PARSE_TEST(Path)

INSTANTIATE_TEST_SUITE_P(
    , ParsePathTest,
    ::testing::Values(
        PathTestParamsType{
            .name = "singleSegment",
            .input = "a",
            .expectedValue = Path{.isAbsolute = false, .segments = {Identifier{.value = "a"}}},
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        PathTestParamsType{
            .name = "multiSegments",
            .input = "a::b::c",
            .expectedValue =
                Path{
                    .isAbsolute = false,
                    .segments = {Identifier{.value = "a"}, Identifier{.value = "b"}, Identifier{.value = "c"}}
                },
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        PathTestParamsType{
            .name = "AbsolutePath",
            .input = "::a::b::c",
            .expectedValue =
                Path{
                    .isAbsolute = true,
                    .segments = {Identifier{.value = "a"}, Identifier{.value = "b"}, Identifier{.value = "c"}}
                },

            .shouldSucceed = true,
            .shouldConsumeAll = true
        }
    ),
    [](testing::TestParamInfo<PathTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);

PARSE_TEST(Type)

INSTANTIATE_TEST_SUITE_P(
    , ParseTypeTest,
    ::testing::Values(
        TypeTestParamsType{
            .name = "noNamespace",
            .input = "hello",
            .expectedValue =
                Type{
                    .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "hello"}}},
                    .templateArguments = {}
                },
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        TypeTestParamsType{
            .name = "absoluteNamespace",
            .input = "::hello<int>",
            .expectedValue =
                Type{
                    .path = Path{.isAbsolute = true, .segments = {Identifier{.value = "hello"}}},
                    .templateArguments = {Type{
                        .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "int"}}},
                        .templateArguments = {}
                    }}
                },
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        TypeTestParamsType{
            .name = "templateArgumentHasNamespace",
            .input = "::hello<std::Array>",
            .expectedValue =
                Type{
                    .path = Path{.isAbsolute = true, .segments = {Identifier{.value = "hello"}}},
                    .templateArguments = {Type{
                        .path =
                            Path{
                                .isAbsolute = false,
                                .segments = {Identifier{.value = "std"}, Identifier{.value = "Array"}}
                            },
                        .templateArguments = {}
                    }}
                },
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        TypeTestParamsType{
            .name = "templateArgumentHasAbsoluteNamespace",
            .input = "::hello<::std::Array>",
            .expectedValue =
                Type{
                    .path = Path{.isAbsolute = true, .segments = {Identifier{.value = "hello"}}},
                    .templateArguments = {Type{
                        .path =
                            Path{
                                .isAbsolute = true,
                                .segments = {Identifier{.value = "std"}, Identifier{.value = "Array"}}
                            },
                        .templateArguments = {}
                    }}
                },
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        TypeTestParamsType{
            .name = "multipleTemplateArgument",
            .input = "a::b::hello<::std::Array, a::b::C<int, double>>",
            .expectedValue =
                Type{
                    .path =
                        Path{
                            .isAbsolute = false,
                            .segments =
                                {Identifier{.value = "a"}, Identifier{.value = "b"}, Identifier{.value = "hello"}}
                        },
                    .templateArguments =
                        {Type{
                             .path =
                                 Path{
                                     .isAbsolute = true,
                                     .segments = {Identifier{.value = "std"}, Identifier{.value = "Array"}}
                                 },
                             .templateArguments = {}
                         },
                         Type{
                             .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "a"}, Identifier{.value = "b"}, Identifier{.value = "C"}}},
                             .templateArguments =
                                 {Type{
                                      .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "int"}}},
                                      .templateArguments = {}
                                  },
                                  Type{
                                      .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "double"}}},
                                      .templateArguments = {}
                                  }}
                         }}
                },
            .shouldSucceed = true,
            .shouldConsumeAll = true
        }
    ),
    [](testing::TestParamInfo<TypeTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);

PARSE_TEST(Argument)

INSTANTIATE_TEST_SUITE_P(
    , ParseArgumentTest,
    ::testing::Values(
        ArgumentTestParamsType{
            .name = "noNamespace",
            .input = "hello:T",
            .expectedValue =
                Argument{
                    .name = Identifier{.value = "hello"},
                    .type =
                        Type{
                            .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "T"}}},
                            .templateArguments = {}
                        }
                },
            .shouldSucceed = true,
            .shouldConsumeAll = true
        },
        ArgumentTestParamsType{
            .name = "multipleTemplateArgument",
            .input = "hello: a::b::hello<::std::Array, a::b::C<int, double>>",
            .expectedValue =
                Argument{
                    .name = Identifier{.value = "hello"},
                    .type =
                        Type{
                            .path =
                                Path{
                                    .isAbsolute = false,
                                    .segments =
                                        {Identifier{.value = "a"}, Identifier{.value = "b"},
                                         Identifier{.value = "hello"}}
                                },
                            .templateArguments =
                                {Type{
                                     .path =
                                         Path{
                                             .isAbsolute = true,
                                             .segments = {Identifier{.value = "std"}, Identifier{.value = "Array"}}
                                         },
                                     .templateArguments = {}
                                 },
                                 Type{
                                     .path =
                                         Path{
                                             .isAbsolute = false,
                                             .segments =
                                                 {Identifier{.value = "a"}, Identifier{.value = "b"},
                                                  Identifier{.value = "C"}}
                                         },
                                     .templateArguments =
                                         {Type{
                                              .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "int"}}},
                                              .templateArguments = {}
                                          },
                                          Type{
                                              .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "double"}}},
                                              .templateArguments = {}
                                          }}
                                 }}
                        }
                },
            .shouldSucceed = true,
            .shouldConsumeAll = true
        }
    ),
    [](testing::TestParamInfo<ArgumentTestParamsType> const& paramInfo) { return std::string{paramInfo.param.name}; }
);

PARSE_TEST(ArgumentList)

INSTANTIATE_TEST_SUITE_P(
    , ParseArgumentListTest,
    ::testing::
        Values(
            ArgumentListTestParamsType{
                .name = "noArgument",
                .input = "()",
                .expectedValue = {},
                .shouldSucceed = true,
                .shouldConsumeAll = true
            },
            ArgumentListTestParamsType{
                .name = "oneArgument",
                .input = "(hello:T)",
                .expectedValue =
                    {
                        Argument{
                            .name = Identifier{.value = "hello"},
                            .type =
                                Type{
                                    .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "T"}}},
                                    .templateArguments = {}
                                }
                        },
                    },
                .shouldSucceed = true,
                .shouldConsumeAll = true
            },
            ArgumentListTestParamsType{
                .name = "multipleArguments",
                .input = "(hello:T, world:U)",
                .expectedValue =
                    {
                        Argument{
                            .name = Identifier{.value = "hello"},
                            .type =
                                Type{
                                    .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "T"}}},
                                    .templateArguments = {}
                                }
                        },
                        Argument{
                            .name = Identifier{.value = "world"},
                            .type =
                                Type{
                                    .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "U"}}},
                                    .templateArguments = {}
                                }
                        },
                    },
                .shouldSucceed = true,
                .shouldConsumeAll = true
            },
            ArgumentListTestParamsType{
                .name = "oneTemplateArgument",
                .input = "(hello: a::b::hello<::std::Array, a::b::C<int, double>>)",
                .expectedValue =
                    {
                        Argument{
                            .name = Identifier{.value = "hello"},
                            .type =
                                Type{
                                    .path =
                                        Path{
                                            .isAbsolute = false,
                                            .segments =
                                                {Identifier{.value = "a"}, Identifier{.value = "b"},
                                                 Identifier{.value = "hello"}}
                                        },
                                    .templateArguments =
                                        {Type{
                                             .path =
                                                 Path{
                                                     .isAbsolute = true,
                                                     .segments =
                                                         {Identifier{.value = "std"}, Identifier{.value = "Array"}}
                                                 },
                                             .templateArguments = {}
                                         },
                                         Type{
                                             .path =
                                                 Path{
                                                     .isAbsolute = false,
                                                     .segments = {Identifier{.value = "a"}, Identifier{.value = "b"}, Identifier{.value = "C"}}
                                                 },
                                             .templateArguments =
                                                 {Type{
                                                      .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "int"}}},
                                                      .templateArguments = {}
                                                  },
                                                  Type{
                                                      .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "double"}}},
                                                      .templateArguments = {}
                                                  }}
                                         }}
                                }
                        },
                    },
                .shouldSucceed = true,
                .shouldConsumeAll = true
            },
            ArgumentListTestParamsType{
                .name = "multipleTemplateArguments",
                .input = "(hello: a::b::hello<::std::Array, a::b::C<int, double>>, world: ::world<A<::B>, C<D>>)",
                .expectedValue =
                    {
                        Argument{
                            .name = Identifier{.value = "hello"},
                            .type = Type{.path = Path{.isAbsolute = false, .segments = {Identifier{.value = "a"}, Identifier{.value = "b"}, Identifier{.value = "hello"}}}, .templateArguments = {Type{.path = Path{.isAbsolute = true, .segments = {Identifier{.value = "std"}, Identifier{.value = "Array"}}}, .templateArguments = {}}, Type{.path = Path{.isAbsolute = false, .segments = {Identifier{.value = "a"}, Identifier{.value = "b"}, Identifier{.value = "C"}}}, .templateArguments = {Type{.path = Path{.isAbsolute = false, .segments = {Identifier{.value = "int"}}}, .templateArguments = {}}, Type{.path = Path{.isAbsolute = false, .segments = {Identifier{.value = "double"}}}, .templateArguments = {}}}}}}
                        },
                        Argument{
                            .name = Identifier{.value = "world"},
                            .type =
                                Type{
                                    .path = Path{.isAbsolute = true, .segments = {Identifier{.value = "world"}}},
                                    .templateArguments =
                                        {
                                            Type{
                                                .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "A"}}},
                                                .templateArguments =
                                                    {
                                                        Type{
                                                            .path = Path{.isAbsolute = true, .segments = {Identifier{.value = "B"}}},
                                                            .templateArguments = {}
                                                        },
                                                    }
                                            },
                                            Type{
                                                .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "C"}}},
                                                .templateArguments =
                                                    {
                                                        Type{
                                                            .path = Path{.isAbsolute = false, .segments = {Identifier{.value = "D"}}},
                                                            .templateArguments = {}
                                                        },
                                                    }
                                            },
                                        },
                                },
                        },
                    },
                .shouldSucceed = true,
                .shouldConsumeAll = true
            }
        ),
    [](testing::TestParamInfo<ArgumentListTestParamsType> const& paramInfo) {
      return std::string{paramInfo.param.name};
    }
);