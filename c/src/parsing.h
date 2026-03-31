#pragma once

#include <stddef.h>
#include <stdint.h>

#include "line_error.h"
#include "string_slice.h"
#include "string_to_int_map.h"

typedef struct {
    StringSlice line;
    size_t token_start;
    size_t token_end;
} Tokenizer;

typedef enum {
    OPERAND_INVALID = 0,
    OPERAND_REGISTER,
    OPERAND_UINT,
    OPERAND_SYMBOL,
} OperandType;

typedef struct {
    OperandType operand_type;
    union {
        uint64_t reg;
        uint64_t uint;
        StringSlice symbol;
        StringSlice invalid;
    };
} Operand;

bool tokenizer_next(Tokenizer* tokenizer, StringSlice* token, LineError* err);
size_t tokenizer_next_token_start(Tokenizer* tokenizer);
void tokenizer_curr(Tokenizer* tokenizer, StringSlice* token);
void tokenizer_put_back(Tokenizer* tokenizer);

Operand parse_operand(StringSlice token, LineError* err);
bool parse_register(StringSlice token, uint64_t* reg);
int64_t parse_expression(Tokenizer* line, LineError* err, StringToIntMap* def_map);

uint64_t expect_register(Tokenizer* tokenizer, LineError* err);
bool expect_token(Tokenizer* line, StringSlice expected_token, LineError* err);
StringSlice expect_label(Tokenizer* tokenizer, LineError* err);

char* found_token_string(StringSlice token);
