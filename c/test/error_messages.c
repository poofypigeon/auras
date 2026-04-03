#include <assert.h>
#include <stdio.h>

#include "../src/line_error.h"
#include "../src/linker_section.h"
#include "../src/parsing.h"
#include "../src/string_slice.h"
#include "../src/encode_instruction.h"

static void show_instruction_or_error(char* line_cstring) {
    Tokenizer line = { .line = slice_from_cstring(line_cstring) };
    StringToIntMap defines = {};
    LineError err = {};

    uint32_t machine_word = encode_instruction(&line, &defines, &err).machine_word;
    if (err.error_tag) {
        print_line_error("file.s", 1, err, line.line);
    } else {
        printf("%s: 0x%08x\n", line_cstring, machine_word);
    }
}

static void show_instruction_or_error_with_define(char* line_cstring, char* define, int64_t value) {
    Tokenizer line = { .line = slice_from_cstring(line_cstring) };
    StringToIntMap defines = map_init();
    map_insert(&defines, slice_from_cstring(define), value);
    LineError err = {};

    uint32_t machine_word = encode_instruction(&line, &defines, &err).machine_word;
    if (err.error_tag) {
        print_line_error("file.s", 1, err, line.line);
    } else {
        printf("%s: 0x%08x\n", line_cstring, machine_word);
    }
}

static void show_linker_line_or_error(LinkerSection* section, char* line_cstring) {
    StringToIntMap defines = {};
    LineError err = {};
    StringSlice line = slice_from_cstring(line_cstring);

    bool line_has_section_content = process_line(section, line, &defines, &err);
    if (err.error_tag) {
        print_line_error("file.s", 1, err, line);
    } else {
        fprintf(stderr, "%s: %s\n", line_cstring, line_has_section_content ? "ok" : "directive");
    }
}

void lw() {
    fprintf(stderr, "================================================================\n");
    fprintf(stderr, " M-Type (LW)\n");
    fprintf(stderr, "================================================================\n");
    show_instruction_or_error("    lw");
    show_instruction_or_error("    lw 123 + 1");
    show_instruction_or_error("    lw x0 \"\"");
    show_instruction_or_error("    lw x0, s3");
    show_instruction_or_error("    lw x0, boop");
    show_instruction_or_error("    lw x0, [t1  ; comment");
    show_instruction_or_error("    lw t0, [t1, t2 sll");
    show_instruction_or_error("    lw t0, [t1, 10 sll 1]");
    show_instruction_or_error("    lw \"asdf");
    show_instruction_or_error("    lw t0, [t1, -t2 sll -1]");
    show_instruction_or_error("    lw t0, [t1, 0xffff_ffff_ffff]");
    show_instruction_or_error("    lw t0, [t1, 0xffff_ffff + 1]");
    show_instruction_or_error("    lw t0, [t1, 0xff00_0000]");
    show_instruction_or_error("    lw t0, [t1, t2 sll ~boop * beep]");
    show_instruction_or_error("    lw t0, [t1, 200 >> -(100 << 4-1)]");
    show_instruction_or_error("    lw t0, [t1, 'x\\xx']");
    show_instruction_or_error("    lw t0, [t1, 'abcde']");
    show_instruction_or_error("    lw t0, [t1, 'abcdefghi']");
    show_instruction_or_error_with_define("    lw t0, [t1, foo >> 1]", "foo", 0xffffffffffff);
}

void mvi() {
    fprintf(stderr, "================================================================\n");
    fprintf(stderr, " I-Type (MVI)\n");
    fprintf(stderr, "================================================================\n");
    show_instruction_or_error("    mvi");
    show_instruction_or_error("    mvi 1");
    show_instruction_or_error("    mvi t0, 1<<23");
    show_instruction_or_error("    mvi t0, 0xffff_ffff + 1");
    show_instruction_or_error("    mvi t0, 0xffff_ffff_fffff * 0xfffff_ffff_ffff * 0xfffff_ffff_ffff");
    show_instruction_or_error("    mvi t0, 0xffff_ffff_fffff * foo");
}

void lsr() {
    fprintf(stderr, "================================================================\n");
    fprintf(stderr, " S-Type: (LSR)\n");
    fprintf(stderr, "================================================================\n");
    show_instruction_or_error("    lsr");
    show_instruction_or_error("    lsr t0, 63");
    show_instruction_or_error("    lsr t0, -1");
    show_instruction_or_error("    lsr t0, 64");
    show_instruction_or_error_with_define("    lsr t0, BAD_CSR", "BAD_CSR", 0xffffffffffff);
    show_instruction_or_error_with_define("    lsr t0, BAD_CSR", "BAD_CSR", 63);
}

void ssr() {
    fprintf(stderr, "================================================================\n");
    fprintf(stderr, " S-Type: (SSR)\n");
    fprintf(stderr, "================================================================\n");
    show_instruction_or_error("    ssr");
    show_instruction_or_error("    ssr t0, 0, 63");
    show_instruction_or_error("    ssr t0, t1, 64");
    show_instruction_or_error("    ssr t0, 256, 0");
    show_instruction_or_error("    ssr t0, -1, 0");
}

void syscall() {
    fprintf(stderr, "================================================================\n");
    fprintf(stderr, " S-Type: (SYSCALL)\n");
    fprintf(stderr, "================================================================\n");
    show_instruction_or_error("    syscall");
    show_instruction_or_error("    syscall 256");
    show_instruction_or_error("    syscall -1");
}

void d_type() {
    fprintf(stderr, "================================================================\n");
    fprintf(stderr, " D-Type:\n");
    fprintf(stderr, "================================================================\n");
    show_instruction_or_error("    nop x1");
    show_instruction_or_error("    nop 0xAA");

    show_instruction_or_error("    srlk x1, x2, 5");
    show_instruction_or_error("    srak x1, x2, 5");

    show_instruction_or_error("    addk x1, x2, x3 srl 1");
    show_instruction_or_error("    addk x1, x2, x3 sra 1");

    show_instruction_or_error("    add x1, x2, x3 srl 0");
    show_instruction_or_error("    add x1, x2, x3 sra 0");

    show_instruction_or_error("    adc x1, x2, 3");
    show_instruction_or_error("    sbc x1, x2, 3");
    show_instruction_or_error("    adck x1, x2, 3");
    show_instruction_or_error("    sbck x1, x2, 3");

    show_instruction_or_error("    add t0, t1, 0xf_ffff_ffff_ffff_ffff");
    show_instruction_or_error("    add t0, t1, 'abcdefghi'");

    show_instruction_or_error("    add t0, t1, 0xffff_ffff_ffff");
    show_instruction_or_error("    add t0, t1, 0xffff_ffff + 1");
    show_instruction_or_error("    add t0, t1, 'abcde'");

    show_instruction_or_error("    add x1, x2, 0x2AA00");

    show_instruction_or_error("    add t0, t1, t2 sll -1");
    show_instruction_or_error("    add t0, t1, t2 sll 1 - 19");

    show_instruction_or_error("    add t0, t1, 200 >> -(100 << 4-1)");

    show_instruction_or_error("    add x1, x2, 0x0011 sll 2");
    show_instruction_or_error("    add x1, x2, 0x0011 srl 2");
    show_instruction_or_error("    add x1, x2, 0x0011 sra 2");
    show_instruction_or_error("    add x1, x2, -257");
    show_instruction_or_error("    add x2, 256");
}

void linker_section() {
    fprintf(stderr, "================================================================\n");
    fprintf(stderr, " Linker section:\n");
    fprintf(stderr, "================================================================\n");

    // Missing section declaration
    show_linker_line_or_error(nullptr, "anything");

    LinkerSection section = linker_section_init();

    // Expected 'eol' style errors
    show_linker_line_or_error(&section, "L1: extra");
    show_linker_line_or_error(&section, "    addr L1 extra");
    show_linker_line_or_error(&section, "    ascii \"abc\" extra");
    show_linker_line_or_error(&section, "    align 4 extra");
    show_linker_line_or_error(&section, "    nop extra");
    show_linker_line_or_error(&section, "    word 1 extra");

    // Other diagnostics
    show_linker_line_or_error(&section, "    addr");
    show_linker_line_or_error(&section, "    addr 123");
    show_linker_line_or_error(&section, "    bad");
    show_linker_line_or_error(&section, "0:");
    show_linker_line_or_error(&section, "L_no_colon");
    show_linker_line_or_error(&section, "Lbad:!");
    show_linker_line_or_error(&section, "    ascii \"\\x\"");
    show_linker_line_or_error(&section, "    ascii \"");
    show_linker_line_or_error(&section, "    ascii!");
    show_linker_line_or_error(&section, "    align 3");
    show_linker_line_or_error(&section, "    align 5");
    show_linker_line_or_error(&section, "    align -4");
    show_linker_line_or_error(&section, "    align 2");
    show_linker_line_or_error(&section, "    align 0");
    show_linker_line_or_error(&section, "    align foo");
    show_linker_line_or_error(&section, "    word 0x1_0000_0000");
    show_linker_line_or_error(&section, "    word -0x8000_0001");
    show_linker_line_or_error(&section, "    half 0x1_0000");
    show_linker_line_or_error(&section, "    half -0x8001");
    show_linker_line_or_error(&section, "    byte 0x100");
    show_linker_line_or_error(&section, "    byte -0x81");
    show_linker_line_or_error(&section, "    word!");
    show_linker_line_or_error(&section, "    word 0,!");
    show_linker_line_or_error(&section, "    word *!");
    show_linker_line_or_error(&section, "    word * word!");
    show_linker_line_or_error(&section, "    word * word *");
    show_linker_line_or_error(&section, "L1:");

    // Label max length error (256 chars > 255 max)
    char long_label[260];
    for (size_t i = 0; i < 256; i++) long_label[i] = 'a';
    long_label[256] = ':';
    long_label[257] = '\0';
    show_linker_line_or_error(&section, long_label);

    linker_section_free(&section);
}

int main(void) {
    lw();
    mvi();
    lsr();
    ssr();
    syscall();
    d_type();
    linker_section();
}
