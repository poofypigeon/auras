#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

#include "encode_instruction.h"
#include "endianness.h"
#include "gperf/perfect_hash.h"
#include "line_error.h"
#include "linker_section.h"
#include "parsing.h"
#include "string_slice.h"
#include "string_to_int_map.h"

#define append(list, value) \
do { \
    if ((list)->length == (list)->capacity) { \
        (list)->capacity += (list)->capacity/2; \
        (list)->items = realloc((list)->items, (list)->capacity * sizeof(*(list)->items)); \
        if ((list)->items == NULL) { \
            perror("realloc"); \
            exit(EXIT_FAILURE); \
        } \
    } \
    (list)->items[(list)->length] = (value); \
    (list)->length++; \
} while(0)

static void append_slice(CharBuffer* buffer, StringSlice slice) {
    size_t new_length = buffer->length + slice.length + 1;
    if (new_length > buffer->capacity) {
        buffer->capacity = new_length;
        buffer->capacity += buffer->capacity/2;
        buffer->items = realloc(buffer->items, buffer->capacity);
        if (buffer->items == NULL) { \
            perror("realloc");
            exit(EXIT_FAILURE);
        }
    }
    buffer->items[buffer->length] = slice.length;
    memcpy(&buffer->items[buffer->length+1], slice.bytes, slice.length);
    buffer->length += slice.length + 1;
}

static const StringSlice TOKEN_ASTERISK = (StringSlice){ .length = 1, .bytes = "*" };
static const StringSlice TOKEN_COLON    = (StringSlice){ .length = 1, .bytes = ":" };
static const StringSlice TOKEN_COMMA    = (StringSlice){ .length = 1, .bytes = "," };
static const StringSlice TOKEN_PERIOD   = (StringSlice){ .length = 1, .bytes = "." };

LinkerSection linker_section_init(void) {
    return (LinkerSection){
        .buffer = {
            .length = 0,
            .capacity = 256,
            .items = malloc(256),
        },
        .symbol_table = {
            .length = 0,
            .capacity = 64,
            .items = malloc(64 * sizeof(SymbolTableEntry)),
        },
        .relocation_table = {
            .length = 0,
            .capacity = 64,
            .items = malloc(64 * sizeof(RelocationTableEntry)),
        },
        .string_table = {
            .length = 0,
            .capacity = 256,
            .items = malloc(256),
        },
        .symbol_map = map_init(),
    };
}

void linker_section_free(LinkerSection* section) {
    free(section->buffer.items);
    free(section->symbol_table.items);
    free(section->relocation_table.items);
    free(section->string_table.items);
    map_free(&section->symbol_map);
}

static void add_relocation_symbol(LinkerSection* section, StringSlice relocation_symbol) {
    int64_t symbol_index = 0;
    if (!map_find(&section->symbol_map, relocation_symbol, &symbol_index)) {
        // Create symbol table entry
        symbol_index = section->symbol_table.length;
        map_insert(&section->symbol_map, relocation_symbol, symbol_index);
        SymbolTableEntry symbol_entry = {
            .section_offset = UNDEFINED_OFFSET, // unknown at this time
            .string_table_start_index = section->string_table.length,
        };
        append(&section->symbol_table, symbol_entry);
        append_slice(&section->string_table, relocation_symbol);
    }
    RelocationTableEntry relocation_entry = {
        .section_offset = section->buffer.length,
        .symbol_table_index = symbol_index,
    };
    append(&section->relocation_table, relocation_entry);
}

