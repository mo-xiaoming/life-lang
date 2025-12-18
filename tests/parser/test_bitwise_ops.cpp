#include <doctest/doctest.h>
#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {

// Bitwise AND tests
constexpr auto k_bitwise_and_should_succeed = true;
constexpr auto k_bitwise_and_input = "a & b";
constexpr auto k_bitwise_and_expected = R"((binary & (var ((var_segment "a"))) (var ((var_segment "b")))))";

constexpr auto k_bitwise_and_literals_should_succeed = true;
constexpr auto k_bitwise_and_literals_input = "0xFF & 0x0F";
constexpr auto k_bitwise_and_literals_expected = R"((binary & (integer "0xFF") (integer "0x0F")))";

constexpr auto k_bitwise_and_binary_should_succeed = true;
constexpr auto k_bitwise_and_binary_input = "0b1111 & 0b0011";
constexpr auto k_bitwise_and_binary_expected = R"((binary & (integer "0b1111") (integer "0b0011")))";

// Bitwise OR tests
constexpr auto k_bitwise_or_should_succeed = true;
constexpr auto k_bitwise_or_input = "a | b";
constexpr auto k_bitwise_or_expected = R"((binary | (var ((var_segment "a"))) (var ((var_segment "b")))))";

constexpr auto k_bitwise_or_literals_should_succeed = true;
constexpr auto k_bitwise_or_literals_input = "0b0001 | 0b0010";
constexpr auto k_bitwise_or_literals_expected = R"((binary | (integer "0b0001") (integer "0b0010")))";

constexpr auto k_bitwise_or_chained_should_succeed = true;
constexpr auto k_bitwise_or_chained_input = "0b0001 | 0b0010 | 0b0100";
constexpr auto k_bitwise_or_chained_expected =
    R"((binary | (binary | (integer "0b0001") (integer "0b0010")) (integer "0b0100")))";

// Bitwise XOR tests
constexpr auto k_bitwise_xor_should_succeed = true;
constexpr auto k_bitwise_xor_input = "a ^ b";
constexpr auto k_bitwise_xor_expected = R"((binary ^ (var ((var_segment "a"))) (var ((var_segment "b")))))";

constexpr auto k_bitwise_xor_literals_should_succeed = true;
constexpr auto k_bitwise_xor_literals_input = "0xFF ^ 0xAA";
constexpr auto k_bitwise_xor_literals_expected = R"((binary ^ (integer "0xFF") (integer "0xAA")))";

// Shift left tests
constexpr auto k_shift_left_should_succeed = true;
constexpr auto k_shift_left_input = "value << 2";
constexpr auto k_shift_left_expected = R"((binary << (var ((var_segment "value"))) (integer "2")))";

constexpr auto k_shift_left_literal_should_succeed = true;
constexpr auto k_shift_left_literal_input = "1 << 8";
constexpr auto k_shift_left_literal_expected = R"((binary << (integer "1") (integer "8")))";

constexpr auto k_shift_left_binary_should_succeed = true;
constexpr auto k_shift_left_binary_input = "0b0001 << 4";
constexpr auto k_shift_left_binary_expected = R"((binary << (integer "0b0001") (integer "4")))";

// Shift right tests
constexpr auto k_shift_right_should_succeed = true;
constexpr auto k_shift_right_input = "value >> 2";
constexpr auto k_shift_right_expected = R"((binary >> (var ((var_segment "value"))) (integer "2")))";

constexpr auto k_shift_right_literal_should_succeed = true;
constexpr auto k_shift_right_literal_input = "256 >> 4";
constexpr auto k_shift_right_literal_expected = R"((binary >> (integer "256") (integer "4")))";

constexpr auto k_shift_right_hex_should_succeed = true;
constexpr auto k_shift_right_hex_input = "0xFF00 >> 8";
constexpr auto k_shift_right_hex_expected = R"((binary >> (integer "0xFF00") (integer "8")))";

// Mixed bitwise operations
constexpr auto k_mixed_and_or_should_succeed = true;
constexpr auto k_mixed_and_or_input = "a & b | c";
constexpr auto k_mixed_and_or_expected =
    R"((binary | (binary & (var ((var_segment "a"))) (var ((var_segment "b")))) (var ((var_segment "c")))))";

