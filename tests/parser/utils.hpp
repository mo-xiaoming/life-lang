#include <fmt/core.h>
#include <fmt/format.h>

#include <boost/fusion/include/io.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <rules.hpp>
#include <string_view>

#define PARSE_TEST(AstType, fn_name)                                                          \
  namespace {                                                                                 \
  using AstType##_Params = Parse_Test_Params<AstType>;                                        \
  void check_parse(AstType##_Params const& params) {                                          \
    auto input_start = params.input.cbegin();                                                 \
    auto const input_end = params.input.cend();                                               \
    std::ostringstream error_msg;                                                             \
    auto const got = life_lang::internal::parse_##fn_name(input_start, input_end, error_msg); \
    CHECK(params.should_succeed == bool(got));                                                \
    if (params.should_succeed != bool(got)) {                                                 \
      UNSCOPED_INFO((got ? to_json_string(*got, 2) : error_msg.str()));                       \
    }                                                                                         \
    auto const rest = std::string_view{input_start, input_end};                               \
    CHECK(params.rest == rest);                                                               \
    if (got) {                                                                                \
      CHECK(to_json_string(params.expected, 2) == to_json_string(*got, 2));                   \
    }                                                                                         \
  }                                                                                           \
  }  // namespace

template <typename Ast_Type>
struct Parse_Test_Params {
  std::string_view name;
  std::string input;
  Ast_Type expected;
  bool should_succeed{};
  std::string_view rest;
};

// Catch2 uses operator<< to print test parameters in failure messages
// Note: Uses indent=-1 for compact single-line JSON output
template <typename T>
std::ostream& operator<<(std::ostream& a_os, Parse_Test_Params<T> const& a_params) {
  return a_os << fmt::format(
             R"({{.input = "{}", .expected = {}, .shouldSucceed = {}, .rest = "{}"}})", a_params.input,
             to_json_string(a_params.expected, -1), a_params.should_succeed, a_params.rest
         );
}