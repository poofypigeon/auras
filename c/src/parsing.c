#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "line_error.h"
#include "parsing.h"
#include "string_slice.h"

static bool is_symbol_start_char(char c) {
    return isalpha(c) || c == '_';
}

StringSlice tokenizer_next(Tokenizer* tokenizer, LineError* err) {
    StringSlice* line = &tokenizer->line;

    for (; tokenizer->token_end < line->length; tokenizer->token_end++) {
        if (!isspace(line->bytes[tokenizer->token_end])) break;
    }
    tokenizer->token_start = tokenizer->token_end;
    if (tokenizer->token_end == line->length) return (StringSlice){};
    if (line->bytes[tokenizer->token_start] == ';') return (StringSlice){};
    tokenizer->token_end++;

    // String literals and character literals are consumed as one token
    if (line->bytes[tokenizer->token_start] == '"' || line->bytes[tokenizer->token_start] == '\'') {
        bool escaped = false;
        while (true) {
            if (tokenizer->token_end == line->length) {
                *err = (LineError){
                    .error_tag = LINE_ERROR_UNEXPECTED_EOL,
                    .unexpected_eol = (LineErrorUnexpectedEOL){ .column = tokenizer->token_end },
                };
                return (StringSlice){};
            }

            if (line->bytes[tokenizer->token_end] == line->bytes[tokenizer->token_start] && !escaped) break;
            escaped = (line->bytes[tokenizer->token_end] == '\\' && !escaped);
            tokenizer->token_end++;
        }
        tokenizer->token_end++;
        return (StringSlice){
            .bytes = &line->bytes[tokenizer->token_start],
            .length = tokenizer->token_end - tokenizer->token_start,
        };
    }

    // Shift operators ('<<', '>>') are the only two-character operators
    if (tokenizer->token_end != line->length && (line->bytes[tokenizer->token_start] == '<' || line->bytes[tokenizer->token_start] == '>')) {
        if (line->bytes[tokenizer->token_start+1] == line->bytes[tokenizer->token_start]) {
            tokenizer->token_end++;
            return (StringSlice){
                .bytes = &line->bytes[tokenizer->token_start],
                .length = tokenizer->token_end - tokenizer->token_start,
            };
        }
    }

    // Any non-alphanumeric characters besides whitespace and underscores are distinct tokens
    if (!is_symbol_start_char(line->bytes[tokenizer->token_start]) && !isdigit(line->bytes[tokenizer->token_start])) {
        return (StringSlice){
            .bytes = &line->bytes[tokenizer->token_start],
            .length = tokenizer->token_end - tokenizer->token_start,
        };
    }

    // Consume characters until a non-label character is encountered
    for (; tokenizer->token_end < line->length; tokenizer->token_end++) {
        if (!is_symbol_start_char(line->bytes[tokenizer->token_end]) && !isdigit(line->bytes[tokenizer->token_end])) break;
    }
    return (StringSlice){
        .bytes = &line->bytes[tokenizer->token_start],
        .length = tokenizer->token_end - tokenizer->token_start,
    };
}

size_t tokenizer_next_token_start(Tokenizer* tokenizer) {
    StringSlice* line = &tokenizer->line;

    for (; tokenizer->token_end < line->length; tokenizer->token_end++) {
        if (!isspace(line->bytes[tokenizer->token_end])) break;
    }
    return tokenizer->token_end;
}

void tokenizer_put_back(Tokenizer* tokenizer) {
    tokenizer->token_end = tokenizer->token_start;
}

StringSlice tokenizer_curr(Tokenizer* tokenizer) {
    StringSlice* line = &tokenizer->line;
    return (StringSlice){
        .bytes = &line->bytes[tokenizer->token_start],
        .length = tokenizer->token_end - tokenizer->token_start,
    };
}

