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

static bool check_branch_label(char* line_raw, uint32_t expected_flags, char* expected_label) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring(line_raw) };
    StringToIntMap defines = {};
    LineError err = {};

    Instruction instr = encode_instruction(&line, &defines, &err);
    if (err.error_tag) return false;
    if (instr.machine_word != expected_flags) return false;
    return slice_eq(instr.label, slice_from_cstring(expected_label));
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
    ck_assert_uint_eq(machine_word("    mvi x1, 0xFFFF0000"), 0x21FF0000);
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
//  S-Type
// ================================================================

START_TEST(test_s_type_rsys_reserved) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    lsr x1, 63"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    ssr x1, 0, 63"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    ssr x1, x2, 63"));
} END_TEST

START_TEST(test_s_type_rsys_out_of_range) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    lsr x1, 64"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    lsr x1, 128"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    lsr x1, -1"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    ssr x1, 0, 64"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    ssr x1, x2, -1"));
} END_TEST

START_TEST(test_s_type_ssr_imm_out_of_range) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    ssr x1, 256, 0"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    ssr x1, -1, 0"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    ssr x1, 0x100, 5"));
} END_TEST

START_TEST(test_s_type_syscall_comment_out_of_range) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    syscall 256"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    syscall -1"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    syscall 0x1000"));
} END_TEST

START_TEST(test_s_type_lsr_basic) {
    ck_assert_uint_eq(machine_word("    lsr x0, 0"),   0x00600000);
    ck_assert_uint_eq(machine_word("    lsr x1, 0"),   0x01600000);
    ck_assert_uint_eq(machine_word("    lsr x1, 1"),   0x01600100);
    ck_assert_uint_eq(machine_word("    lsr x2, 5"),   0x02600500);
    ck_assert_uint_eq(machine_word("    lsr x31, 62"), 0x1F603E00);
    ck_assert_uint_eq(machine_word("    lsr t0, 10"),  0x03600A00);
    ck_assert_uint_eq(machine_word("    lsr sp, 0"),   0x02600000);
} END_TEST

START_TEST(test_s_type_ssr_immediate) {
    ck_assert_uint_eq(machine_word("    ssr x0, 0, 0"),     0x0060C000);
    ck_assert_uint_eq(machine_word("    ssr x1, 0, 0"),     0x0061C000);
    ck_assert_uint_eq(machine_word("    ssr x1, 0xAA, 0"),  0x0061C0AA);
    ck_assert_uint_eq(machine_word("    ssr x1, 0xFF, 5"),  0x0061C5FF);
    ck_assert_uint_eq(machine_word("    ssr x2, 128, 62"),  0x0062FE80);
    ck_assert_uint_eq(machine_word("    ssr t0, 0x42, 10"), 0x0063CA42);
    ck_assert_uint_eq(machine_word("    ssr sp, 255, 0"),   0x0062C0FF);
} END_TEST

START_TEST(test_s_type_ssr_register) {
    ck_assert_uint_eq(machine_word("    ssr x0, x0, 0"),  0x00604000);
    ck_assert_uint_eq(machine_word("    ssr x1, x0, 0"),  0x00614000);
    ck_assert_uint_eq(machine_word("    ssr x1, x2, 0"),  0x00614002);
    ck_assert_uint_eq(machine_word("    ssr x1, x31, 5"), 0x0061451F);
    ck_assert_uint_eq(machine_word("    ssr x2, x3, 62"), 0x00627E03);
    ck_assert_uint_eq(machine_word("    ssr t0, t1, 10"), 0x00634A04);
    ck_assert_uint_eq(machine_word("    ssr sp, s0, 0"),  0x00624010);
} END_TEST

START_TEST(test_s_type_syscall_basic) {
    ck_assert_uint_eq(machine_word("    syscall 0"),    0x00603F00);
    ck_assert_uint_eq(machine_word("    syscall 1"),    0x00603F01);
    ck_assert_uint_eq(machine_word("    syscall 0xAA"), 0x00603FAA);
    ck_assert_uint_eq(machine_word("    syscall 255"),  0x00603FFF);
    ck_assert_uint_eq(machine_word("    syscall 128"),  0x00603F80);
} END_TEST

