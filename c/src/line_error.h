#pragma once

#include <stddef.h>
#include <stdint.h>

#include "string_slice.h"

typedef enum {
    LINE_ERROR_NONE = 0,
    LINE_ERROR_UNEXPECTED_EOL,
    LINE_ERROR_UNEXPECTED_TOKEN,
    LINE_ERROR_NOT_ENCODABLE,
    LINE_ERROR_NEGATIVE_SHIFT_AMOUNT,
    LINE_ERROR_REDEFINITION,
    LINE_ERROR_REPEATED_LABEL,
    LINE_ERROR_UNDEFINED_IDENTIFIER,
    LINE_ERROR_UNKNOWN_ESCAPE_SEQUENCE,
    LINE_ERROR_MISSING_SECTION_DECLARATION,
} LineErrorType;

typedef struct {
    size_t column; } LineErrorUnexpectedEOL;

typedef struct {
    size_t column; char* found; char* expected;
} LineErrorUnexpectedToken;

typedef struct {
    size_t start_column;
    size_t end_column;
    char* message;
} LineErrorNotEncodable;

typedef struct {
    size_t start_column;
    size_t end_column;
} LineErrorNegativeShiftAmount;

typedef struct {
    char* symbol;
} LineErrorRedefinition;

typedef struct {
    char* symbol;
} LineErrorDuplicateLabel;

typedef struct {
    size_t column;
    char* symbol;
} LineErrorUndefinedIdentifier;

typedef struct {
    size_t column;
} LineErrorUnknownEscapeSequence;

typedef struct {
    size_t column;
} LineErrorMissingSectionDeclaration;

typedef struct {
    LineErrorType error_tag;
    union {
        LineErrorUnexpectedEOL unexpected_eol;
        LineErrorUnexpectedToken unexpected_token;
        LineErrorNotEncodable not_encodable;
        LineErrorNegativeShiftAmount negative_shift_amount;
        LineErrorRedefinition redefinition;
        LineErrorUndefinedIdentifier undefined_identifier;
        LineErrorUnknownEscapeSequence unknown_escape_sequence;
        LineErrorMissingSectionDeclaration missing_section_declaration;
    };
} LineError;

void print_line_error(char* file_path, size_t line_number, LineError err, StringSlice line_text);