bool parse_register(StringSlice token, uint64_t* reg) { 
    // x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  x10 x11 x12 x13 x14 x15 x16 x17 x18 x19 x20 x21 x22 x23 x24 x25 x26 x27 x28 x29 x30 x31
    // x0  lr  sp  t0  t1  t2  t3  t4  a0  a1  a2  a3  a4  a5  a6  a7  s0  s1  s2  s3  s4  s5  s6  s7  s8  s9  s10 s11 t5  t6  t7  t8
   
    // x0-31
    if (token.bytes[0] == 'x') {
        if (token.length < 2 || token.length > 3 || !isdigit(token.bytes[1])) return false;
        *reg = token.bytes[1] - '0';
        if (token.length == 3) {
            if (!isdigit(token.bytes[2])) return false;
            *reg *= 10;
            *reg += token.bytes[2] - '0';
        }
        return (*reg < 32);
    };

    // t0-t8
    if (token.bytes[0] == 't') {
        if (token.length != 2 || !isdigit(token.bytes[1])) return false;
        *reg = token.bytes[1] - '0';
        if (*reg > 8) return false;
        *reg += (*reg > 4) ? 23 : 3; // t0-8 are addresses x3-7 and x28-31
        return true;
    };

    // a0-a7
    if (token.bytes[0] == 'a') {
        if (token.length != 2 || !isdigit(token.bytes[1])) return false;
        *reg = token.bytes[1] - '0';
        if (*reg > 7) return false;
        *reg += 8; // a0-7 are addresses x8-15
        return true;
    };

    // sp/s0-11
    if (token.bytes[0] == 's') {
        // sp
        if (token.length == 2 && token.bytes[1] == 'p') {
            *reg = 2;
            return true;
        }

        if (token.length < 2 || token.length > 3 || !isdigit(token.bytes[1])) return false;
        *reg = token.bytes[1] - '0';
        if (token.length == 3) {
            if (!isdigit(token.bytes[2])) return false;
            *reg *= 10;
            *reg += token.bytes[2] - '0';
        }
        if (*reg > 11) return false;
        *reg += 16; // s0-11 are addresses x16-27
        return true;
    };

    // lr
    if (token.length == 2 && token.bytes[0] == 'l' && token.bytes[1] == 'r') {
        *reg = 1;
        return true;
    }

    return false;
}

static bool parse_char_literal(StringSlice token, uint64_t* v, LineError* err) {
    if (token.bytes[0] != '\'') return false;
    assert(token.bytes[token.length-1] == '\'');

    *v = 0;
    size_t bytes = 0;
    for (size_t i = 1; i < token.length-1; i++) {
        size_t ch = token.bytes[i];
        if (ch == '\\') {
            ch = token.bytes[i+1];
            switch (ch) {
            case '\\': break;
            case '\'': ch = '\''; break;
            case 'n': ch = '\n'; break;
            case 't': ch = '\t'; break;
            default:
                *err = (LineError){
                    .error_tag = LINE_ERROR_UNKNOWN_ESCAPE_SEQUENCE,
                    .unknown_escape_sequence = (LineErrorUnknownEscapeSequence){
                        .column = i, // offset into token
                                     // caller adds token_start to calculate actual offset
                    },
                };
                return false;
            }
            i++;
        }
        *v += ch << (bytes * 8);

        bytes++;
        if (bytes > 7) {
            *err = (LineError){
                .error_tag = LINE_ERROR_NOT_ENCODABLE,
                .not_encodable = (LineErrorNotEncodable){
                    .message = "character literal is too large",
                },
            };
            return false;
        }
    }
    return true;
}

