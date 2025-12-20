#include <doctest/doctest.h>
#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;
using namespace test_sexp;

PARSE_TEST(Expr, expr)

namespace {

// Bitwise AND tests
constexpr auto k_bitwise_and_should_succeed = true;
constexpr auto k_bitwise_and_input = "a & b";
inline auto const k_bitwise_and_expected = binary_expr("&", var_name("a"), var_name("b"));

constexpr auto k_bitwise_and_literals_should_succeed = true;
constexpr auto k_bitwise_and_literals_input = "0xFF & 0x0F";
inline auto const k_bitwise_and_literals_expected = binary_expr("&", integer("0xFF"), integer("0x0F"));

constexpr auto k_bitwise_and_binary_should_succeed = true;
constexpr auto k_bitwise_and_binary_input = "0b1111 & 0b0011";
inline auto const k_bitwise_and_binary_expected = binary_expr("&", integer("0b1111"), integer("0b0011"));

// Bitwise OR tests
constexpr auto k_bitwise_or_should_succeed = true;
constexpr auto k_bitwise_or_input = "a | b";
inline auto const k_bitwise_or_expected = binary_expr("|", var_name("a"), var_name("b"));

constexpr auto k_bitwise_or_literals_should_succeed = true;
constexpr auto k_bitwise_or_literals_input = "0b0001 | 0b0010";
inline auto const k_bitwise_or_literals_expected = binary_expr("|", integer("0b0001"), integer("0b0010"));

constexpr auto k_bitwise_or_chained_should_succeed = true;
constexpr auto k_bitwise_or_chained_input = "0b0001 | 0b0010 | 0b0100";
inline auto const k_bitwise_or_chained_expected =
    binary_expr("|", binary_expr("|", integer("0b0001"), integer("0b0010")), integer("0b0100"));

// Bitwise XOR tests
constexpr auto k_bitwise_xor_should_succeed = true;
constexpr auto k_bitwise_xor_input = "a ^ b";
inline auto const k_bitwise_xor_expected = binary_expr("^", var_name("a"), var_name("b"));

constexpr auto k_bitwise_xor_literals_should_succeed = true;
constexpr auto k_bitwise_xor_literals_input = "0xFF ^ 0xAA";
inline auto const k_bitwise_xor_literals_expected = binary_expr("^", integer("0xFF"), integer("0xAA"));

// Shift left tests
constexpr auto k_shift_left_should_succeed = true;
constexpr auto k_shift_left_input = "value << 2";
inline auto const k_shift_left_expected = binary_expr("<<", var_name("value"), integer("2"));

constexpr auto k_shift_left_literal_should_succeed = true;
constexpr auto k_shift_left_literal_input = "1 << 8";
inline auto const k_shift_left_literal_expected = binary_expr("<<", integer("1"), integer("8"));

constexpr auto k_shift_left_binary_should_succeed = true;
constexpr auto k_shift_left_binary_input = "0b0001 << 4";
inline auto const k_shift_left_binary_expected = binary_expr("<<", integer("0b0001"), integer("4"));

// Shift right tests
constexpr auto k_shift_right_should_succeed = true;
constexpr auto k_shift_right_input = "value >> 2";
inline auto const k_shift_right_expected = binary_expr(">>", var_name("value"), integer("2"));

constexpr auto k_shift_right_literal_should_succeed = true;
constexpr auto k_shift_right_literal_input = "256 >> 4";
inline auto const k_shift_right_literal_expected = binary_expr(">>", integer("256"), integer("4"));

constexpr auto k_shift_right_hex_should_succeed = true;
constexpr auto k_shift_right_hex_input = "0xFF00 >> 8";
inline auto const k_shift_right_hex_expected = binary_expr(">>", integer("0xFF00"), integer("8"));

// Mixed bitwise operations
constexpr auto k_mixed_and_or_should_succeed = true;
constexpr auto k_mixed_and_or_input = "a & b | c";
inline auto const k_mixed_and_or_expected =
    binary_expr("|", binary_expr("&", var_name("a"), var_name("b")), var_name("c"));

constexpr auto k_mixed_shift_and_should_succeed = true;
constexpr auto k_mixed_shift_and_input = "x << 2 & mask";
inline auto const k_mixed_shift_and_expected =
    binary_expr("&", binary_expr("<<", var_name("x"), integer("2")), var_name("mask"));

// Complex expressions
constexpr auto k_complex_flags_should_succeed = true;
constexpr auto k_complex_flags_input = "(flags & 0xFF) | (value << 8)";
inline auto const k_complex_flags_expected = binary_expr(
    "|",
    binary_expr("&", var_name("flags"), integer("0xFF")),
    binary_expr("<<", var_name("value"), integer("8"))
);

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
    for (auto const& params: params_list) {
      SUBCASE(std::string(params.name).c_str()) {
        check_parse(params);
      }
    }
  }
}
