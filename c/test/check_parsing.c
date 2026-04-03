#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <check.h>

#include "../src/line_error.h"
#include "../src/parsing.h"
#include "../src/string_slice.h"

// ================================================================
//  Tokenizer
// ================================================================

// --- string literals ---

START_TEST(test_tokenizer_string_literal_unexpected_eol) {
    Tokenizer line = {};
    LineError err = {};

    line = (Tokenizer){ .line = slice_from_cstring("\" foo") };
    (void)tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_EOL);

    line = (Tokenizer){ .line = slice_from_cstring("\"\\\" foo") };
    (void)tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_EOL);
} END_TEST

START_TEST(test_tokenizer_string_literal) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("\"foo\" bar") };
    LineError err = {};

    StringSlice token = tokenizer_next(&line, &err);
    ck_assert(slice_eq(token, slice_from_cstring("\"foo\"")));
} END_TEST

START_TEST(test_tokenizer_string_literal_escape_sequence) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("\"foo\\n\" bar") };
    LineError err = {};

    StringSlice token = tokenizer_next(&line, &err);
    ck_assert(slice_eq(token, slice_from_cstring("\"foo\\n\"")));
} END_TEST

// --- character literals ---

START_TEST(test_tokenizer_character_literal_unexpected_eol) {
    Tokenizer line = {};
    LineError err = {};

    line = (Tokenizer){ .line = slice_from_cstring("' foo") };
    (void)tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_EOL);

    line = (Tokenizer){ .line = slice_from_cstring("'\\' foo") };
    (void)tokenizer_next(&line, &err);

    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_EOL);
} END_TEST

START_TEST(test_tokenizer_1_byte_character_literal) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("'a' foo" ) };
    LineError err = {};

    StringSlice token = tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_uint_gt(token.length, 0);
    ck_assert(slice_eq(token, slice_from_cstring("'a'")));
}

START_TEST(test_tokenizer_multi_byte_character_literal) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("'foo' bar" ) };
    LineError err = {};

    StringSlice token =  tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_uint_gt(token.length, 0);
    ck_assert(slice_eq(token, slice_from_cstring("'foo'")));
}

START_TEST(test_tokenizer_backslash_character_literal) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("'\\\\' foo" ) };
    LineError err = {};

    StringSlice token =  tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_uint_gt(token.length, 0);
    ck_assert(slice_eq(token, slice_from_cstring("'\\\\'")));
}

START_TEST(test_tokenizer_single_quote_character_literal) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("'\\'' foo" ) };
    LineError err = {};

    StringSlice token =  tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_uint_gt(token.length, 0);
    ck_assert(slice_eq(token, slice_from_cstring("'\\''")));
}

// --- shift operands ---

START_TEST(test_tokenizer_single_lt) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("< <" ) };
    LineError err = {};

    StringSlice token = tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_uint_gt(token.length, 0);
    ck_assert(slice_eq(token, slice_from_cstring("<")));
}

START_TEST(test_tokenizer_single_gt) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("> >" ) };
    LineError err = {};

    StringSlice token =  tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_uint_gt(token.length, 0);
    ck_assert(slice_eq(token, slice_from_cstring(">")));
}

START_TEST(test_tokenizer_lt_gt) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("<>" ) };
    LineError err = {};

    StringSlice token = tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_uint_gt(token.length, 0);
    ck_assert(slice_eq(token, slice_from_cstring("<")));
}

START_TEST(test_tokenizer_gt_lt) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("><" ) };
    LineError err = {};

    StringSlice token = tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_uint_gt(token.length, 0);
    ck_assert(slice_eq(token, slice_from_cstring(">")));
}

START_TEST(test_tokenizer_left_shift) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("<< x" ) };
    LineError err = {};

    StringSlice token = tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_uint_gt(token.length, 0);
    ck_assert(slice_eq(token, slice_from_cstring("<<")));
}

START_TEST(test_tokenizer_triple_lt) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("<<< x" ) };
    LineError err = {};

    StringSlice token = tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_uint_gt(token.length, 0);
    ck_assert(slice_eq(token, slice_from_cstring("<<")));
}

START_TEST(test_tokenizer_right_shift) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring(">> x" ) };
    LineError err = {};

    StringSlice token = tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_uint_gt(token.length, 0);
    ck_assert(slice_eq(token, slice_from_cstring(">>")));
}