bool parse_uint(StringSlice token, uint64_t* v, LineError* err) {
    uint64_t digit;

    if (token.bytes[0] == '\'') return parse_char_literal(token, v, err);

    *v = 0;
    if (token.bytes[0] == '0') {
        if (token.length == 1) return true; // decimal 0
        if (token.length < 3) return false;
        switch(token.bytes[1]) {
        case 'b': // binary (0b)
            *v = token.bytes[2] - '0';
            if (*v > 1) return false;
            for (size_t i = 3; i < token.length; i++) {
                if (token.bytes[i] == '_') continue;
                if (*v > UINT64_MAX/2) goto TOO_LARGE;
                *v *= 2;
                digit = token.bytes[i] - '0';
                if (digit > 1) return false;
                if (*v > UINT64_MAX-digit) goto TOO_LARGE;
                *v += digit;
            }
            return true;
        case 'x': // hexadecimal (0x)
            *v = token.bytes[2];
            if      (*v >= 'a') *v -= 'a' - 0xa;
            else if (*v >= 'A') *v -= 'A' - 0xa;
            else if (*v >= '0') *v -= '0';
            if (*v > 15) return false;
            for (size_t i = 3; i < token.length; i++) {
                if (token.bytes[i] == '_') continue;
                if (*v > UINT64_MAX/16) goto TOO_LARGE;
                *v *= 16;
                digit = token.bytes[i];
                if      (digit >= 'a') digit -= 'a' - 0xa;
                else if (digit >= 'A') digit -= 'A' - 0xa;
                else if (digit >= '0') digit -= '0';
                if (digit > 15) return false;
                if (*v > UINT64_MAX-digit) goto TOO_LARGE;
                *v += digit;
            }
            return true;
        default:
            return false;
        }
    }

    // decimal
    *v = token.bytes[0] - '0';
    if (*v > 9) return false;
    for (size_t i = 1; i < token.length; i++) {
        if (token.bytes[i] == '_') continue;
        if (*v > UINT64_MAX/10) goto TOO_LARGE;
        *v *= 10;
        digit = token.bytes[i] - '0';
        if (digit > 9) return false;
        if (*v > UINT64_MAX-digit) goto TOO_LARGE;
        *v += digit;
    }
    return true;

TOO_LARGE:
    *err = (LineError){
        .error_tag = LINE_ERROR_NOT_ENCODABLE,
        .not_encodable = (LineErrorNotEncodable){
            .message = "integer literal is too large",
        },
    };
    return false;
}

Operand parse_operand(StringSlice token, LineError* err) {
    assert(token.length > 0);
    uint64_t v;

    // register
    if (parse_register(token, &v)) {
        return (Operand){ .operand_type = OPERAND_REGISTER, .reg  = v };
    }

    // uint (binary, hexadecimal, decimal)
    if (parse_uint(token, &v, err)) {
        return (Operand){ .operand_type = OPERAND_UINT, .uint = v };
    }
    if (err->error_tag) return (Operand){ .operand_type = OPERAND_UINT };

    // symbol
    if (is_symbol_start_char(token.bytes[0])) {
        return (Operand){ .operand_type = OPERAND_SYMBOL, .symbol = token };
    }

    return (Operand){};
}

char* found_token_string(StringSlice token) {
    assert(token.length > 0);
    if (token.bytes[0] == '"') return "string literal";
    if (token.bytes[0] == '\'') return "character literal";

    LineError err = {};
    Operand op = parse_operand(token, &err);
    switch (op.operand_type) {
    case OPERAND_REGISTER: return "register";
    case OPERAND_UINT:     return "integer literal";
    case OPERAND_SYMBOL:   return quoted_cstring_from_slice(token);
    case OPERAND_INVALID:  return quoted_cstring_from_slice(token);
    }
}

uint64_t expect_register(Tokenizer* line, LineError* err) {
    StringSlice token = {};
    uint64_t reg = 0;

    token = tokenizer_next(line, err);
    if (err->error_tag) return 0;
    if (token.length == 0) {
        *err = (LineError){
            .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
            .unexpected_token = (LineErrorUnexpectedToken){
                .column = line->token_start,
                .expected = "register",
            },
        };
        return 0;
    }

    if (!parse_register(token, &reg)) {
        *err = (LineError){
            .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
            .unexpected_token = (LineErrorUnexpectedToken){
                .column = line->token_start,
                .expected = "register",
                .found = found_token_string(token),
            },
        };
        return 0;
    }

    return (uint64_t)reg;
} 

