#pragma once

#include <stddef.h>
#include <stdint.h>

#include "gperf/perfect_hash.h"
#include "parsing.h"

typedef struct {
    uint32_t machine_word;
    uint32_t second_machine_word;
    StringSlice relocation_symbol;
} Instruction;

Instruction encode_instruction_by_mnemonic(Tokenizer* line, Mnemonic mnem, StringToIntMap* defines, LineError* err);
Instruction encode_instruction(Tokenizer* line, StringToIntMap* defines, LineError* err);