START_TEST(test_tokenizer_triple_gt) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring(">>> x" ) };
    LineError err = {};

    StringSlice token = tokenizer_next(&line, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_uint_gt(token.length, 0);
    ck_assert(slice_eq(token, slice_from_cstring(">>")));
}

Suite* tokenizer_suite(void) {
    Suite* suite = suite_create("Tokenizer");

    TCase* tc_string_literals = tcase_create("string literals");
    tcase_add_test(tc_string_literals, test_tokenizer_string_literal_unexpected_eol);
    tcase_add_test(tc_string_literals, test_tokenizer_string_literal);
    tcase_add_test(tc_string_literals, test_tokenizer_string_literal_escape_sequence);

    TCase* tc_character_literals = tcase_create("character literals");
    tcase_add_test(tc_character_literals, test_tokenizer_character_literal_unexpected_eol);
    tcase_add_test(tc_character_literals, test_tokenizer_1_byte_character_literal);
    tcase_add_test(tc_character_literals, test_tokenizer_multi_byte_character_literal);
    tcase_add_test(tc_character_literals, test_tokenizer_backslash_character_literal);
    tcase_add_test(tc_character_literals, test_tokenizer_single_quote_character_literal);

    TCase* tc_shift_operands = tcase_create("shift operands");
    tcase_add_test(tc_shift_operands, test_tokenizer_single_lt);
    tcase_add_test(tc_shift_operands, test_tokenizer_single_gt);
    tcase_add_test(tc_shift_operands, test_tokenizer_lt_gt);
    tcase_add_test(tc_shift_operands, test_tokenizer_gt_lt);
    tcase_add_test(tc_shift_operands, test_tokenizer_left_shift);
    tcase_add_test(tc_shift_operands, test_tokenizer_triple_lt);
    tcase_add_test(tc_shift_operands, test_tokenizer_right_shift);
    tcase_add_test(tc_shift_operands, test_tokenizer_triple_gt);

    suite_add_tcase(suite, tc_string_literals);
    suite_add_tcase(suite, tc_character_literals);
    suite_add_tcase(suite, tc_shift_operands);

    return suite;
}

// ================================================================
//  parse_operand
// ================================================================

// --- reg ---