bool expect_token(Tokenizer* line, StringSlice expected_token, LineError* err) {
    StringSlice token = tokenizer_next(line, err);
    if (err->error_tag) return false;
    if (token.length == 0) {
        *err = (LineError){
            .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
            .unexpected_token = (LineErrorUnexpectedToken){
                .column = line->token_start,
                .expected = quoted_cstring_from_slice(expected_token),
            },
        };
        return false;
    }
    if (!slice_eq(token, expected_token)) {
        *err = (LineError){
            .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
            .unexpected_token = (LineErrorUnexpectedToken){
                .column = line->token_start,
                .expected = quoted_cstring_from_slice(expected_token),
                .found = found_token_string(token),
            },
        };
        return false;
    }
    return true;
}

StringSlice expect_label(Tokenizer* line, LineError* err) {
    StringSlice token = tokenizer_next(line, err);
    if (err->error_tag) return (StringSlice){};
    if (token.length == 0) {
        *err = (LineError){
            .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
            .unexpected_token = (LineErrorUnexpectedToken){
                .column = line->token_start,
                .expected = "label",
            },
        };
        return (StringSlice){};
    }
    if (!is_symbol_start_char(token.bytes[0])) {
        *err = (LineError){
            .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
            .unexpected_token = (LineErrorUnexpectedToken){
                .column = line->token_start,
                .expected = "label",
                .found = found_token_string(token),
            },
        };
        return (StringSlice){};
    }
    return token;
}

// ================================================================
//  Pratt Expression Parser
// ================================================================

typedef enum {
    BP_NONE,
    BP_OR,
    BP_XOR,
    BP_AND,
    BP_SHIFT,
    BP_TERM,
    BP_FACTOR,
} BindingPower;

// define in advance so that null_denotation can call this
static int64_t expression(Tokenizer* line, LineError* err, StringToIntMap* defines, BindingPower rbp);

static int64_t null_denotation(Tokenizer* line, LineError* err, StringToIntMap* defines) {
    StringSlice token = tokenizer_curr(line);

    size_t nud_start = line->token_start;
    char unary_operation = '+';
    switch (token.bytes[0]) {
    case '-':
    case '~':
        unary_operation = token.bytes[0];
    case '+':
        token = tokenizer_next(line, err);
        if (err->error_tag) return 0;
        if (token.length == 0) {
            *err = (LineError){
                .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
                .unexpected_token = (LineErrorUnexpectedToken){
                    .column = line->token_start,
                    .expected = "expression",
                },
            };
            return 0;
        }
    }

    int64_t value = 0;
    if (token.bytes[0] == '(') {
        value = expression(line, err, defines, 0);
        if (err->error_tag) return 0;

        token = tokenizer_next(line, err);
        if (err->error_tag) return 0;
        if (token.length == 0) {
            *err = (LineError){
                .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
                .unexpected_token = (LineErrorUnexpectedToken){
                    .column = line->token_start,
                    .expected = "')'",
                },
            };
            return 0;
        }

        if (token.bytes[0] != ')') {
            *err = (LineError){
                .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
                .unexpected_token = (LineErrorUnexpectedToken){
                    .column = line->token_start,
                    .expected = "')'",
                    .found = (token.length > 0) ? found_token_string(token) : nullptr,
                },
            };
            return 0;
        }

    } else {
        Operand op = parse_operand(token, err);
        if (err->error_tag == LINE_ERROR_UNKNOWN_ESCAPE_SEQUENCE) {
            err->not_encodable.start_column += line->token_start;
            return 0;
        }
        if (err->error_tag == LINE_ERROR_NOT_ENCODABLE) {
            err->not_encodable.start_column = nud_start;
            err->not_encodable.end_column = line->token_end;
            return 0;
        }
        assert(err->error_tag == LINE_ERROR_NONE);

        switch (op.operand_type) {
        case OPERAND_UINT:
            if ((op.uint > -((uint64_t)INT64_MIN)) || ((unary_operation != '-') && op.uint > ((uint64_t)INT64_MAX))) {
                *err = (LineError){
                    .error_tag = LINE_ERROR_NOT_ENCODABLE,
                    .not_encodable = (LineErrorNotEncodable){
                        .start_column = nud_start,
                        .end_column = line->token_end,
                        .message = "integer literal is too large"
                    },
                };
                return 0;
            }
            value = (int64_t)op.uint;
            break;

        case OPERAND_SYMBOL:
            if (!map_find(defines, op.symbol, &value)) {
                *err = (LineError){
                    .error_tag = LINE_ERROR_UNDEFINED_IDENTIFIER,
                    .undefined_identifier = (LineErrorUndefinedIdentifier){
                        .column = line->token_start,
                        .symbol = strndup(op.symbol.bytes, op.symbol.length),
                    },
                };
                return 0;
            }
            break;

        default:
            *err = (LineError){
                .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
                .unexpected_token = (LineErrorUnexpectedToken){
                    .column = line->token_start,
                    .expected = "expression",
                    .found = found_token_string(token),
                },
            };
            return 0;
        }
    }

    switch (unary_operation) {
    case '-': return -value;
    case '~': return ~value;
    default:  return  value;
    }
}

