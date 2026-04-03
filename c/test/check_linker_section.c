#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <check.h>

#include "../src/endianness.h"
#include "../src/line_error.h"
#include "../src/linker_section.h"
#include "../src/string_slice.h"
#include "../src/string_to_int_map.h"

static bool produces_unexpected_token_error(LinkerSection* section, char* line_raw) {
    StringToIntMap defines = {};
    LineError err = {};
    process_line(section, slice_from_cstring(line_raw), &defines, &err);
    return err.error_tag == LINE_ERROR_UNEXPECTED_TOKEN;
}

static bool produces_line_error(LinkerSection* section, char* line_raw, LineErrorType expected) {
    StringToIntMap defines = {};
    LineError err = {};
    process_line(section, slice_from_cstring(line_raw), &defines, &err);
    return err.error_tag == expected;
}

static LineError line_error_from_process_line(LinkerSection* section, char* line_raw) {
    StringToIntMap defines = {};
    LineError err = {};
    process_line(section, slice_from_cstring(line_raw), &defines, &err);
    return err;
}

static void expect_u32le_word_bytes(CharBuffer buffer, size_t word_index, uint32_t expected) {
    union { uint32_t w; uint8_t b[4]; } expected_le = { .w = htole32(expected) };
    ck_assert_uint_eq((uint8_t)buffer.items[word_index*4 + 0], expected_le.b[0]);
    ck_assert_uint_eq((uint8_t)buffer.items[word_index*4 + 1], expected_le.b[1]);
    ck_assert_uint_eq((uint8_t)buffer.items[word_index*4 + 2], expected_le.b[2]);
    ck_assert_uint_eq((uint8_t)buffer.items[word_index*4 + 3], expected_le.b[3]);
}

static void expect_string_entry(CharBuffer string_table, size_t index, const char* s) {
    StringSlice expected = slice_from_cstring(s);
    ck_assert_uint_eq((uint8_t)string_table.items[index], expected.length);
    for (size_t i = 0; i < expected.length; i++) {
        ck_assert_int_eq(string_table.items[index + 1 + i], expected.bytes[i]);
    }
}

// ================================================================
//  Core process_line behavior
// ================================================================

START_TEST(test_missing_section_declaration) {
    StringToIntMap defines = {};
    LineError err = {};
    LineContent line_content = process_line(nullptr, slice_from_cstring("anything"), &defines, &err);

    ck_assert(line_content == LINE_HAS_SECTION_CONTENT);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_MISSING_SECTION_DECLARATION);
} END_TEST

START_TEST(test_empty_line) {
    LinkerSection section = linker_section_init();

    StringToIntMap defines = {};
    LineError err = {};

    ck_assert(process_line(&section, slice_from_cstring(""), &defines, &err) == LINE_HAS_SECTION_CONTENT);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    err = (LineError){};
    ck_assert(process_line(&section, slice_from_cstring("    "), &defines, &err) == LINE_HAS_SECTION_CONTENT);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    err = (LineError){};
    ck_assert(process_line(&section, slice_from_cstring("; some comment"), &defines, &err) == LINE_HAS_SECTION_CONTENT);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    err = (LineError){};
    ck_assert(process_line(&section, slice_from_cstring("    ; some comment"), &defines, &err) == LINE_HAS_SECTION_CONTENT);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    ck_assert_uint_eq(section.buffer.length, 0);
    ck_assert_uint_eq(section.symbol_table.length, 0);
    ck_assert_uint_eq(section.relocation_table.length, 0);
    ck_assert_uint_eq(section.string_table.length, 0);
    ck_assert_uint_eq(section.symbol_map.size, 0);

    linker_section_free(&section);
} END_TEST

// ================================================================
//  Local label handling
// ================================================================

START_TEST(test_local_label_non_label_character) {
    LinkerSection section = linker_section_init();
    ck_assert(produces_unexpected_token_error(&section, "0"));
    linker_section_free(&section);
} END_TEST

START_TEST(test_local_label_missing_colon) {
    LinkerSection section = linker_section_init();
    ck_assert(produces_unexpected_token_error(&section, "L1"));
    linker_section_free(&section);
} END_TEST

START_TEST(test_local_label_unexpected_token) {
    LinkerSection section = linker_section_init();
    ck_assert(produces_unexpected_token_error(&section, "L1:!"));
    linker_section_free(&section);
} END_TEST

START_TEST(test_local_label) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    LineContent line_content = process_line(&section, slice_from_cstring("L1:"), &defines, &err);
    ck_assert(line_content == LINE_HAS_SECTION_CONTENT);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    ck_assert_uint_eq(section.buffer.length, 0);

    ck_assert_uint_eq(section.symbol_table.length, 1);
    ck_assert_uint_eq(section.symbol_table.items[0].section_offset, 0);
    ck_assert_uint_eq(section.symbol_table.items[0].string_table_start_index, 0);

    ck_assert_uint_eq(section.relocation_table.length, 0);

    ck_assert_uint_eq(section.string_table.length, 3);
    expect_string_entry(section.string_table, 0, "L1");

    int64_t index = -1;
    ck_assert(map_find(&section.symbol_map, slice_from_cstring("L1"), &index));
    ck_assert_int_eq(index, 0);

    linker_section_free(&section);
} END_TEST

START_TEST(test_local_label_redefinition) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("L1:"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    err = (LineError){};
    process_line(&section, slice_from_cstring("L1:"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_REDEFINITION);

    linker_section_free(&section);
} END_TEST

START_TEST(test_local_label_exceeds_max_length) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    char line[260];
    for (size_t i = 0; i < 256; i++) line[i] = 'a';
    line[256] = ':';
    line[257] = '\0';

    process_line(&section, slice_from_cstring(line), &defines, &err);

    ck_assert_int_eq(err.error_tag, LINE_ERROR_NOT_ENCODABLE);
    ck_assert_ptr_nonnull(err.not_encodable.message);
    ck_assert_str_eq(err.not_encodable.message, "label exceeds max length (255)");

    linker_section_free(&section);
} END_TEST

START_TEST(test_local_label_max_length_255) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    char line[258];
    for (size_t i = 0; i < 255; i++) line[i] = 'a';
    line[255] = ':';
    line[256] = '\0';

    process_line(&section, slice_from_cstring(line), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    ck_assert_uint_eq(section.symbol_table.length, 1);

    linker_section_free(&section);
} END_TEST

// ================================================================
//  Instruction dispatch and encoding integration
// ================================================================

START_TEST(test_invalid_mnemonic) {
    LinkerSection section = linker_section_init();
    ck_assert(produces_unexpected_token_error(&section, "    bad"));
    linker_section_free(&section);
} END_TEST

START_TEST(test_instruction_extraneous_token) {
    LinkerSection section = linker_section_init();

    ck_assert(produces_unexpected_token_error(&section, "    b label!"));
    ck_assert(produces_unexpected_token_error(&section, "    nop!"));
    ck_assert(produces_unexpected_token_error(&section, "    add x1, x2, x3 sll x4!"));
    ck_assert(produces_unexpected_token_error(&section, "    add x1, x2, x3 sll 4!"));
    ck_assert(produces_unexpected_token_error(&section, "    sll x1, x2, x3!"));
    ck_assert(produces_unexpected_token_error(&section, "    b x1!"));
    ck_assert(produces_unexpected_token_error(&section, "    mov x1, x2!"));
    ck_assert(produces_unexpected_token_error(&section, "    mvi32 x1, 0!"));
    ck_assert(produces_unexpected_token_error(&section, "    syscall 0xAA!"));
    ck_assert(produces_unexpected_token_error(&section, "    mvi x1, 0!"));
    ck_assert(produces_unexpected_token_error(&section, "    lsr x1, x2!"));
    ck_assert(produces_unexpected_token_error(&section, "    ssr x1, x2!"));
    ck_assert(produces_unexpected_token_error(&section, "    lw x1, [x2]."));

    linker_section_free(&section);
} END_TEST

START_TEST(test_instruction_alignment) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    byte 0x11, 0x22"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    err = (LineError){};
    process_line(&section, slice_from_cstring("    nop"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    ck_assert_uint_eq(section.buffer.length, 8);
    expect_u32le_word_bytes(section.buffer, 0, 0x00002211);
    expect_u32le_word_bytes(section.buffer, 1, 0x40002000);

    linker_section_free(&section);
} END_TEST

START_TEST(test_general_instruction) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    LineContent line_content = process_line(&section, slice_from_cstring("    mvi x1, 0xAA"), &defines, &err);
    ck_assert(line_content == LINE_HAS_SECTION_CONTENT);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    ck_assert_uint_eq(section.buffer.length, 4);
    expect_u32le_word_bytes(section.buffer, 0, 0x210000AA);

    ck_assert_uint_eq(section.symbol_table.length, 0);
    ck_assert_uint_eq(section.relocation_table.length, 0);
    ck_assert_uint_eq(section.string_table.length, 0);
    ck_assert_uint_eq(section.symbol_map.size, 0);

    linker_section_free(&section);
} END_TEST

// ================================================================
//  Relocation and symbol table behavior
// ================================================================

START_TEST(test_m32_integer_literal) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    mvi32 x1, 0xDEADBEEF"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    ck_assert_uint_eq(section.buffer.length, 8);
    expect_u32le_word_bytes(section.buffer, 0, 0x212DBEEF);
    expect_u32le_word_bytes(section.buffer, 1, 0x4181B7BD);

    ck_assert_uint_eq(section.symbol_table.length, 0);
    ck_assert_uint_eq(section.relocation_table.length, 0);
    ck_assert_uint_eq(section.string_table.length, 0);
    ck_assert_uint_eq(section.symbol_map.size, 0);

    linker_section_free(&section);
} END_TEST

START_TEST(test_m32_relocation) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    lda x1, L1"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    ck_assert_uint_eq(section.buffer.length, 8);
    expect_u32le_word_bytes(section.buffer, 0, 0x21000000);
    expect_u32le_word_bytes(section.buffer, 1, 0x4101B700);

    ck_assert_uint_eq(section.symbol_table.length, 1);
    ck_assert_uint_eq(section.symbol_table.items[0].section_offset, UNDEFINED_OFFSET);
    ck_assert_uint_eq(section.symbol_table.items[0].string_table_start_index, 0);

    ck_assert_uint_eq(section.relocation_table.length, 1);
    ck_assert_uint_eq(section.relocation_table.items[0].section_offset, 0);
    ck_assert_uint_eq(section.relocation_table.items[0].symbol_table_index, 0);

    ck_assert_uint_eq(section.string_table.length, 3);
    expect_string_entry(section.string_table, 0, "L1");

    int64_t index = -1;
    ck_assert(map_find(&section.symbol_map, slice_from_cstring("L1"), &index));
    ck_assert_int_eq(index, 0);

    linker_section_free(&section);
} END_TEST

START_TEST(test_branch_relocation) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    beq L1"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    ck_assert_uint_eq(section.buffer.length, 4);
    expect_u32le_word_bytes(section.buffer, 0, 0x90000000);

    ck_assert_uint_eq(section.symbol_table.length, 1);
    ck_assert_uint_eq(section.symbol_table.items[0].section_offset, UNDEFINED_OFFSET);
    ck_assert_uint_eq(section.symbol_table.items[0].string_table_start_index, 0);

    ck_assert_uint_eq(section.relocation_table.length, 1);
    ck_assert_uint_eq(section.relocation_table.items[0].section_offset, 0);
    ck_assert_uint_eq(section.relocation_table.items[0].symbol_table_index, 0);

    ck_assert_uint_eq(section.string_table.length, 3);
    expect_string_entry(section.string_table, 0, "L1");

    int64_t index = -1;
    ck_assert(map_find(&section.symbol_map, slice_from_cstring("L1"), &index));
    ck_assert_int_eq(index, 0);

    linker_section_free(&section);
} END_TEST

START_TEST(test_addr_relocation) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    addr L1"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    ck_assert_uint_eq(section.buffer.length, 4);
    expect_u32le_word_bytes(section.buffer, 0, 0x00000000);

    ck_assert_uint_eq(section.symbol_table.length, 1);
    ck_assert_uint_eq(section.symbol_table.items[0].section_offset, UNDEFINED_OFFSET);
    ck_assert_uint_eq(section.symbol_table.items[0].string_table_start_index, 0);

    ck_assert_uint_eq(section.relocation_table.length, 1);
    ck_assert_uint_eq(section.relocation_table.items[0].section_offset, 0);
    ck_assert_uint_eq(section.relocation_table.items[0].symbol_table_index, 0);

    ck_assert_uint_eq(section.string_table.length, 3);
    expect_string_entry(section.string_table, 0, "L1");

    int64_t index = -1;
    ck_assert(map_find(&section.symbol_map, slice_from_cstring("L1"), &index));
    ck_assert_int_eq(index, 0);

    linker_section_free(&section);
} END_TEST

START_TEST(test_addr_alignment) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    byte 0x11, 0x22"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    addr L1"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    ck_assert_uint_eq(section.buffer.length, 8);
    expect_u32le_word_bytes(section.buffer, 0, 0x00002211);
    expect_u32le_word_bytes(section.buffer, 1, 0x00000000);

    linker_section_free(&section);
} END_TEST

START_TEST(test_multiple_labels_and_relocations) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("L1:"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    lda x1, L2"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("L2:"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    beq L1"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    ck_assert_uint_eq(section.buffer.length, 12);
    expect_u32le_word_bytes(section.buffer, 0, 0x21000000);
    expect_u32le_word_bytes(section.buffer, 1, 0x4101B700);
    expect_u32le_word_bytes(section.buffer, 2, 0x90000000);

    ck_assert_uint_eq(section.symbol_table.length, 2);
    ck_assert_uint_eq(section.symbol_table.items[0].section_offset, 0);
    ck_assert_uint_eq(section.symbol_table.items[0].string_table_start_index, 0);
    ck_assert_uint_eq(section.symbol_table.items[1].section_offset, 8);
    ck_assert_uint_eq(section.symbol_table.items[1].string_table_start_index, 3);

    ck_assert_uint_eq(section.relocation_table.length, 2);
    ck_assert_uint_eq(section.relocation_table.items[0].section_offset, 0);
    ck_assert_uint_eq(section.relocation_table.items[0].symbol_table_index, 1);
    ck_assert_uint_eq(section.relocation_table.items[1].section_offset, 8);
    ck_assert_uint_eq(section.relocation_table.items[1].symbol_table_index, 0);

    ck_assert_uint_eq(section.string_table.length, 6);
    expect_string_entry(section.string_table, 0, "L1");
    expect_string_entry(section.string_table, 3, "L2");

    int64_t index = -1;
    ck_assert(map_find(&section.symbol_map, slice_from_cstring("L1"), &index));
    ck_assert_int_eq(index, 0);
    ck_assert(map_find(&section.symbol_map, slice_from_cstring("L2"), &index));
    ck_assert_int_eq(index, 1);

    linker_section_free(&section);
} END_TEST

START_TEST(test_relocation_symbol_deduplication) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    beq L1"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    beq L1"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    ck_assert_uint_eq(section.symbol_table.length, 1);
    ck_assert_uint_eq(section.relocation_table.length, 2);
    ck_assert_uint_eq(section.relocation_table.items[0].symbol_table_index, 0);
    ck_assert_uint_eq(section.relocation_table.items[1].symbol_table_index, 0);

    linker_section_free(&section);
} END_TEST

START_TEST(test_forward_relocation_resolves_on_definition) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    beq L1"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    beq L1"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("L1:"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    ck_assert_uint_eq(section.symbol_table.length, 1);
    ck_assert_uint_eq(section.symbol_table.items[0].section_offset, 8);
    ck_assert_uint_eq(section.relocation_table.length, 2);

    linker_section_free(&section);
} END_TEST

// ================================================================
//  Data word and syntax diagnostics
// ================================================================

START_TEST(test_static_data_out_of_range) {
    LinkerSection section = linker_section_init();

    ck_assert(produces_line_error(&section, "    word 0x1_0000_0000", LINE_ERROR_NOT_ENCODABLE));
    ck_assert(produces_line_error(&section, "    half 0x1_0000", LINE_ERROR_NOT_ENCODABLE));
    ck_assert(produces_line_error(&section, "    byte 0x100", LINE_ERROR_NOT_ENCODABLE));
    ck_assert(produces_line_error(&section, "    word -0x8000_0001", LINE_ERROR_NOT_ENCODABLE));
    ck_assert(produces_line_error(&section, "    half -0x8001", LINE_ERROR_NOT_ENCODABLE));
    ck_assert(produces_line_error(&section, "    byte -0x81", LINE_ERROR_NOT_ENCODABLE));

    linker_section_free(&section);
} END_TEST

START_TEST(test_static_data_unexpected_token) {
    LinkerSection section = linker_section_init();
    ck_assert(produces_unexpected_token_error(&section, "    word!"));
    ck_assert(produces_unexpected_token_error(&section, "    word 0,!"));
    linker_section_free(&section);
} END_TEST

START_TEST(test_expected_eol_errors) {
    LinkerSection section = linker_section_init();
    LineError err = {};

    err = line_error_from_process_line(&section, "    addr L1 extra");
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "'eol'");

    err = line_error_from_process_line(&section, "    ascii \"abc\" extra");
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "'eol'");

    err = line_error_from_process_line(&section, "    align 4 extra");
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "'eol'");

    err = line_error_from_process_line(&section, "    nop extra");
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "'eol'");

    err = line_error_from_process_line(&section, "    word 1 extra");
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "'eol'");

    err = line_error_from_process_line(&section, "L1: extra");
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "'eol'");

    linker_section_free(&section);
} END_TEST

START_TEST(test_addr_missing_symbol) {
    LinkerSection section = linker_section_init();
    LineError err = line_error_from_process_line(&section, "    addr");
    ck_assert_int_eq(err.error_tag, LINE_ERROR_UNEXPECTED_TOKEN);
    ck_assert_str_eq(err.unexpected_token.expected, "symbol");
    linker_section_free(&section);
} END_TEST

START_TEST(test_static_data_single_value) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    word 0xDEAD_BEEF"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    word -1"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    half 0xBEEF"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    half -1"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    byte 0xAA"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    byte -1"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    uint8_t expected[] = {
        0xEF, 0xBE, 0xAD, 0xDE,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xEF, 0xBE,
        0xFF, 0xFF,
        0xAA, 0xFF,
    };
    ck_assert_uint_eq(section.buffer.length, sizeof(expected));
    ck_assert_mem_eq(section.buffer.items, expected, sizeof(expected));

    ck_assert_uint_eq(section.symbol_table.length, 0);
    ck_assert_uint_eq(section.relocation_table.length, 0);
    ck_assert_uint_eq(section.string_table.length, 0);
    ck_assert_uint_eq(section.symbol_map.size, 0);

    linker_section_free(&section);
} END_TEST

START_TEST(test_static_data_multiple_values) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    word 0, 1, 2, 3"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    half 0, 1, 2, 3"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    byte 0, 1, 2, 3"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    uint8_t expected[] = {
        0, 0, 0, 0,
        1, 0, 0, 0,
        2, 0, 0, 0,
        3, 0, 0, 0,
        0, 0, 1, 0, 2, 0, 3, 0,
        0, 1, 2, 3,
    };
    ck_assert_uint_eq(section.buffer.length, sizeof(expected));
    ck_assert_mem_eq(section.buffer.items, expected, sizeof(expected));

    linker_section_free(&section);
} END_TEST

START_TEST(test_static_data_auto_length_unexpected_token) {
    LinkerSection section = linker_section_init();
    ck_assert(produces_unexpected_token_error(&section, "    word *!"));
    ck_assert(produces_unexpected_token_error(&section, "    word * word!"));
    ck_assert(produces_unexpected_token_error(&section, "    word * word *"));
    linker_section_free(&section);
} END_TEST

START_TEST(test_static_data_multiple_values_auto_length) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    word * word 0, 1, 2, 3"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    half * half 0, 1, 2, 3"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    byte * byte 0, 1, 2, 3"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    uint8_t expected[] = {
        4, 0, 0, 0,
        0, 0, 0, 0,
        1, 0, 0, 0,
        2, 0, 0, 0,
        3, 0, 0, 0,
        4, 0, 0, 0, 1, 0, 2, 0, 3, 0,
        4, 0, 1, 2, 3,
    };
    ck_assert_uint_eq(section.buffer.length, sizeof(expected));
    ck_assert_mem_eq(section.buffer.items, expected, sizeof(expected));

    linker_section_free(&section);
} END_TEST

// ================================================================
//  ASCII handling
// ================================================================

START_TEST(test_static_data_ascii_unexpected_token) {
    LinkerSection section = linker_section_init();
    ck_assert(produces_unexpected_token_error(&section, "    ascii!"));
    linker_section_free(&section);
} END_TEST

START_TEST(test_static_data_ascii_unexpected_eol) {
    LinkerSection section = linker_section_init();

    ck_assert(produces_line_error(&section, "    ascii \"", LINE_ERROR_UNEXPECTED_EOL));
    ck_assert(produces_line_error(&section, "    ascii \"\\\"", LINE_ERROR_UNEXPECTED_EOL));

    linker_section_free(&section);
} END_TEST

START_TEST(test_static_data_ascii_unknown_escape_sequence) {
    LinkerSection section = linker_section_init();

    ck_assert(produces_line_error(&section, "    ascii \"\\0\"", LINE_ERROR_UNKNOWN_ESCAPE_SEQUENCE));
    ck_assert(produces_line_error(&section, "    ascii \"\\x\"", LINE_ERROR_UNKNOWN_ESCAPE_SEQUENCE));
    ck_assert(produces_line_error(&section, "    ascii \"\\$\"", LINE_ERROR_UNKNOWN_ESCAPE_SEQUENCE));

    linker_section_free(&section);
} END_TEST

START_TEST(test_static_data_ascii) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    ascii \"\\tabc\\n\""), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    uint8_t expected[] = { '\t', 'a', 'b', 'c', '\n' };
    ck_assert_uint_eq(section.buffer.length, sizeof(expected));
    ck_assert_mem_eq(section.buffer.items, expected, sizeof(expected));

    linker_section_free(&section);
} END_TEST

START_TEST(test_static_data_ascii_valid_backslash_and_quote) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    ascii \"\\\\\\'\""), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    uint8_t expected[] = { '\\', '\'' };
    ck_assert_uint_eq(section.buffer.length, sizeof(expected));
    ck_assert_mem_eq(section.buffer.items, expected, sizeof(expected));

    linker_section_free(&section);
} END_TEST

// ================================================================
//  Directive passthrough and alignment
// ================================================================

START_TEST(test_directive_line_no_mutation) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    LineContent line_content = process_line(&section, slice_from_cstring(".foo"), &defines, &err);
    ck_assert(line_content == LINE_HAS_DIRECTIVE);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    ck_assert_uint_eq(section.buffer.length, 0);
    ck_assert_uint_eq(section.symbol_table.length, 0);
    ck_assert_uint_eq(section.relocation_table.length, 0);
    ck_assert_uint_eq(section.string_table.length, 0);
    ck_assert_uint_eq(section.symbol_map.size, 0);

    linker_section_free(&section);
} END_TEST

START_TEST(test_directive_line_with_null_section) {
    StringToIntMap defines = {};
    LineError err = {};
    LineContent line_content = process_line(nullptr, slice_from_cstring(".text"), &defines, &err);
    ck_assert(line_content == LINE_HAS_DIRECTIVE);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
} END_TEST

START_TEST(test_static_data_ascii_auto_length) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    byte * ascii \"ascii\""), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    uint8_t expected[] = { 5, 'a', 's', 'c', 'i', 'i' };
    ck_assert_uint_eq(section.buffer.length, sizeof(expected));
    ck_assert_mem_eq(section.buffer.items, expected, sizeof(expected));

    linker_section_free(&section);
} END_TEST

START_TEST(test_static_data_ascii_auto_length_escape_characters) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    byte * ascii \"\\tabc\\n\""), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    uint8_t expected[] = { 5, '\t', 'a', 'b', 'c', '\n' };
    ck_assert_uint_eq(section.buffer.length, sizeof(expected));
    ck_assert_mem_eq(section.buffer.items, expected, sizeof(expected));

    linker_section_free(&section);
} END_TEST

START_TEST(test_align_non_power_of_two) {
    LinkerSection section = linker_section_init();
    ck_assert(produces_line_error(&section, "    align 3", LINE_ERROR_NOT_ENCODABLE));
    ck_assert(produces_line_error(&section, "    align 5", LINE_ERROR_NOT_ENCODABLE));
    ck_assert(produces_line_error(&section, "    align -4", LINE_ERROR_NOT_ENCODABLE));
    linker_section_free(&section);
} END_TEST

START_TEST(test_align) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    byte 0xAA"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("    align 4"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    uint8_t expected_4[] = { 0xAA, 0, 0, 0 };
    ck_assert_uint_eq(section.buffer.length, sizeof(expected_4));
    ck_assert_mem_eq(section.buffer.items, expected_4, sizeof(expected_4));

    err = (LineError){};
    process_line(&section, slice_from_cstring("    align 8"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    uint8_t expected_8[] = { 0xAA, 0, 0, 0, 0, 0, 0, 0 };
    ck_assert_uint_eq(section.buffer.length, sizeof(expected_8));
    ck_assert_mem_eq(section.buffer.items, expected_8, sizeof(expected_8));

    linker_section_free(&section);
} END_TEST

START_TEST(test_label_alignment) {
    LinkerSection section = linker_section_init();
    StringToIntMap defines = {};
    LineError err = {};

    process_line(&section, slice_from_cstring("    byte 0xAA"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);
    err = (LineError){};
    process_line(&section, slice_from_cstring("L1:"), &defines, &err);
    ck_assert_int_eq(err.error_tag, LINE_ERROR_NONE);

    uint8_t expected[] = { 0xAA, 0, 0, 0 };
    ck_assert_uint_eq(section.buffer.length, sizeof(expected));
    ck_assert_mem_eq(section.buffer.items, expected, sizeof(expected));

    ck_assert_uint_eq(section.symbol_table.length, 1);
    ck_assert_uint_eq(section.symbol_table.items[0].section_offset, 4);
    ck_assert_uint_eq(section.symbol_table.items[0].string_table_start_index, 0);

    linker_section_free(&section);
} END_TEST

Suite* linker_section_suite(void) {
    Suite* suite = suite_create("Linker section");

    TCase* tc_basic = tcase_create("basic");
    tcase_add_test(tc_basic, test_missing_section_declaration);
    tcase_add_test(tc_basic, test_empty_line);
    tcase_add_test(tc_basic, test_local_label_non_label_character);
    tcase_add_test(tc_basic, test_local_label_missing_colon);
    tcase_add_test(tc_basic, test_local_label_unexpected_token);
    tcase_add_test(tc_basic, test_local_label);
    tcase_add_test(tc_basic, test_local_label_redefinition);
    tcase_add_test(tc_basic, test_local_label_exceeds_max_length);
    tcase_add_test(tc_basic, test_local_label_max_length_255);
    tcase_add_test(tc_basic, test_invalid_mnemonic);
    tcase_add_test(tc_basic, test_instruction_extraneous_token);
    tcase_add_test(tc_basic, test_instruction_alignment);
    tcase_add_test(tc_basic, test_general_instruction);
    tcase_add_test(tc_basic, test_expected_eol_errors);
    tcase_add_test(tc_basic, test_addr_missing_symbol);
    tcase_add_test(tc_basic, test_directive_line_no_mutation);
    tcase_add_test(tc_basic, test_directive_line_with_null_section);
    suite_add_tcase(suite, tc_basic);

    TCase* tc_relocation = tcase_create("relocation");
    tcase_add_test(tc_relocation, test_m32_integer_literal);
    tcase_add_test(tc_relocation, test_m32_relocation);
    tcase_add_test(tc_relocation, test_branch_relocation);
    tcase_add_test(tc_relocation, test_addr_relocation);
    tcase_add_test(tc_relocation, test_addr_alignment);
    tcase_add_test(tc_relocation, test_multiple_labels_and_relocations);
    tcase_add_test(tc_relocation, test_relocation_symbol_deduplication);
    tcase_add_test(tc_relocation, test_forward_relocation_resolves_on_definition);
    suite_add_tcase(suite, tc_relocation);

    TCase* tc_static_data = tcase_create("static-data");
    tcase_add_test(tc_static_data, test_static_data_out_of_range);
    tcase_add_test(tc_static_data, test_static_data_unexpected_token);
    tcase_add_test(tc_static_data, test_static_data_single_value);
    tcase_add_test(tc_static_data, test_static_data_multiple_values);
    tcase_add_test(tc_static_data, test_static_data_auto_length_unexpected_token);
    tcase_add_test(tc_static_data, test_static_data_multiple_values_auto_length);
    tcase_add_test(tc_static_data, test_static_data_ascii_unexpected_token);
    tcase_add_test(tc_static_data, test_static_data_ascii_unexpected_eol);
    tcase_add_test(tc_static_data, test_static_data_ascii_unknown_escape_sequence);
    tcase_add_test(tc_static_data, test_static_data_ascii);
    tcase_add_test(tc_static_data, test_static_data_ascii_valid_backslash_and_quote);
    tcase_add_test(tc_static_data, test_static_data_ascii_auto_length);
    tcase_add_test(tc_static_data, test_static_data_ascii_auto_length_escape_characters);
    suite_add_tcase(suite, tc_static_data);

    TCase* tc_align = tcase_create("align");
    tcase_add_test(tc_align, test_align_non_power_of_two);
    tcase_add_test(tc_align, test_align);
    tcase_add_test(tc_align, test_label_alignment);
    suite_add_tcase(suite, tc_align);

    return suite;
}

int main(void) {
    int failed = 0;

    Suite* suite = linker_section_suite();
    SRunner* runner = srunner_create(suite);
    srunner_run_all(runner, CK_NORMAL);
    failed += srunner_ntests_failed(runner);
    srunner_free(runner);

    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