static void process_local_label(LinkerSection* section, Tokenizer* line, StringSlice label, LineError* err) {
    if (!is_symbol_start_char(label.bytes[0])) {
        *err = (LineError){
            .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
            .unexpected_token = (LineErrorUnexpectedToken){
                .column = line->token_start,
                .expected = "symbol",
                .found = found_token_string(label),
            },
        };
        return;
    }

    // Ensure labels are word aligned
    size_t current_alignment = section->buffer.length % SIZE_OF_WORD;
    size_t alignment_padding = (current_alignment == 0) ? 0 : SIZE_OF_WORD - current_alignment;
    for (size_t i = 0; i < alignment_padding; i++) {
        append(&section->buffer, 0);
    }

    int64_t symbol_index = 0;
    if (map_find(&section->symbol_map, label, &symbol_index)) {
        // If the label string is found and has a defined offet, it is a label
        // redefinition within the same section.
        if (section->symbol_table.items[symbol_index].section_offset != UNDEFINED_OFFSET) {
            *err = (LineError){
                .error_tag = LINE_ERROR_REDEFINITION,
                .redefinition = (LineErrorRedefinition){
                    .symbol = cstring_from_slice(label),
                }
            };
            return;
        }
        // Otherwise, the label string was referenced by a relocation entry and
        // now is being defined.
        section->symbol_table.items[symbol_index].section_offset = section->buffer.length;
    } else { // Create symbol table entry
        if (label.length > UINT8_MAX) {
            *err = (LineError){
                .error_tag = LINE_ERROR_NOT_ENCODABLE,
                .not_encodable = (LineErrorNotEncodable){
                    .message = "label exceeds max length (255)",
                    .start_column = line->token_start,
                    .end_column = line->token_end,
                }
            };
            return;
        }
        map_insert(&section->symbol_map, label, section->symbol_table.length);
        SymbolTableEntry symbol_entry = {
            .section_offset = section->buffer.length,
            .string_table_start_index = section->string_table.length,
        };
        append(&section->symbol_table, symbol_entry);
        append_slice(&section->string_table, label);
    }

    if (!expect_token(line, TOKEN_COLON, err)) return;
    if (!expect_eol(line, err)) return;
}

static void process_addr(LinkerSection* section, Tokenizer* line, LineError* err) {
    // Ensure addresses are word aligned
    size_t current_alignment = section->buffer.length % SIZE_OF_WORD;
    size_t alignment_padding = (current_alignment == 0) ? 0 : SIZE_OF_WORD - current_alignment;
    for (size_t i = 0; i < alignment_padding; i++) {
        append(&section->buffer, 0);
    }

    StringSlice relocation_symbol = expect_symbol(line, err);
    if (err->error_tag) return;
    add_relocation_symbol(section, relocation_symbol);

    // Empty space for linker to fill in address
    for (int i = 0; i < SIZE_OF_WORD; i++) {
        append(&section->buffer, 0);
    }

    if (!expect_eol(line, err)) return;
}

static size_t process_ascii(LinkerSection* section, Tokenizer* line, LineError* err) {
    StringSlice token = tokenizer_next(line, err);
    if (err->error_tag) return 0;
    if (token.length == 0) {
        *err = (LineError){
            .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
            .unexpected_token = (LineErrorUnexpectedToken){
                .column = line->token_start,
                .expected = "string literal",
            }
        };
        return 0;
    }
    if (token.bytes[0] != '"') {
        *err = (LineError){
            .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
            .unexpected_token = (LineErrorUnexpectedToken){
                .column = line->token_start,
                .expected = "string literal",
                .found = found_token_string(token),
            }
        };
        return 0;
    }

    size_t string_length = 0;
    for (size_t i = 1; i < token.length-1; i++) {
        char ch = token.bytes[i];
        if (ch == '\\') {
            ch = token.bytes[i+1];
            switch (ch) {
            case '\\': ch = '\\'; break;
            case '\'': ch = '\''; break;
            case 'n': ch = '\n'; break;
            case 't': ch = '\t'; break;
            default: 
                *err = (LineError){
                    .error_tag = LINE_ERROR_UNKNOWN_ESCAPE_SEQUENCE,
                    .unknown_escape_sequence = (LineErrorUnknownEscapeSequence){
                        .column = line->token_start + i,
                    }
                };
                return 0;
            }
            i++;
        }
        append(&section->buffer, (uint8_t)ch);
        string_length++;
    }
   
    if (!expect_eol(line, err)) return 0;

    return string_length;
}

static size_t process_static_data(LinkerSection* section, Tokenizer* line, size_t data_type_size, size_t recursion_depth, StringToIntMap* defines, LineError* err) {
    assert(data_type_size == SIZE_OF_WORD || data_type_size == SIZE_OF_HALF || data_type_size == SIZE_OF_BYTE);
    assert(recursion_depth <= 1);

    int64_t data_type_max = (1llu << (data_type_size * 8)) - 1;

    size_t mnem_start_column = line->token_start;
    
    StringSlice token = tokenizer_next(line, err);
    if (err->error_tag) return 0;

    size_t array_length = 0;

    if (slice_eq(token, TOKEN_ASTERISK)) {
        if (recursion_depth == 1) {
            *err = (LineError){
                .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
                .unexpected_token = (LineErrorUnexpectedToken){
                    .column = line->token_start,
                    .expected = "integer literal",
                    .found = cstring_from_slice(token),
                }
            };
            return 0;
        }

        size_t array_length_offset = section->buffer.length;
        for (size_t i = 0; i < data_type_size; i++) {
            append(&section->buffer, 0);
        }

        token = tokenizer_next(line, err);
        if (err->error_tag) return 0;
        if (token.length == 0) {
            *err = (LineError){
                .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
                .unexpected_token = (LineErrorUnexpectedToken){
                    .column = line->token_start,
                    .expected = "'word', 'half', 'byte', or 'ascii'",
                },
            };
            return 0;
        }

        Mnemonic mnem = parse_mnemonic(token);
        switch (mnem) {
        case MN_WORD:  array_length = process_static_data(section, line, SIZE_OF_WORD, 1, defines, err); break;
        case MN_HALF:  array_length = process_static_data(section, line, SIZE_OF_HALF, 1, defines, err); break;
        case MN_BYTE:  array_length = process_static_data(section, line, SIZE_OF_BYTE, 1, defines, err); break;
        case MN_ASCII: array_length = process_ascii(section, line, err); break;
        default:
            *err = (LineError){
                .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
                .unexpected_token = (LineErrorUnexpectedToken){
                    .column = line->token_start,
                    .expected = "'word', 'half', 'byte', or 'ascii'",
                    .found = found_token_string(token),
                },
            };
            return 0;
        }
        if (err->error_tag) return 0;

        // Inject the array size into the buffer before the array data
        if (array_length > data_type_max) {
            *err = (LineError){
                .error_tag = LINE_ERROR_NOT_ENCODABLE,
                .not_encodable = (LineErrorNotEncodable){
                    .start_column = mnem_start_column,
                },
            };
            switch (data_type_size) {
            case SIZE_OF_WORD: err->not_encodable.message = "array length exceeds maximum value of type 'word' (4,294,967,295)"; break;
            case SIZE_OF_HALF: err->not_encodable.message = "array length exceeds maximum value of type 'half' (65,535)"; break;
            case SIZE_OF_BYTE: err->not_encodable.message = "array length exceeds maximum value of type 'byte' (255)"; break;
            default: unreachable();
            }
            return 0;
        }

        union { uint32_t w; uint8_t b[4]; } array_length_le = { .w = htole32(array_length) };
        for (size_t i = 0; i < data_type_size; i++) {
            section->buffer.items[array_length_offset + i] = array_length_le.b[i];
        }
        
        return 0;
    }

    tokenizer_put_back(line);
    while (true) {
        size_t value_start_column = tokenizer_next_token_start(line);

        int64_t value = parse_expression(line, err, defines);
        if (err->error_tag) return 0;

        bool positive_and_not_encodable = value > data_type_max;
        bool negative_and_not_encodable = value < 0 && value >> (data_type_size*8 - 1) != -1;
        if (positive_and_not_encodable || negative_and_not_encodable) {
            *err = (LineError){
                .error_tag = LINE_ERROR_NOT_ENCODABLE,
                .not_encodable = (LineErrorNotEncodable){
                    .start_column = value_start_column,
                    .end_column = line->token_end,
                },
            };
            switch (data_type_size) {
            case SIZE_OF_WORD: err->not_encodable.message = "value is not encodable as type 'word'"; break;
            case SIZE_OF_HALF: err->not_encodable.message = "value is not encodable as type 'half'"; break;
            case SIZE_OF_BYTE: err->not_encodable.message = "value is not encodable as type 'byte'"; break;
            default: unreachable();
            }
            return 0;
        }
        
        union { uint32_t w; uint8_t b[4]; } value_le = { .w = htole32(value) };
        for (size_t i = 0; i < data_type_size; i++) {
            append(&section->buffer, value_le.b[i]);
        }

        array_length++;

        token = tokenizer_next(line, err);
        if (err->error_tag) return 0;

        if (token.length == 0) break;

        if (!slice_eq(token, TOKEN_COMMA)) {
            *err = (LineError){
                .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
                .unexpected_token = (LineErrorUnexpectedToken){
                    .column = line->token_start,
                    .expected = "'eol'",
                    .found = found_token_string(token),
                },
            };
            return 0;
        }
    }

    return array_length;
}


static void process_align(LinkerSection* section, Tokenizer* line, StringToIntMap* defines, LineError* err) {
    size_t alignment_start_column = tokenizer_next_token_start(line);

    int64_t alignment = parse_expression(line, err, defines);
    if (err->error_tag) return;

    char* message = nullptr;
    if (alignment < 0) {
        message = "alignment value must be positive";
    } else if (alignment < SIZE_OF_WORD) {
        message = "alignment value must be four or greater";
    } else if (__builtin_popcountll((unsigned long long)alignment) != 1) {
        message = "alignment value must be a power of two";
    }
    if (message) {
        *err = (LineError){
            .error_tag = LINE_ERROR_NOT_ENCODABLE,
            .not_encodable = (LineErrorNotEncodable){
                .message = message,
                .start_column = alignment_start_column,
                .end_column = line->token_end,
            }
        };
        return;
    }

    size_t alignment_u = (size_t)alignment;
    size_t current_alignment = section->buffer.length % alignment_u;
    size_t alignment_padding = (current_alignment == 0) ? 0 : alignment_u - current_alignment;
    for (size_t i = 0; i < alignment_padding; i++) {
        append(&section->buffer, 0);
    }

    if (!expect_eol(line, err)) return;
}

static void process_instruction(LinkerSection* section, Tokenizer* line, Mnemonic mnem, StringToIntMap* defines, LineError* err) {
    Instruction instr = encode_instruction_by_mnemonic(line, mnem, defines, err);
    if (err->error_tag) return;

    if (instr.relocation_symbol.length > 0) {
        add_relocation_symbol(section, instr.relocation_symbol);
    }

    // Ensure instructions are word aligned
    size_t current_alignment = section->buffer.length % SIZE_OF_WORD;
    size_t alignment_padding = (current_alignment == 0) ? 0 : SIZE_OF_WORD - current_alignment;
    for (size_t i = 0; i < alignment_padding; i++) {
        append(&section->buffer, 0);
    }

    union { uint32_t w; uint8_t b[4]; } machine_word_le = { .w = htole32(instr.machine_word) };
    for (size_t i = 0; i < SIZE_OF_WORD; i++) {
        append(&section->buffer, machine_word_le.b[i]);
    }

    // A valid second machine word field will never be 0x00000000 (lw x0, [x0])
    if (instr.second_machine_word) {
        machine_word_le.w = htole32(instr.second_machine_word);
        for (size_t i = 0; i < SIZE_OF_WORD; i++) {
            append(&section->buffer, machine_word_le.b[i]);
        }
    }
}

LineContent process_line(LinkerSection* section, StringSlice line_slice, StringToIntMap* defines, LineError* err) {
    Tokenizer line = (Tokenizer){ .line = line_slice };

    // Ignore empty lines and defer directives
    StringSlice token = tokenizer_next(&line, err);
    if (err->error_tag) return LINE_HAS_SECTION_CONTENT;
    if (token.length == 0) return LINE_HAS_SECTION_CONTENT;

    // Return of false indicates that this line should be processed as a directive
    if (slice_eq(token, TOKEN_PERIOD)) return LINE_HAS_DIRECTIVE;

    if (section == nullptr) {
        *err = (LineError){
            .error_tag = LINE_ERROR_MISSING_SECTION_DECLARATION,
            .missing_section_declaration = (LineErrorMissingSectionDeclaration){
                .column = line.token_start,
            }
        };
        return LINE_HAS_SECTION_CONTENT;
    }

    // If the first character in a non-directive line is not whitespace, then
    // the line must be a label
    if (!isspace(line_slice.bytes[0])) {
        process_local_label(section, &line, token, err);
        return LINE_HAS_SECTION_CONTENT;
    }
    
    Mnemonic mnem = parse_mnemonic(token);
    switch (mnem) {
    case MN_INVALID:
        *err = (LineError){
            .error_tag = LINE_ERROR_UNEXPECTED_TOKEN,
            .unexpected_token = (LineErrorUnexpectedToken){
                .column = line.token_start,
                .expected = "mnemonic",
                .found = found_token_string(token),
            }
        };
        return LINE_HAS_SECTION_CONTENT;
    case MN_ADDR:  process_addr(section, &line, err); break;
    case MN_ASCII: (void)process_ascii(section, &line, err); break;
    case MN_WORD:  (void)process_static_data(section, &line, SIZE_OF_WORD, 0, defines, err); break;
    case MN_HALF:  (void)process_static_data(section, &line, SIZE_OF_HALF, 0, defines, err); break;
    case MN_BYTE:  (void)process_static_data(section, &line, SIZE_OF_BYTE, 0, defines, err); break;
    case MN_ALIGN: process_align(section, &line, defines, err); break;
    default:       process_instruction(section, &line, mnem, defines, err); break;
    }

    return LINE_HAS_SECTION_CONTENT;
}