START_TEST(test_parse_operand_register_x0_to_x31) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("x0"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_REGISTER);
    ck_assert_int_eq(op.reg, 0);

    op = parse_operand(slice_from_cstring("x31"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_REGISTER);
    ck_assert_int_eq(op.reg, 31);

    op = parse_operand(slice_from_cstring("x32"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_ne(op.operand_type, OPERAND_REGISTER);
}

START_TEST(test_parse_operand_register_t0_to_t8) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("t0"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_REGISTER);
    ck_assert_int_eq(op.reg, 3);

    op = parse_operand(slice_from_cstring("t4"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_REGISTER);
    ck_assert_int_eq(op.reg, 7);

    op = parse_operand(slice_from_cstring("t5"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_REGISTER);
    ck_assert_int_eq(op.reg, 28);

    op = parse_operand(slice_from_cstring("t8"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_REGISTER);
    ck_assert_int_eq(op.reg, 31);

    op = parse_operand(slice_from_cstring("t9"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_ne(op.operand_type, OPERAND_REGISTER);
}

START_TEST(test_parse_operand_register_a0_to_a7) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("a0"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_REGISTER);
    ck_assert_int_eq(op.reg, 8);

    op = parse_operand(slice_from_cstring("a7"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_REGISTER);
    ck_assert_int_eq(op.reg, 15);

    op = parse_operand(slice_from_cstring("a8"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_ne(op.operand_type, OPERAND_REGISTER);
}

START_TEST(test_parse_operand_register_s0_to_s11) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("s0"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_REGISTER);
    ck_assert_int_eq(op.reg, 16);

    op = parse_operand(slice_from_cstring("s11"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_REGISTER);
    ck_assert_int_eq(op.reg, 27);

    op = parse_operand(slice_from_cstring("s12"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_ne(op.operand_type, OPERAND_REGISTER);
}

START_TEST(test_parse_operand_register_lr_and_sp) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("lr"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_REGISTER);
    ck_assert_int_eq(op.reg, 1);

    op = parse_operand(slice_from_cstring("sp"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_REGISTER);
    ck_assert_int_eq(op.reg, 2);
}

// --- uint ---

START_TEST(test_parse_operand_uint_binary_malformed) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("0b"), &err);
    ck_assert_int_eq(op.operand_type, OPERAND_INVALID);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    op = parse_operand(slice_from_cstring("0b01a"), &err);
    ck_assert_int_eq(op.operand_type, OPERAND_INVALID);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    op = parse_operand(slice_from_cstring("0b2"), &err);
    ck_assert_int_eq(op.operand_type, OPERAND_INVALID);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
}

START_TEST(test_parse_operand_uint_hexadecimal_malformed) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("0x"), &err);
    ck_assert_int_eq(op.operand_type, OPERAND_INVALID);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    op = parse_operand(slice_from_cstring("0xg"), &err);
    ck_assert_int_eq(op.operand_type, OPERAND_INVALID);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    op = parse_operand(slice_from_cstring("0xG"), &err);
    ck_assert_int_eq(op.operand_type, OPERAND_INVALID);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
}

START_TEST(test_parse_operand_uint_decimal_malformed) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("01"), &err);
    ck_assert_int_eq(op.operand_type, OPERAND_INVALID);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    op = parse_operand(slice_from_cstring("10a"), &err);
    ck_assert_int_eq(op.operand_type, OPERAND_INVALID);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
}

START_TEST(test_parse_operand_uint_binary) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("0b0"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, 0b0);

    op = parse_operand(slice_from_cstring("0b1010"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, 0b1010);

    op = parse_operand(slice_from_cstring("0b0_0000_0001_0010_0011_0100_0101_0110_0111_1000_1001_1010_1011_1100_1101_1110_1111"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, 0x0123456789ABCDEF);

    op = parse_operand(slice_from_cstring("0b1_0000_0001_0010_0011_0100_0101_0110_0111_1000_1001_1010_1011_1100_1101_1110_1111"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NOT_ENCODABLE);
}

START_TEST(test_parse_operand_uint_hexadecimal) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("0x0"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, 0x0);

    op = parse_operand(slice_from_cstring("0x5"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, 0x5);

    op = parse_operand(slice_from_cstring("0x0123456789abcdef"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, 0x0123456789ABCDEF);

    op = parse_operand(slice_from_cstring("0x0123456789ABCDEF"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, 0x0123456789ABCDEF);

    op = parse_operand(slice_from_cstring("0x0123_4567_89AB_CDEF"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, 0x0123456789ABCDEF);

    op = parse_operand(slice_from_cstring("0x1_0123_4567_89AB_CDEF"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NOT_ENCODABLE);
}

START_TEST(test_parse_operand_uint_decimal) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("0"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, 0);

    op = parse_operand(slice_from_cstring("5"), &err);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.uint, 5);

    op = parse_operand(slice_from_cstring("123456789"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, 123456789llu);


    op = parse_operand(slice_from_cstring("184_467_440_737_095_516_15"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, 18446744073709551615llu);

    op = parse_operand(slice_from_cstring("18446744073709551616"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NOT_ENCODABLE);
}

// --- character literal ---

START_TEST(test_parse_operand_character_literal_too_large) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("'012345678'"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NOT_ENCODABLE);
}

START_TEST(test_parse_operand_character_literal_empty) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("''"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, 0);
}

START_TEST(test_parse_operand_character_literal_1_byte) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("'1'"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, '1');
}

START_TEST(test_parse_operand_character_literal_multi_byte) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("'1234'"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, ((uint64_t)('1'<<(8*0))|('2'<<(8*1))|('3'<<(8*2))|('4'<<(8*3))));
}

START_TEST(test_parse_operand_character_literal_backslash) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("'\\\\'"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, '\\');
}

START_TEST(test_parse_operand_character_literal_single_quote) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("'\''"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, '\'');
}

START_TEST(test_parse_operand_character_literal_newline) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("'\\n'"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, '\n');
}

START_TEST(test_parse_operand_character_literal_tab) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("'\\t'"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, '\t');
}

START_TEST(test_parse_operand_character_literal_mix) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("'\\tx\\n'"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_UINT);
    ck_assert_int_eq(op.uint, ((uint64_t)('\t'<<(8*0))|('x'<<(8*1))|('\n'<<(8*2))));
}


// --- symbol ---

START_TEST(test_parse_operand_symbol_invalid) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("+"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_INVALID);

    op = parse_operand(slice_from_cstring("["), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_INVALID);

    op = parse_operand(slice_from_cstring(";"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_INVALID);
}

START_TEST(test_parse_operand_symbol) {
    Operand op = {};
    LineError err = {};

    op = parse_operand(slice_from_cstring("abc"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_SYMBOL);
    ck_assert(slice_eq(op.symbol, slice_from_cstring("abc")));

    op = parse_operand(slice_from_cstring("_abc"), &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(op.operand_type, OPERAND_SYMBOL);
    ck_assert(slice_eq(op.symbol, slice_from_cstring("_abc")));
}

Suite* parse_operand_suite(void) {
    Suite* suite = suite_create("parse_operand");

    TCase* tc_register = tcase_create("reg");
    tcase_add_test(tc_register, test_parse_operand_register_x0_to_x31);
    tcase_add_test(tc_register, test_parse_operand_register_t0_to_t8);
    tcase_add_test(tc_register, test_parse_operand_register_a0_to_a7);
    tcase_add_test(tc_register, test_parse_operand_register_s0_to_s11);
    tcase_add_test(tc_register, test_parse_operand_register_lr_and_sp);

    TCase* tc_uint = tcase_create("uint");
    tcase_add_test(tc_uint, test_parse_operand_uint_binary_malformed);
    tcase_add_test(tc_uint, test_parse_operand_uint_hexadecimal_malformed);
    tcase_add_test(tc_uint, test_parse_operand_uint_decimal_malformed);
    tcase_add_test(tc_uint, test_parse_operand_uint_binary);
    tcase_add_test(tc_uint, test_parse_operand_uint_hexadecimal);
    tcase_add_test(tc_uint, test_parse_operand_uint_decimal);

    TCase* tc_character_literal = tcase_create("character literal");
    tcase_add_test(tc_uint, test_parse_operand_character_literal_too_large);
    tcase_add_test(tc_uint, test_parse_operand_character_literal_empty);
    tcase_add_test(tc_uint, test_parse_operand_character_literal_1_byte);
    tcase_add_test(tc_uint, test_parse_operand_character_literal_multi_byte);
    tcase_add_test(tc_uint, test_parse_operand_character_literal_backslash);
    tcase_add_test(tc_uint, test_parse_operand_character_literal_single_quote);
    tcase_add_test(tc_uint, test_parse_operand_character_literal_newline);
    tcase_add_test(tc_uint, test_parse_operand_character_literal_tab);
    tcase_add_test(tc_uint, test_parse_operand_character_literal_mix);

    TCase* tc_symbol = tcase_create("symbol");
    tcase_add_test(tc_symbol, test_parse_operand_symbol_invalid);
    tcase_add_test(tc_symbol, test_parse_operand_symbol);

    suite_add_tcase(suite, tc_register);
    suite_add_tcase(suite, tc_uint);
    suite_add_tcase(suite, tc_character_literal);
    suite_add_tcase(suite, tc_symbol);

    return suite;
}

// ================================================================
//  parse_expression
// ================================================================

// --- errors ---

START_TEST(test_expression_integer_too_large_positive) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("9223372036854775808") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NOT_ENCODABLE);
    ck_assert_str_eq(err.not_encodable.message, "integer literal is too large");
} END_TEST

START_TEST(test_expression_integer_too_large_negative) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("-9223372036854775809") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NOT_ENCODABLE);
    ck_assert_str_eq(err.not_encodable.message, "integer literal is too large");
} END_TEST

