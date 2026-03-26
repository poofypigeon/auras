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
} Mnemonic;

Mnemonic parse_mnemonic(StringSlice token);
