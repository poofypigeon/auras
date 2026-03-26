#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <check.h>

#include "../src/line_error.h"
#include "../src/parsing.h"
#include "../src/string_slice.h"
#include "../src/encode_instruction.h"

static bool produces_line_error(LineErrorType line_error, char* line_raw) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring(line_raw) };
    StringToIntMap defines = {};
    LineError err = {};

    uint32_t encoding = encode_instruction(&line, &defines, &err).machine_word;
    return (err.error_tag == line_error);
}

static uint32_t machine_word(char* line_raw) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring(line_raw) };
    StringToIntMap defines = {};
    LineError err = {};
    return encode_instruction(&line, &defines, &err).machine_word;
}

// ================================================================
//  M-Type
// ================================================================

START_TEST(test_m_type_unexpected_token) {
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0,"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, ["));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, []"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1,"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1,]"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1, t2"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1, t2 sll"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1, t2 sll]"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1, t2 sll 1"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1, 1"));

    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw!"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0!"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0,!"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [!"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1!"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1,!"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1, 1!"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1, t2!"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1, t2 sll!"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1, t2 sll 1!"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw t0, [t1, 10 sll 1]"));
} END_TEST

START_TEST(test_m_type_unexpected_eol) {
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw\"foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0\"foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0,\"foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, [\"foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, [t1\"foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, [t1,\"foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, [t1, t2\"foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, [t1, t2 sll\"foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, [t1, t2 sll 1\"foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, [t1, 1\"foo"));

    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw'foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0'foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0,'foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, ['foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, [t1'foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, [t1,'foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, [t1, t2'foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, [t1, t2 sll'foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, [t1, t2 sll 1 'foo"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    lw t0, [t1, 1 'foo"));
} END_TEST

START_TEST(test_m_type_literal_too_large) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    lw t0, [t1, 0xf_ffff_ffff_ffff_ffff]"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    lw t0, [t1, 'abcdefghi']"));
} END_TEST

START_TEST(test_m_type_exceeds_register_width) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    lw t0, [t1, 0xffff_ffff_ffff]"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    lw t0, [t1, 0xffff_ffff + 1]"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    lw t0, [t1, 'abcde']"));
} END_TEST

START_TEST(test_m_type_immediate_too_wide) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    lw x1, [x2, 0x2AA00]"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    lw x1, [x2, 0x34100]"));
} END_TEST

START_TEST(test_m_type_exceeds_maximum_shift_amount) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    lw t0, [t1, 0xff00_0000]"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    lw t0, [t1, 0x0800_0000]"));
} END_TEST

START_TEST(test_m_type_negative_shift_amount) {
    ck_assert(produces_line_error(LINE_ERROR_NEGATIVE_SHIFT_AMOUNT, "    lw t0, [t1, t2 sll -1]!"));
    ck_assert(produces_line_error(LINE_ERROR_NEGATIVE_SHIFT_AMOUNT, "    lw t0, [t1, -t2 sll 1 - 19]"));
} END_TEST

START_TEST(test_m_type_negative_shift_amount_expression) {
    ck_assert(produces_line_error(LINE_ERROR_NEGATIVE_SHIFT_AMOUNT, "    lw t0, [t1, 200 >> -(100 << 4-1)]"));
} END_TEST

START_TEST(test_m_type_unknown_escape_sequence) {
    ck_assert(produces_line_error(LINE_ERROR_UNKNOWN_ESCAPE_SEQUENCE, "    lw t0, [t1, 'x\\xx']"));
    ck_assert(produces_line_error(LINE_ERROR_UNKNOWN_ESCAPE_SEQUENCE, "    lw t0, [t1, 'x\\.x']"));
} END_TEST

START_TEST(test_m_type_undefined_identifier) {
    ck_assert(produces_line_error(LINE_ERROR_UNDEFINED_IDENTIFIER, "    lw t0, [t1, t2 sll ~foo * bar]"));
    ck_assert(produces_line_error(LINE_ERROR_UNDEFINED_IDENTIFIER, "    lw t0, [t1, _]"));
} END_TEST

START_TEST(test_m_type_sll_on_immediate) {
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    lw x1, [x2, 0x0011 sll 2]"));
} END_TEST

