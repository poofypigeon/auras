#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "line_error.h"
#include "parsing.h"
#include "string_slice.h"

#define ANSI_CSI "\x1b["
#define ANSI_SGR "m"

#define ANSI_FG_BRIGHT_RED "91"
#define ANSI_FG_BRIGHT_YELLOW "93"
#define ANSI_BOLD "1"
#define ANSI_RESET "0"

static void underline(StringSlice line_text, size_t start_column, size_t end_column) {
    if (end_column < start_column) {
        Tokenizer tokenizer = (Tokenizer){ .line = line_text, .token_end = start_column };
        LineError err = {};
        StringSlice token = tokenizer_next(&tokenizer, &err);
        assert(err.error_tag == LINE_ERROR_NONE);
        end_column = tokenizer.token_end;
    }

    fputs(strndup(line_text.bytes, start_column), stderr);
    fputs(ANSI_CSI ANSI_FG_BRIGHT_YELLOW ANSI_SGR, stderr);
    fputs(strndup(line_text.bytes + start_column, end_column - start_column), stderr);
    fputs(ANSI_CSI ANSI_RESET ANSI_SGR, stderr);
    fputs(strndup(line_text.bytes + end_column, line_text.length - end_column), stderr);
    fputc('\n', stderr);

    for (size_t i = 0; i < start_column; i++) {
        fputc(' ', stderr);
    }
    fputs(ANSI_CSI ANSI_FG_BRIGHT_YELLOW ANSI_SGR "^", stderr);
    for (size_t i = start_column+1; i < end_column; i++) {
        fputc('~', stderr);
    }
    fputs(ANSI_CSI ANSI_RESET ANSI_SGR "\n", stderr);
}

void print_line_error(char* file_path, size_t line_number, LineError err, StringSlice line_text) {
    size_t start_column = 0;
    switch (err.error_tag) {
    case LINE_ERROR_UNEXPECTED_EOL: start_column = err.unexpected_eol.column; break;
    case LINE_ERROR_UNEXPECTED_TOKEN: start_column = err.unexpected_token.column; break;
    case LINE_ERROR_NOT_ENCODABLE: start_column = err.not_encodable.start_column; break;
    case LINE_ERROR_NEGATIVE_SHIFT_AMOUNT: start_column = err.negative_shift_amount.start_column; break;
    case LINE_ERROR_REDEFINITION: start_column = 0; break;
    case LINE_ERROR_REPEATED_LABEL: start_column = 0; break;
    case LINE_ERROR_UNDEFINED_IDENTIFIER: start_column = err.undefined_identifier.column; break;
    case LINE_ERROR_UNKNOWN_ESCAPE_SEQUENCE: start_column = err.unknown_escape_sequence.column; break;
    case LINE_ERROR_MISSING_SECTION_DECLARATION: start_column = err.missing_section_declaration.column; break;
    case LINE_ERROR_NONE: unreachable();
    }

    fprintf(stderr, "%s(%zu:%zu): ", file_path, line_number, start_column + 1);
    fprintf(stderr, ANSI_CSI ANSI_FG_BRIGHT_RED ";" ANSI_BOLD ANSI_SGR "error: " ANSI_CSI ANSI_RESET ANSI_SGR);

    switch (err.error_tag) {
    case LINE_ERROR_UNEXPECTED_EOL:
        fprintf(stderr, "unexpected 'eol'\n");
        underline(line_text, start_column, 0);
        break;
    case LINE_ERROR_UNEXPECTED_TOKEN:
        fprintf(stderr, "expected %s", err.unexpected_token.expected);
        if (err.unexpected_token.found) {
            fprintf(stderr, ", found %s", err.unexpected_token.found);
        }
        fputc('\n', stderr);
        underline(line_text, start_column, 0);
        break;
    case LINE_ERROR_NOT_ENCODABLE:
        fprintf(stderr, "%s\n", err.not_encodable.message);
        underline(line_text, start_column, err.not_encodable.end_column);
        break;
    case LINE_ERROR_NEGATIVE_SHIFT_AMOUNT:
        fprintf(stderr, "negative shift amount\n");
        underline(line_text, start_column, err.negative_shift_amount.end_column);
        break;
    case LINE_ERROR_REDEFINITION:
        fprintf(stderr, "redefinition of '%s'\n", err.redefinition.symbol);
        break;
    case LINE_ERROR_REPEATED_LABEL:
        fprintf(stderr, "label '%s' repeated\n", err.redefinition.symbol);
        break;
    case LINE_ERROR_UNDEFINED_IDENTIFIER:
        fprintf(stderr, "'%s' not defined\n", err.undefined_identifier.symbol);
        underline(line_text, start_column, 0);
        break;
    case LINE_ERROR_UNKNOWN_ESCAPE_SEQUENCE:
        fprintf(stderr, "unknown escape sequence\n");
        underline(line_text, start_column, start_column + 2);
        break;
    case LINE_ERROR_MISSING_SECTION_DECLARATION:
        fprintf(stderr, "expected section declaration\n");
        underline(line_text, start_column, 0);
        break;
    case LINE_ERROR_NONE: unreachable();
    }
}