constexpr auto k_mixed_shift_and_should_succeed = true;
constexpr auto k_mixed_shift_and_input = "x << 2 & mask";
constexpr auto k_mixed_shift_and_expected =
    R"((binary & (binary << (var ((var_segment "x"))) (integer "2")) (var ((var_segment "mask")))))";

// Complex expressions
constexpr auto k_complex_flags_should_succeed = true;
constexpr auto k_complex_flags_input = "(flags & 0xFF) | (value << 8)";
constexpr auto k_complex_flags_expected =
    R"((binary | (binary & (var ((var_segment "flags"))) (integer "0xFF")) (binary << (var ((var_segment "value"))) (integer "8"))))";

}  // namespace

TEST_SUITE("Parser - Bitwise Operators") {
  TEST_CASE("Bitwise operators parse correctly") {
    std::vector<Parse_Test_Params<Expr>> const params_list{
        // Bitwise AND
        {.name = "bitwise AND",
         .input = k_bitwise_and_input,
         .expected = k_bitwise_and_expected,
         .should_succeed = k_bitwise_and_should_succeed},
        {.name = "bitwise AND with hex literals",
         .input = k_bitwise_and_literals_input,
         .expected = k_bitwise_and_literals_expected,
         .should_succeed = k_bitwise_and_literals_should_succeed},
        {.name = "bitwise AND with binary literals",
         .input = k_bitwise_and_binary_input,
         .expected = k_bitwise_and_binary_expected,
         .should_succeed = k_bitwise_and_binary_should_succeed},
        // Bitwise OR
        {.name = "bitwise OR",
         .input = k_bitwise_or_input,
         .expected = k_bitwise_or_expected,
         .should_succeed = k_bitwise_or_should_succeed},
        {.name = "bitwise OR with binary literals",
         .input = k_bitwise_or_literals_input,
         .expected = k_bitwise_or_literals_expected,
         .should_succeed = k_bitwise_or_literals_should_succeed},
        {.name = "bitwise OR chained",
         .input = k_bitwise_or_chained_input,
         .expected = k_bitwise_or_chained_expected,
         .should_succeed = k_bitwise_or_chained_should_succeed},
        // Bitwise XOR
        {.name = "bitwise XOR",
         .input = k_bitwise_xor_input,
         .expected = k_bitwise_xor_expected,
         .should_succeed = k_bitwise_xor_should_succeed},
        {.name = "bitwise XOR with hex literals",
         .input = k_bitwise_xor_literals_input,
         .expected = k_bitwise_xor_literals_expected,
         .should_succeed = k_bitwise_xor_literals_should_succeed},
        // Shift left
        {.name = "shift left",
         .input = k_shift_left_input,
         .expected = k_shift_left_expected,
         .should_succeed = k_shift_left_should_succeed},
        {.name = "shift left literal",
         .input = k_shift_left_literal_input,
         .expected = k_shift_left_literal_expected,
         .should_succeed = k_shift_left_literal_should_succeed},
        {.name = "shift left binary literal",
         .input = k_shift_left_binary_input,
         .expected = k_shift_left_binary_expected,
         .should_succeed = k_shift_left_binary_should_succeed},
        // Shift right
        {.name = "shift right",
         .input = k_shift_right_input,
         .expected = k_shift_right_expected,
         .should_succeed = k_shift_right_should_succeed},
        {.name = "shift right literal",
         .input = k_shift_right_literal_input,
         .expected = k_shift_right_literal_expected,
         .should_succeed = k_shift_right_literal_should_succeed},
        {.name = "shift right hex literal",
         .input = k_shift_right_hex_input,
         .expected = k_shift_right_hex_expected,
         .should_succeed = k_shift_right_hex_should_succeed},
        // Mixed operations
        {.name = "mixed AND and OR",
         .input = k_mixed_and_or_input,
         .expected = k_mixed_and_or_expected,
         .should_succeed = k_mixed_and_or_should_succeed},
        {.name = "mixed shift and AND",
         .input = k_mixed_shift_and_input,
         .expected = k_mixed_shift_and_expected,
         .should_succeed = k_mixed_shift_and_should_succeed},
        {.name = "complex flags expression",
         .input = k_complex_flags_input,
         .expected = k_complex_flags_expected,
         .should_succeed = k_complex_flags_should_succeed},
    };
    for (auto const& params : params_list) {
      SUBCASE(std::string(params.name).c_str()) {
        check_parse(params);
      }
    }
  }
}