START_TEST(test_m_type_mnemonic_variants) {
    ck_assert_uint_eq(machine_word("    lw  x1, [x2]"), 0x01020000);
    ck_assert_uint_eq(machine_word("    lb  x1, [x2]"), 0x01220000);
    ck_assert_uint_eq(machine_word("    lbu x1, [x2]"), 0x01221000);
    ck_assert_uint_eq(machine_word("    lh  x1, [x2]"), 0x01420000);
    ck_assert_uint_eq(machine_word("    lhu x1, [x2]"), 0x01421000);
    ck_assert_uint_eq(machine_word("    sw  x1, [x2]"), 0x01022000);
    ck_assert_uint_eq(machine_word("    sb  x1, [x2]"), 0x01222000);
    ck_assert_uint_eq(machine_word("    sh  x1, [x2]"), 0x01422000);
} END_TEST

START_TEST(test_m_type_register_offset) {
    ck_assert_uint_eq(machine_word("    lw x1, [sp, t0]"),        0x01020003);
    ck_assert_uint_eq(machine_word("    lw s0, [x2, -x3]"),       0x10820003);
    ck_assert_uint_eq(machine_word("    lw x1, [x2, x3 sll 4]"),  0x01020403);
    ck_assert_uint_eq(machine_word("    lw x1, [x2, -x3 sll 4]"), 0x01820403);
    ck_assert_uint_eq(machine_word("    lw x1, [x2, x3 sll 15]"), 0x01020F03);
    ck_assert_uint_eq(machine_word("    lw x1, [x2, x3 sll 0]"),  0x01020003);
} END_TEST

START_TEST(test_m_type_immediate) {
    ck_assert_uint_eq(machine_word("    lw x1, [x2, 0xAA]"),  0x010280AA);
    ck_assert_uint_eq(machine_word("    lw x1, [x2, -0xAA]"), 0x01828056);
    ck_assert_uint_eq(machine_word("    lw x1, [x2, 0xFF]"),  0x010280FF);
    ck_assert_uint_eq(machine_word("    lw x1, [x2, -256]"),  0x01828000);
} END_TEST

START_TEST(test_m_type_writeback_register_offset) {
    ck_assert_uint_eq(machine_word("    lw x1, [x2, x3]!"),        0x01024003);
    ck_assert_uint_eq(machine_word("    lw x1, [x2, -x3]!"),       0x01824003);
    ck_assert_uint_eq(machine_word("    lw x1, [x2, x3 sll 4]!"),  0x01024403);
    ck_assert_uint_eq(machine_word("    lw x1, [x2, -x3 sll 4]!"), 0x01824403);
} END_TEST

START_TEST(test_m_type_writeback_immediate) {
    ck_assert_uint_eq(machine_word("    lw x1, [x2, 0xAA]!"), 0x0102C0AA);
} END_TEST

START_TEST(test_m_type_assembler_auto_shift) {
    ck_assert_uint_eq(machine_word("    lw x1, [x2, 0x550]"),   0x010283AA);
    ck_assert_uint_eq(machine_word("    lw x1, [x2, 0x14400]"), 0x010289A2);
    ck_assert_uint_eq(machine_word("    lw x1, [x2, 0x6480]"),  0x010287C9);
} END_TEST

START_TEST(test_m_type_auto_shift_negative_literal) {
    ck_assert_uint_eq(machine_word("    lw x1, [x2, -0x15800]"), 0x01828954);
} END_TEST

START_TEST(test_m_type_auto_shift_positive_msb_literal) {
    ck_assert_uint_eq(machine_word("    lw x1, [x2, 0xFFFEA800]"), 0x01828954);
} END_TEST

START_TEST(test_m_type_expression) {
    ck_assert_uint_eq(machine_word("    lw x1, [x2, 0xA0 + 0x0A]"),   0x010280AA);
    ck_assert_uint_eq(machine_word("    lw x1, [x2, x3 sll 20 - 5]"), 0x01020F03);
} END_TEST

START_TEST(test_m_type_defines) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("    lw x1, [x2, foo]!") };
    StringToIntMap defines = map_init();
    map_insert(&defines, slice_from_cstring("foo"), 0xAA);
    LineError err = {};
    ck_assert_uint_eq(encode_instruction(&line, &defines, &err).machine_word, 0x0102C0AA);
    map_free(&defines);
} END_TEST