static BindingPower binding_power(StringSlice token) {
    switch (token.bytes[0]) {
    case '|': return BP_OR;
    case '^': return BP_XOR;
    case '&': return BP_AND;
    case '<': return (token.length == 2 && token.bytes[1] == '<') ? BP_SHIFT : BP_NONE;
    case '>': return (token.length == 2 && token.bytes[1] == '>') ? BP_SHIFT : BP_NONE;
    case '+': return BP_TERM;
    case '-': return BP_TERM;
    case '*': return BP_FACTOR;
    case '/': return BP_FACTOR;
    case '%': return BP_FACTOR;
    default: return BP_NONE;
    }
}

static int64_t expression(Tokenizer* line, LineError* err, StringToIntMap* defines, BindingPower rbp) {
    StringSlice token = tokenizer_next(line, err);
    if (err->error_tag) return 0;
    if (token.length == 0) {
        *err = (LineError){
            .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
            .unexpected_token = (LineErrorUnexpectedToken){
                .column = line->token_start,
                .expected = "expression",
            },
        };
        return 0;
    }

    int64_t left = null_denotation(line, err, defines);
    if (err->error_tag) return 0;

    size_t expression_start_column = 0;

    while (true) {
        token = tokenizer_next(line, err);
        if (err->error_tag) return 0;

        BindingPower bp = (token.length == 0) ? BP_NONE : binding_power(token);

        if (bp <= rbp) break;

        expression_start_column = tokenizer_next_token_start(line);
        int64_t right = expression(line, err, defines, bp);
        if (err->error_tag) return 0;

        switch (token.bytes[0]) {
        case '+': left +=  right; break;
        case '-': left -=  right; break;
        case '<':
            if (right < 0) goto NEGATIVE_SHIFT_VALUE;
            left <<= right;
            break;
        case '>':
            if (right < 0) goto NEGATIVE_SHIFT_VALUE;
            left >>= right;
            break;
        case '&': left &=  right; break;
        case '|': left |=  right; break;
        case '^': left ^=  right; break;
        case '*': left *=  right; break;
        case '/': left /=  right; break;
        case '%': left %=  right; break;
        }
    }

    tokenizer_put_back(line);
    return left;

NEGATIVE_SHIFT_VALUE:
    *err = (LineError){
        .error_tag = LINE_ERROR_NEGATIVE_SHIFT_AMOUNT,
        .negative_shift_amount = (LineErrorNegativeShiftAmount){
            .start_column = expression_start_column,
            .end_column = line->token_end,
        },
    };
    return 0;
}

int64_t parse_expression(Tokenizer* line, LineError* err, StringToIntMap* defines) {
    return expression(line, err, defines, 0);
}