START_TEST(test_s_type_expression) {
    ck_assert_uint_eq(machine_word("    lsr x1, 5 + 3"),          0x01600800);
    ck_assert_uint_eq(machine_word("    ssr x1, 0x10 + 0x20, 2"), 0x0061C230);
    ck_assert_uint_eq(machine_word("    syscall 100 + 55"),       0x00603F9B);
} END_TEST

START_TEST(test_s_type_defines) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("    lsr x1, CSR_STATUS") };
    StringToIntMap defines = map_init();
    map_insert(&defines, slice_from_cstring("CSR_STATUS"), 5);
    LineError err = {};
    ck_assert_uint_eq(encode_instruction(&line, &defines, &err).machine_word, 0x01600500);
    map_free(&defines);
} END_TEST

Suite* s_type_suite(void) {
    Suite* suite = suite_create("S-Type");

    TCase* tc_errors = tcase_create("errors");
    tcase_add_test(tc_errors, test_s_type_rsys_reserved);
    tcase_add_test(tc_errors, test_s_type_rsys_out_of_range);
    tcase_add_test(tc_errors, test_s_type_ssr_imm_out_of_range);
    tcase_add_test(tc_errors, test_s_type_syscall_comment_out_of_range);

    TCase* tc_encodings = tcase_create("encodings");
    tcase_add_test(tc_encodings, test_s_type_lsr_basic);
    tcase_add_test(tc_encodings, test_s_type_ssr_immediate);
    tcase_add_test(tc_encodings, test_s_type_ssr_register);
    tcase_add_test(tc_encodings, test_s_type_syscall_basic);
    tcase_add_test(tc_encodings, test_s_type_expression);
    tcase_add_test(tc_encodings, test_s_type_defines);

    suite_add_tcase(suite, tc_errors);
    suite_add_tcase(suite, tc_encodings);

    return suite;
}

// ================================================================
//  D-Type
// ================================================================

START_TEST(test_d_type_nop_unexpected_token) {
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    nop x1"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    nop 0xAA"));
} END_TEST

START_TEST(test_d_type_mov_unexpected_token) {
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    mov x1, x2, x3"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    mov x1, x2, 0xAA"));
} END_TEST

START_TEST(test_d_type_not_unexpected_token) {
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    not x1, x2, x3"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    not x1, x2, 0xAA"));
} END_TEST

START_TEST(test_d_type_no_rd_unexpected_token) {
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    tst x1, x2, x3"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    teq x1, x2, 0xAA"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    cmp x1, x2, x3 lsl 2"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    cpn x1, x2, 10 + 2"));
} END_TEST

START_TEST(test_d_type_invalid_shift_keep) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    srlk x1, x2, 3"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    srak x1, x2, 3"));
} END_TEST

START_TEST(test_d_type_shift_amount_out_of_range) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add x1, x2, x3 srl 33"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add x1, x2, x3 sra 33"));
} END_TEST

START_TEST(test_d_type_shift_on_immediate) {
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    add x1, x2, 0x0011 sll 2"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    add x1, x2, 0x0011 srl 2"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    add x1, x2, 0x0011 sra 2"));
}

START_TEST(test_d_type_right_shift_keep) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    addk x1, x2, x3 srl 1"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    addk x1, x2, x3 sra 1"));
} END_TEST

START_TEST(test_d_type_right_shift_zero) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add x1, x2, x3 srl 0"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add x1, x2, x3 sra 0"));
} END_TEST

START_TEST(test_d_type_standalone_right_shift_zero) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    srl x1, x2, 0"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    sra x1, x2, 0"));
} END_TEST

START_TEST(test_d_type_carry_in_immediate) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    adc x1, x2, 3"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    sbc x1, x2, 3"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    adck x1, x2, 3"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    sbck x1, x2, 3"));
} END_TEST

START_TEST(test_d_type_imm_out_of_range) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add x1, x2, -257"));
} END_TEST

START_TEST(test_d_type_literal_too_large) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add t0, t1, 0xf_ffff_ffff_ffff_ffff"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add t0, t1, 'abcdefghi'"));
} END_TEST

START_TEST(test_d_type_exceeds_register_width) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add t0, t1, 0xffff_ffff_ffff"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add t0, t1, 0xffff_ffff + 1"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add t0, t1, 'abcde'"));
} END_TEST

START_TEST(test_d_type_immediate_too_wide) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add x1, x2, 0x2AA00"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add x1, x2, 0x34100"));
} END_TEST

START_TEST(test_d_type_auto_shift_non_encodable_values) {
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add x1, x2, 0x0100_0001"));
    ck_assert(produces_line_error(LINE_ERROR_NOT_ENCODABLE, "    add x1, x2, -0x0100_0001"));
} END_TEST

START_TEST(test_d_type_negative_shift_amount) {
    ck_assert(produces_line_error(LINE_ERROR_NEGATIVE_SHIFT_AMOUNT, "    add t0, t1, t2 sll -1"));
    ck_assert(produces_line_error(LINE_ERROR_NEGATIVE_SHIFT_AMOUNT, "    add t0, t1, t2 sll 1 - 19"));
} END_TEST

START_TEST(test_d_type_negative_shift_amount_in_expression) {
    ck_assert(produces_line_error(LINE_ERROR_NEGATIVE_SHIFT_AMOUNT, "    add t0, t1, 200 >> -(100 << 4-1)"));
} END_TEST

START_TEST(test_d_type_unknown_escape_sequence) {
    ck_assert(produces_line_error(LINE_ERROR_UNKNOWN_ESCAPE_SEQUENCE, "    add t0, t1, 'x\\xx'"));
    ck_assert(produces_line_error(LINE_ERROR_UNKNOWN_ESCAPE_SEQUENCE, "    add t0, t1, 'x\\.x'"));
} END_TEST

START_TEST(test_d_type_undefined_identifier) {
    ck_assert(produces_line_error(LINE_ERROR_UNDEFINED_IDENTIFIER, "    add t0, t1, t2 sll ~foo * bar]"));
    ck_assert(produces_line_error(LINE_ERROR_UNDEFINED_IDENTIFIER, "    add t0, t1, _"));
} END_TEST

START_TEST(test_d_type_nop) {
    ck_assert_uint_eq(machine_word("    nop"), 0x40002000);
} END_TEST

START_TEST(test_d_type_mov) {
    ck_assert_uint_eq(machine_word("    mov x1, x2"), 0x41022000);
    ck_assert_uint_eq(machine_word("    mov x1, x1"), 0x41012000);
} END_TEST

START_TEST(test_d_type_not) {
    ck_assert_uint_eq(machine_word("    not x1, x2"), 0x41A280FF);
    ck_assert_uint_eq(machine_word("    not x1, x1"), 0x41A180FF);
} END_TEST

START_TEST(test_d_type_add_basic) {
    ck_assert_uint_eq(machine_word("    add x1, x2, 0"),     0x41028000);
    ck_assert_uint_eq(machine_word("    add x1, x2, 1"),     0x41028001);
    ck_assert_uint_eq(machine_word("    add x1, x2, 0xAA"),  0x410280AA);
    ck_assert_uint_eq(machine_word("    add x1, x2, -0xAA"), 0x41828056);
    ck_assert_uint_eq(machine_word("    add x1, x2, 255"),   0x410280FF);
    ck_assert_uint_eq(machine_word("    add x1, x2, -256"),  0x41828000);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3"),    0x41020003);
    ck_assert_uint_eq(machine_word("    add x31, x30, x29"), 0x5F1E001D);
} END_TEST

START_TEST(test_d_type_sub_basic) {
    ck_assert_uint_eq(machine_word("    sub x1, x2, x3"), 0x41820003);
    ck_assert_uint_eq(machine_word("    sub x1, x2, 3"),  0x418280fd);
} END_TEST

START_TEST(test_d_type_logical_ops) {
    ck_assert_uint_eq(machine_word("    and x1, x2, x3"),   0x41420003);
    ck_assert_uint_eq(machine_word("    and x1, x2, 0xAA"), 0x414280AA);
    ck_assert_uint_eq(machine_word("    or x1, x2, x3"),    0x41620003);
    ck_assert_uint_eq(machine_word("    or x1, x2, 0xAA"),  0x416280AA);
    ck_assert_uint_eq(machine_word("    xor x1, x2, x3"),   0x41220003);
    ck_assert_uint_eq(machine_word("    xor x1, x2, 0xAA"), 0x412280AA);
} END_TEST

START_TEST(test_d_type_compare_ops) {
    ck_assert_uint_eq(machine_word("    tst x2, x3"),   0x40420003);
    ck_assert_uint_eq(machine_word("    tst x2, 0xAA"), 0x404280AA);
    ck_assert_uint_eq(machine_word("    teq x2, x3"),   0x40220003);
    ck_assert_uint_eq(machine_word("    teq x2, 0xAA"), 0x402280AA);
    ck_assert_uint_eq(machine_word("    cmp x2, x3"),   0x40820003);
    ck_assert_uint_eq(machine_word("    cmp x2, 0xAA"), 0x40828056);
    ck_assert_uint_eq(machine_word("    cpn x2, x3"),   0x40020003);
    ck_assert_uint_eq(machine_word("    cpn x2, 0xAA"), 0x400280AA);
} END_TEST

START_TEST(test_d_type_carry_ops) {
    ck_assert_uint_eq(machine_word("    adc x1, x2, x3"), 0x41020083);
    ck_assert_uint_eq(machine_word("    sbc x1, x2, x3"), 0x41820083);
} END_TEST

START_TEST(test_d_type_keep_variants) {
    ck_assert_uint_eq(machine_word("    addk x1, x2, x3"), 0x41022003);
    ck_assert_uint_eq(machine_word("    addk x1, x2, 5"),  0x4102A005);
    ck_assert_uint_eq(machine_word("    subk x1, x2, x3"), 0x41822003);
    ck_assert_uint_eq(machine_word("    andk x1, x2, x3"), 0x41422003);
    ck_assert_uint_eq(machine_word("    andk x1, x2, 5"),  0x4142A005);
    ck_assert_uint_eq(machine_word("    ork  x1, x2, x3"), 0x41622003);
    ck_assert_uint_eq(machine_word("    ork  x1, x2, 5"),  0x4162A005);
    ck_assert_uint_eq(machine_word("    xork x1, x2, x3"), 0x41222003);
    ck_assert_uint_eq(machine_word("    xork x1, x2, 5"),  0x4122A005);
} END_TEST

START_TEST(test_d_type_carry_keep_variants) {
    ck_assert_uint_eq(machine_word("    adck x1, x2, x3"), 0x41022083);
    ck_assert_uint_eq(machine_word("    sbck x1, x2, x3"), 0x41822083);
} END_TEST

START_TEST(test_d_type_shift_immediate) {
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sll 0"),  0x41020003);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sll 5"),  0x41020503);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sll 31"), 0x41021F03);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 srl 1"),  0x41024103);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 srl 5"),  0x41024503);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 srl 31"), 0x41025F03);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 srl 32"), 0x41024003);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sra 1"),  0x41026103);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sra 5"),  0x41026503);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sra 32"), 0x41026003);
} END_TEST

START_TEST(test_d_type_shift_register) {
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sll x4"),  0xC1020403);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 srl x4"),  0xC1024403);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sra x4"),  0xC1026403);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sll x31"), 0xC1021F03);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sll x0"),  0xC1020003);
} END_TEST

START_TEST(test_d_type_shift_standalone) {
    ck_assert_uint_eq(machine_word("    sll x1, x3, 0"),  0x41000003);
    ck_assert_uint_eq(machine_word("    sll x1, x3, 5"),  0x41000503);
    ck_assert_uint_eq(machine_word("    sll x1, x3, x4"), 0xC1000403);
    ck_assert_uint_eq(machine_word("    srl x1, x3, 1"),  0x41004103);
    ck_assert_uint_eq(machine_word("    srl x1, x3, 32"), 0x41004003);
    ck_assert_uint_eq(machine_word("    srl x1, x3, x4"), 0xC1004403);
    ck_assert_uint_eq(machine_word("    sra x1, x3, 1"),  0x41006103);
    ck_assert_uint_eq(machine_word("    sra x1, x3, 32"), 0x41006003);
    ck_assert_uint_eq(machine_word("    sra x1, x3, x4"), 0xC1006403);
    ck_assert_uint_eq(machine_word("    sllk x1, x3, 5"), 0x41002503);
} END_TEST

START_TEST(test_d_type_shift_left_keep) {
    ck_assert_uint_eq(machine_word("    addk x1, x2, x3 sll x4"),  0xC1022403);
} END_TEST

START_TEST(test_d_type_shift_full_range) {
    ck_assert_uint_eq(machine_word("    add t0, t1, 0x7f00_0000"), 0x430497fe);
} END_TEST

START_TEST(test_d_type_all_shift_amounts) {
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sll 0"),  0x41020003);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sll 1"),  0x41020103);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sll 15"), 0x41020F03);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sll 16"), 0x41021003);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sll 30"), 0x41021E03);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sll 31"), 0x41021F03);
} END_TEST

START_TEST(test_d_type_expression) {
    ck_assert_uint_eq(machine_word("    add x1, x2, 10 + 5"),       0x4102800F);
    ck_assert_uint_eq(machine_word("    add x1, x2, x3 sll 2 + 3"), 0x41020503);
} END_TEST

START_TEST(test_d_type_defines) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("    add x1, x2, MY_CONST") };
    StringToIntMap defines = map_init();
    map_insert(&defines, slice_from_cstring("MY_CONST"), 42);
    LineError err = {};
    ck_assert_uint_eq(encode_instruction(&line, &defines, &err).machine_word, 0x4102802A);
    map_free(&defines);
} END_TEST

START_TEST(test_d_type_shift_with_defines) {
    Tokenizer line = (Tokenizer){ .line = slice_from_cstring("    add x1, x2, x3 sll SHIFT_AMT") };
    StringToIntMap defines = map_init();
    map_insert(&defines, slice_from_cstring("SHIFT_AMT"), 5);
    LineError err = {};
    ck_assert_uint_eq(encode_instruction(&line, &defines, &err).machine_word, 0x41020503);
    map_free(&defines);
} END_TEST

START_TEST(test_d_type_assembler_auto_shift) {
    ck_assert_uint_eq(machine_word("    add x1, x2, 0x550"),   0x410283AA);
    ck_assert_uint_eq(machine_word("    add x1, x2, 0x14400"), 0x410289A2);
    ck_assert_uint_eq(machine_word("    add x1, x2, 0x6480"),  0x410287C9);
} END_TEST

START_TEST(test_d_type_auto_shift_negative_literal) {
    ck_assert_uint_eq(machine_word("    add x1, x2, -0x15800"), 0x41828954);
} END_TEST

START_TEST(test_d_type_auto_shift_positive_msb_literal) {
    ck_assert_uint_eq(machine_word("    add x1, x2, 0xFFFEA800"), 0x41828954);
} END_TEST

Suite* d_type_suite(void) {
    Suite* suite = suite_create("D-Type");

    TCase* tc_errors = tcase_create("errors");
    // Syntax / arity errors
    tcase_add_test(tc_errors, test_d_type_nop_unexpected_token);
    tcase_add_test(tc_errors, test_d_type_mov_unexpected_token);
    tcase_add_test(tc_errors, test_d_type_not_unexpected_token);
    tcase_add_test(tc_errors, test_d_type_no_rd_unexpected_token);
    tcase_add_test(tc_errors, test_d_type_shift_on_immediate);

    // Shift-form constraints
    tcase_add_test(tc_errors, test_d_type_invalid_shift_keep);
    tcase_add_test(tc_errors, test_d_type_right_shift_keep);
    tcase_add_test(tc_errors, test_d_type_shift_amount_out_of_range);
    tcase_add_test(tc_errors, test_d_type_right_shift_zero);
    tcase_add_test(tc_errors, test_d_type_standalone_right_shift_zero);
    tcase_add_test(tc_errors, test_d_type_negative_shift_amount);
    tcase_add_test(tc_errors, test_d_type_negative_shift_amount_in_expression);

    // Operand / numeric encoding constraints
    tcase_add_test(tc_errors, test_d_type_carry_in_immediate);
    tcase_add_test(tc_errors, test_d_type_imm_out_of_range);
    tcase_add_test(tc_errors, test_d_type_immediate_too_wide);
    tcase_add_test(tc_errors, test_d_type_auto_shift_non_encodable_values);
    tcase_add_test(tc_errors, test_d_type_literal_too_large);
    tcase_add_test(tc_errors, test_d_type_exceeds_register_width);

    // Expression parser/identifier diagnostics
    tcase_add_test(tc_errors, test_d_type_unknown_escape_sequence);
    tcase_add_test(tc_errors, test_d_type_undefined_identifier);

    TCase* tc_encodings = tcase_create("encodings");
    // Pseudo forms
    tcase_add_test(tc_encodings, test_d_type_nop);
    tcase_add_test(tc_encodings, test_d_type_mov);
    tcase_add_test(tc_encodings, test_d_type_not);

    // Arithmetic / logical families
    tcase_add_test(tc_encodings, test_d_type_add_basic);
    tcase_add_test(tc_encodings, test_d_type_sub_basic);
    tcase_add_test(tc_encodings, test_d_type_logical_ops);
    tcase_add_test(tc_encodings, test_d_type_compare_ops);
    tcase_add_test(tc_encodings, test_d_type_carry_ops);
    tcase_add_test(tc_encodings, test_d_type_keep_variants);
    tcase_add_test(tc_encodings, test_d_type_carry_keep_variants);

    // Shift forms
    tcase_add_test(tc_encodings, test_d_type_shift_immediate);
    tcase_add_test(tc_encodings, test_d_type_shift_register);
    tcase_add_test(tc_encodings, test_d_type_shift_standalone);
    tcase_add_test(tc_encodings, test_d_type_shift_left_keep);
    tcase_add_test(tc_encodings, test_d_type_shift_full_range);
    tcase_add_test(tc_encodings, test_d_type_all_shift_amounts);

    // Expressions, defines, and assembler-assisted immediate packing
    tcase_add_test(tc_encodings, test_d_type_expression);
    tcase_add_test(tc_encodings, test_d_type_defines);
    tcase_add_test(tc_encodings, test_d_type_shift_with_defines);
    tcase_add_test(tc_encodings, test_d_type_assembler_auto_shift);
    tcase_add_test(tc_encodings, test_d_type_auto_shift_negative_literal);
    tcase_add_test(tc_encodings, test_d_type_auto_shift_positive_msb_literal);

    suite_add_tcase(suite, tc_errors);
    suite_add_tcase(suite, tc_encodings);

    return suite;
}

// ================================================================
//  B-Type
// ================================================================

START_TEST(test_b_type_missing_target) {
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    b"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    beq"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_TOKEN, "    bl"));
} END_TEST
    
START_TEST(test_b_type_invalid_token) {
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    b'invalid"));
    ck_assert(produces_line_error(LINE_ERROR_UNEXPECTED_EOL, "    beq\"invalid"));
} END_TEST

START_TEST(test_b_type_register_absolute) {
    ck_assert_uint_eq(machine_word("    b x5"),    0x87050000);
    ck_assert_uint_eq(machine_word("    beq x10"), 0x800A0000);
    ck_assert_uint_eq(machine_word("    bne x15"), 0x810F0000);
    ck_assert_uint_eq(machine_word("    blt x20"), 0x82140000);
    ck_assert_uint_eq(machine_word("    bge x25"), 0x83190000);
    ck_assert_uint_eq(machine_word("    blo x30"), 0x841E0000);
    ck_assert_uint_eq(machine_word("    bhs x31"), 0x851F0000);
    ck_assert_uint_eq(machine_word("    bmi x0"),  0x86000000);
} END_TEST

START_TEST(test_b_type_register_absolute_with_link) {
    ck_assert_uint_eq(machine_word("    bl x5"),    0x8F050000);
    ck_assert_uint_eq(machine_word("    bleq x10"), 0x880A0000);
    ck_assert_uint_eq(machine_word("    blne x15"), 0x890F0000);
    ck_assert_uint_eq(machine_word("    bllt x20"), 0x8A140000);
    ck_assert_uint_eq(machine_word("    blge x25"), 0x8B190000);
    ck_assert_uint_eq(machine_word("    bllo x30"), 0x8C1E0000);
    ck_assert_uint_eq(machine_word("    blhs x31"), 0x8D1F0000);
    ck_assert_uint_eq(machine_word("    blmi x0"),  0x8E000000);
} END_TEST

START_TEST(test_b_type_pc_relative_labels) {
    ck_assert(check_branch_label("    b my_label",                 0x97000000, "my_label"));
    ck_assert(check_branch_label("    beq loop_start",             0x90000000, "loop_start"));
    ck_assert(check_branch_label("    bne exit",                   0x91000000, "exit"));
    ck_assert(check_branch_label("    blt negative",               0x92000000, "negative"));
    ck_assert(check_branch_label("    bge positive",               0x93000000, "positive"));
    ck_assert(check_branch_label("    blo underflow",              0x94000000, "underflow"));
    ck_assert(check_branch_label("    bhs overflow",               0x95000000, "overflow"));
    ck_assert(check_branch_label("    bmi sign_bit",               0x96000000, "sign_bit"));
    ck_assert(check_branch_label("    bl function",                0x9F000000, "function"));
    ck_assert(check_branch_label("    bleq equal_handler",         0x98000000, "equal_handler"));
    ck_assert(check_branch_label("    blne not_equal_handler",     0x99000000, "not_equal_handler"));
    ck_assert(check_branch_label("    bllt less_than_handler",     0x9A000000, "less_than_handler"));
    ck_assert(check_branch_label("    blge greater_equal_handler", 0x9B000000, "greater_equal_handler"));
    ck_assert(check_branch_label("    bllo lower_handler",         0x9C000000, "lower_handler"));
    ck_assert(check_branch_label("    blhs higher_same_handler",   0x9D000000, "higher_same_handler"));
    ck_assert(check_branch_label("    blmi minus_handler",         0x9E000000, "minus_handler"));
} END_TEST

Suite* b_type_suite(void) {
    Suite* suite = suite_create("B-Type");

    TCase* tc_errors = tcase_create("errors");
    tcase_add_test(tc_errors, test_b_type_missing_target);
    tcase_add_test(tc_errors, test_b_type_invalid_token);

    TCase* tc_encodings = tcase_create("encodings");
    tcase_add_test(tc_encodings, test_b_type_register_absolute);
    tcase_add_test(tc_encodings, test_b_type_register_absolute_with_link);
    tcase_add_test(tc_encodings, test_b_type_pc_relative_labels);

    suite_add_tcase(suite, tc_encodings);
    suite_add_tcase(suite, tc_errors);

    return suite;
}

// ================================================================
//  main
// ================================================================

int main(void) {
    SRunner* suite_runner = srunner_create(m_type_suite());
    srunner_add_suite(suite_runner, i_type_suite());
    srunner_add_suite(suite_runner, s_type_suite());
    srunner_add_suite(suite_runner, d_type_suite());
    srunner_add_suite(suite_runner, b_type_suite());

    srunner_run_all(suite_runner, CK_NORMAL);
    int number_failed = srunner_ntests_failed(suite_runner);
    srunner_free(suite_runner);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