Suite* m_type_suite(void) {
    Suite* suite = suite_create("M-Type");

    TCase* tc_errors = tcase_create("errors");
    tcase_add_test(tc_errors, test_m_type_unexpected_token);
    tcase_add_test(tc_errors, test_m_type_unexpected_eol);
    tcase_add_test(tc_errors, test_m_type_literal_too_large);
    tcase_add_test(tc_errors, test_m_type_exceeds_register_width);
    tcase_add_test(tc_errors, test_m_type_immediate_too_wide);
    tcase_add_test(tc_errors, test_m_type_negative_shift_amount);
    tcase_add_test(tc_errors, test_m_type_exceeds_maximum_shift_amount);
    tcase_add_test(tc_errors, test_m_type_undefined_identifier);
    tcase_add_test(tc_errors, test_m_type_sll_on_immediate);

    TCase* tc_encodings = tcase_create("encodings");
    tcase_add_test(tc_encodings, test_m_type_mnemonic_variants);
    tcase_add_test(tc_encodings, test_m_type_register_offset);
    tcase_add_test(tc_encodings, test_m_type_immediate);
    tcase_add_test(tc_encodings, test_m_type_writeback_register_offset);
    tcase_add_test(tc_encodings, test_m_type_writeback_immediate);
    tcase_add_test(tc_encodings, test_m_type_assembler_auto_shift);
    tcase_add_test(tc_encodings, test_m_type_auto_shift_negative_literal);
    tcase_add_test(tc_encodings, test_m_type_auto_shift_positive_msb_literal);
    tcase_add_test(tc_encodings, test_m_type_expression);
    tcase_add_test(tc_encodings, test_m_type_defines);

    suite_add_tcase(suite, tc_errors);
    suite_add_tcase(suite, tc_encodings);

    return suite;
}

// ================================================================
//  I-Type
// ================================================================

START_TEST(test_i_type_literal_too_large) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    mvi x1, 0xf_ffff_ffff_ffff_ffff"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    mvi x1, 'abcdefghi'"));
} END_TEST

START_TEST(test_i_type_exceeds_register_width) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    mvi x1, 0xffff_ffff_ffff"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    mvi x1, 0xffff_ffff + 1"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    mvi x1, 'abcde'"));
} END_TEST

START_TEST(test_i_type_mvi_bounds) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    mvi x1, 0x01FF_FFFF"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    mvi x1, -0x01FF_FFFF"));
} END_TEST

START_TEST(test_i_type_basic) {
    ck_assert_uint_eq(machine_word("    mvi x1, 0"),          0x21000000);
    ck_assert_uint_eq(machine_word("    mvi x1, 1"),          0x21000001);
    ck_assert_uint_eq(machine_word("    mvi x1, 0xAA"),       0x210000AA);
    ck_assert_uint_eq(machine_word("    mvi x1, -0xAA"),      0x21FFFF56);
    ck_assert_uint_eq(machine_word("    mvi x1, 0x7FFFFF"),   0x217FFFFF);
    ck_assert_uint_eq(machine_word("    mvi x1, -8388608"),   0x21800000);
    ck_assert_uint_eq(machine_word("    mvi x1, -1"),         0x21FFFFFF);
    ck_assert_uint_eq(machine_word("    mvi x1, 0xFFFFFFFF"), 0x21FFFFFF);
} END_TEST

START_TEST(test_i_type_expression) {
    ck_assert_uint_eq(machine_word("    mvi x1, 0x7FFF00 + 0xFF"), 0x217FFFFF);
} END_TEST

START_TEST(test_i_type_defines) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("    mvi x1, foo") };
    StringToIntMap defines = map_init();
    map_insert(&defines, slice_from_cstring("foo"), -0xAA);
    LineError err = {};
    ck_assert_uint_eq(encode_instruction(&line, &defines, &err).machine_word, 0x21FFFF56);
    map_free(&defines);
} END_TEST

Suite* i_type_suite(void) {
    Suite* suite = suite_create("I-Type");

    TCase* tc_errors = tcase_create("errors");
    tcase_add_test(tc_errors, test_i_type_literal_too_large);
    tcase_add_test(tc_errors, test_i_type_exceeds_register_width);
    tcase_add_test(tc_errors, test_i_type_mvi_bounds);

    TCase* tc_encodings = tcase_create("encodings");
    tcase_add_test(tc_encodings, test_i_type_basic);
    tcase_add_test(tc_encodings, test_i_type_expression);
    tcase_add_test(tc_encodings, test_i_type_defines);

    suite_add_tcase(suite, tc_errors);
    suite_add_tcase(suite, tc_encodings);

    return suite;
}

// ================================================================
//  main
// ================================================================

 int main(void) {
    SRunner* suite_runner = srunner_create(m_type_suite());
    srunner_add_suite(suite_runner, i_type_suite());

    srunner_run_all(suite_runner, CK_NORMAL);
    int number_failed = srunner_ntests_failed(suite_runner);
    srunner_free(suite_runner);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }
