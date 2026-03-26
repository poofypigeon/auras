#pragma once

#include <stddef.h>
#include <stdint.h>

#include "parsing.h"

typedef struct {
    uint32_t machine_word;
} Instruction;

Instruction encode_instruction(Tokenizer* line, StringToIntMap* defines, LineError* err);