START_TEST(test_expression_empty) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "expression");
} END_TEST

START_TEST(test_expression_lone_unary) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("-") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "expression");

    line = (Tokenizer){ .line = slice_from_cstring("+") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "expression");

    line = (Tokenizer){ .line = slice_from_cstring("~") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "expression");
} END_TEST

START_TEST(test_expression_lone_open_paren) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("(") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "expression");
} END_TEST

START_TEST(test_expression_missing_close_paren) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("(1 + 1") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "')'");
} END_TEST

START_TEST(test_expression_empty_paren) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("()") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "expression");
    ck_assert_str_eq(err.unexpected_token.found, "')'");
} END_TEST

START_TEST(test_expression_undefined) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = map_init();
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("foo") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNDEFINED_IDENTIFIER);

    map_free(&defines);
} END_TEST

START_TEST(test_expression_incomplete) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("1 +") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "expression");
} END_TEST

START_TEST(test_expression_unexpected) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("1 + !") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "expression");
    ck_assert_str_eq(err.unexpected_token.found, "'!'");
} END_TEST

START_TEST(test_expression_negative_shift_amount) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("1 << -1") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NEGATIVE_SHIFT_AMOUNT);

    line = (Tokenizer){ .line = slice_from_cstring("1 >> -1") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NEGATIVE_SHIFT_AMOUNT);

    line = (Tokenizer){ .line = slice_from_cstring("1 >> (1 - 10)") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NEGATIVE_SHIFT_AMOUNT);
} END_TEST

