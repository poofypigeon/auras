#pragma once

#include <stdint.h>
#include <stddef.h>

#include "../string_slice.h"

typedef enum {
    MN_INVALID,
    // DATA ARRAYS
    MN_ADDR, MN_WORD, MN_HALF, MN_BYTE, MN_ASCII, MN_ALIGN,
    // M-TYPE
    MN_LW, MN_LB, MN_LH, MN_LBU, MN_LHU, MN_SW, MN_SB, MN_SH,
    // I-TYPE
    MN_MVI,
    // S-TYPE
    MN_LSR, MN_SSR, MN_SYSCALL,
    // D-TYPE
    MN_NOP,
    MN_MOV, MN_NOT,
    MN_ADD, MN_ADC, MN_SUB, MN_SBC, MN_AND, MN_OR, MN_XOR,
    MN_ADDK, MN_ADCK, MN_SUBK, MN_SBCK, MN_ANDK, MN_ORK, MN_XORK,
    MN_TST, MN_TEQ, MN_CMP, MN_CPN,
    MN_SLL, MN_SRL, MN_SRA, MN_SLLK,
    // D-TYPE invalid variants (recognized for better error messages)
    MN_SRLK, MN_SRAK,
    // B-TYPE
    MN_B, MN_BEQ, MN_BNE, MN_BLT, MN_BGE, MN_BLO, MN_BHS, MN_BMI,
    MN_BL, MN_BLEQ, MN_BLNE, MN_BLLT, MN_BLGE, MN_BLLO, MN_BLHS, MN_BLMI,
} Mnemonic;

Mnemonic parse_mnemonic(StringSlice token);
