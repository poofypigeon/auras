#include <assert.h>
#include <stdio.h>

#include "../src/line_error.h"
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

int main(void) {
    lw();
    mvi();
    lsr();
    ssr();
    syscall();
}