// --- unary expressions ---

START_TEST(test_expression_integer) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("1") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 1);

    line = (Tokenizer){ .line = slice_from_cstring("9223372036854775807") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 9223372036854775807);
} END_TEST

START_TEST(test_expression_character_literal) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("'A'") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 'A');

    line = (Tokenizer){ .line = slice_from_cstring("'ABC'") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, ('A' << 0) + ('B' << 8) + ('C' << 16));
} END_TEST

START_TEST(test_expression_integer_unary_plus) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("+1") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, +1);

    line = (Tokenizer){ .line = slice_from_cstring("+9223372036854775807") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, +9223372036854775807);
} END_TEST

START_TEST(test_expression_integer_unary_minus) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("-1") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, -1);

    line = (Tokenizer){ .line = slice_from_cstring("-9223372036854775808") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, -9223372036854775808u);
} END_TEST

// --- binary expressions ---

START_TEST(test_expression_sum) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("1 + 1") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 1 + 1);

    line = (Tokenizer){ .line = slice_from_cstring("123 + 456") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 123 + 456);
} END_TEST

START_TEST(test_expression_difference) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("1 - 1") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 1 - 1);

    line = (Tokenizer){ .line = slice_from_cstring("123 - 456") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 123 - 456);
} END_TEST

START_TEST(test_expression_and) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("1 & 1") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 1 & 1);

    line = (Tokenizer){ .line = slice_from_cstring("123 & 456") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 123 & 456);
} END_TEST

START_TEST(test_expression_or) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("1 | 1") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 1 | 1);

    line = (Tokenizer){ .line = slice_from_cstring("123 | 456") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 123 | 456);
} END_TEST

START_TEST(test_expression_xor) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("1 ^ 1") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 1 ^ 1);

    line = (Tokenizer){ .line = slice_from_cstring("123 ^ 456") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 123 ^ 456);
} END_TEST

START_TEST(test_expression_left_shift) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("1 << 1") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 1 << 1);

    line = (Tokenizer){ .line = slice_from_cstring("0xCAFE << 8") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 0xCAFE << 8);
} END_TEST

START_TEST(test_expression_right_shift) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("0x8 >> 2") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 0x8 >> 2);

    line = (Tokenizer){ .line = slice_from_cstring("0xCAFE >> 8") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 0xCAFE >> 8);
} END_TEST

START_TEST(test_expression_product) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("10 * 2") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 10 * 2);

    line = (Tokenizer){ .line = slice_from_cstring("123 * 456") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 123 * 456);
} END_TEST

START_TEST(test_expression_quotient) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("10 / 2") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 10 / 2);

    line = (Tokenizer){ .line = slice_from_cstring("456 / 123") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 456 / 123);
} END_TEST

START_TEST(test_expression_modulus) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("10 % 2") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 10 % 2);

    line = (Tokenizer){ .line = slice_from_cstring("456 % 123") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 456 % 123);
} END_TEST

// --- precedence ---

START_TEST(test_expression_precedence) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("10 + 2 * 3") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 10 + 2 * 3);

    line = (Tokenizer){ .line = slice_from_cstring("456 + 123 * 27") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 456 + 123 * 27);
} END_TEST

START_TEST(test_expression_one_in_parens) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("(1)") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, (1));
}

START_TEST(test_expression_parenthesis) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = {};
    int64_t value;

    line = (Tokenizer){ .line = slice_from_cstring("(10 + 2) * 3") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, (10 + 2) * 3);

    line = (Tokenizer){ .line = slice_from_cstring("(456 + 123) * 27") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, (456 + 123) * 27);

    line = (Tokenizer){ .line = slice_from_cstring("-(1)") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, -(1));

    line = (Tokenizer){ .line = slice_from_cstring("+'x' * -(2 + ((456 - (10 << 4)) * 123)) / -27)") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, +'x' * -(2 + ((456 - (10 << 4)) * 123)) / -27);
} END_TEST

// --- defines ---

START_TEST(test_expression_one_define) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = map_init();
    int64_t value;

    StringSlice foo = slice_from_cstring("foo");
    map_insert(&defines, foo, 1);

    line = (Tokenizer){ .line = slice_from_cstring("foo") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 1);

    line = (Tokenizer){ .line = slice_from_cstring("-foo") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, -1);

    line = (Tokenizer){ .line = slice_from_cstring("-(foo)") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, -1);

    map_free(&defines);
} END_TEST

START_TEST(test_expression_two_defines) {
    Tokenizer line = {};
    LineError err = {};
    StringToIntMap defines = map_init();
    int64_t value;

    StringSlice foo = slice_from_cstring("foo");
    StringSlice bar = slice_from_cstring("bar");
    map_insert(&defines, foo, 123);
    map_insert(&defines, bar, 4567);

    line = (Tokenizer){ .line = slice_from_cstring("foo | bar") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 123 | 4567);

    line = (Tokenizer){ .line = slice_from_cstring("-(foo ^ -(-bar))") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, -(123 ^ -(-4567)));

    line = (Tokenizer){ .line = slice_from_cstring("foo*(foo&foo>>2)-bar") };
    value = parse_expression(&line, &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_int_eq(value, 123*(123&123>>2)-4567);

    map_free(&defines);
} END_TEST

Suite* parse_expression_suite(void) {
    Suite* suite = suite_create("parse_expression");

    TCase* tc_error = tcase_create("error");
    tcase_add_test(tc_error, test_expression_integer_too_large_positive);
    tcase_add_test(tc_error, test_expression_integer_too_large_negative);
    tcase_add_test(tc_error, test_expression_empty);
    tcase_add_test(tc_error, test_expression_lone_unary);
    tcase_add_test(tc_error, test_expression_lone_open_paren);
    tcase_add_test(tc_error, test_expression_missing_close_paren);
    tcase_add_test(tc_error, test_expression_empty_paren);
    tcase_add_test(tc_error, test_expression_undefined);
    tcase_add_test(tc_error, test_expression_incomplete);
    tcase_add_test(tc_error, test_expression_unexpected);
    tcase_add_test(tc_error, test_expression_negative_shift_amount);

    TCase* tc_unary = tcase_create("unary");
    tcase_add_test(tc_unary, test_expression_integer);
    tcase_add_test(tc_unary, test_expression_character_literal);
    tcase_add_test(tc_unary, test_expression_integer_unary_plus);
    tcase_add_test(tc_unary, test_expression_integer_unary_minus);

    TCase* tc_binary = tcase_create("binary");
    tcase_add_test(tc_binary, test_expression_sum);
    tcase_add_test(tc_binary, test_expression_difference);
    tcase_add_test(tc_binary, test_expression_and);
    tcase_add_test(tc_binary, test_expression_or);
    tcase_add_test(tc_binary, test_expression_xor);
    tcase_add_test(tc_binary, test_expression_left_shift);
    tcase_add_test(tc_binary, test_expression_right_shift);
    tcase_add_test(tc_binary, test_expression_product);
    tcase_add_test(tc_binary, test_expression_quotient);
    tcase_add_test(tc_binary, test_expression_modulus);

    TCase* tc_precedence = tcase_create("precedence");
    tcase_add_test(tc_precedence, test_expression_precedence);
    tcase_add_test(tc_precedence, test_expression_one_in_parens);
    tcase_add_test(tc_precedence, test_expression_parenthesis);

    TCase* tc_defines = tcase_create("defines");
    tcase_add_test(tc_defines, test_expression_one_define);
    tcase_add_test(tc_defines, test_expression_two_defines);

    suite_add_tcase(suite, tc_error);
    suite_add_tcase(suite, tc_unary);
    suite_add_tcase(suite, tc_binary);
    suite_add_tcase(suite, tc_precedence);
    suite_add_tcase(suite, tc_defines);

    return suite;
}

// ================================================================
//  main
// ================================================================

 int main(void) {
    SRunner* suite_runner = srunner_create(tokenizer_suite());
    srunner_add_suite(suite_runner, parse_operand_suite());
    srunner_add_suite(suite_runner, parse_expression_suite());

    srunner_run_all(suite_runner, CK_NORMAL);
    int number_failed = srunner_ntests_failed(suite_runner);
    srunner_free(suite_runner);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }
