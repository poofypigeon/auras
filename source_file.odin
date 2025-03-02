package auras

import "core:fmt"
import "core:mem"
import "core:unicode"
import "core:math/bits"

Relocation_Table_Entry :: struct {
    offset: u32, // offset from beginning of file
    symbol: u32, // index into symbol table
}

Symbol_Table_Entry :: struct {
    offset: u32, // offset from beginning of file
    name:   u32, // index into string table
}

Source_File :: struct {
    buffer: [dynamic]u8,
    symbol_table: [dynamic]Symbol_Table_Entry,
    relocation_table: [dynamic]Relocation_Table_Entry,
    string_table: [dynamic]u8,
    symbol_map: map[string]u32,
}

// TODO more thoughtful memory management
create_source_file :: proc() -> Source_File {
    return Source_File{
        buffer           = make([dynamic]u8, 0, 1024),
        string_table     = make([dynamic]u8, 0, 1024),
        symbol_map       = make(map[string]u32),
        relocation_table = make([dynamic]Relocation_Table_Entry, 0, 64),
        symbol_table     = make([dynamic]Symbol_Table_Entry, 0, 64)
    }
}

cleanup_source_file :: proc(file: ^Source_File) {
    delete(file.buffer)
    delete(file.string_table)
    delete(file.symbol_map)
    delete(file.relocation_table)
    delete(file.symbol_table)
}

process_line :: proc(file: ^Source_File, line: string) -> (err: Line_Error) {
    token: string = ---
    ok: bool = ---

    line := Tokenizer{ line = line }

    if token, ok = optional_token(&line, eol = true); ok {
        // if token[0] == '.' {
        //     // process_directive(file, &line) or_return
        // }
        return nil
    }

    if !unicode.is_space(rune(line.line[0])) {
        process_local_label(file, &line) or_return
        return nil
    }

    token, _ = tokenizer_next(&line)

    mnem := mnem_from_token(token)
    #partial switch mnem {
    case .invalid:
        return Unexpected_Token{
            column = line.token_start,
            expected = "mnemonic", found = token_str(token)
        }
    case .word: process_static_data(file, &line, SIZE_OF_WORD) or_return
    case .half: process_static_data(file, &line, SIZE_OF_HALF) or_return
    case .byte: process_static_data(file, &line, SIZE_OF_BYTE) or_return
    case .ascii: process_ascii(file, &line) or_return
    case .align: process_align(file, &line) or_return
    case: process_instruction(file, &line, mnem)  or_return
    }
   
    return nil
}

@(private = "file")
process_local_label :: proc(file: ^Source_File, line: ^Tokenizer) -> (err: Line_Error) {
    token: string = ---
    ok: bool = ---

    token, ok = tokenizer_next(line)

    if !(unicode.is_alpha(rune(token[0])) || token[0] == '_') {
        return Unexpected_Token{
            column = line.token_start,
            expected = "label", found = token_str(token)
        }
    }

    // Ensure labels are word aligned
    current_alignment := len(file.buffer) % SIZE_OF_WORD
    alignment_padding := current_alignment == 0 ? 0 : SIZE_OF_WORD - current_alignment
    for _ in 0..<alignment_padding {
        append(&file.buffer, 0)
    }

    symbol_index: u32 = ---
    if symbol_index, ok = file.symbol_map[token]; ok {
        if file.symbol_table[symbol_index].offset < max(u32) {
            return Redefinition{ label = token }
        }
        file.symbol_table[symbol_index].offset = u32(len(file.buffer))
    } else { // create symbol table entry
        file.symbol_map[token] = u32(len(file.symbol_table))
        symbol_entry := Symbol_Table_Entry{
            offset = u32(len(file.buffer)),
            name = u32(len(file.string_table))
        }
        append(&file.symbol_table, symbol_entry)
        append(&file.string_table, token)
        append(&file.string_table, 0)
    }

    _ = expect_token(line, ":") or_return

    if token, ok = tokenizer_next(line); ok {
        return Unexpected_Token{
            column = line.token_start,
            expected = "'eol'", found = token_str(token)
        }
    }

    return nil
}

@(private = "file") STATIC_DATA_LENGTH_NOT_ENCODABLE_WORD_MESSAGE :: "array length exceeds maximum value of type 'word' (4,294,967,295)"
@(private = "file") STATIC_DATA_LENGTH_NOT_ENCODABLE_HALF_MESSAGE :: "array length exceeds maximum value of type 'half' (65,535)"
@(private = "file") STATIC_DATA_LENGTH_NOT_ENCODABLE_BYTE_MESSAGE :: "array length exceeds maximum value of type 'byte' (255)"
@(private = "file") STATIC_DATA_VALUE_NOT_ENCODABLE_WORD_MESSAGE :: "value is not encodable as type 'word'"
@(private = "file") STATIC_DATA_VALUE_NOT_ENCODABLE_HALF_MESSAGE :: "value is not encodable as type 'half'"
@(private = "file") STATIC_DATA_VALUE_NOT_ENCODABLE_BYTE_MESSAGE :: "value is not encodable as type 'byte'"

@(private = "file")
process_static_data :: proc(file: ^Source_File, line: ^Tokenizer, data_type_size: uint, depth: uint = 0) -> (size: uint, err: Line_Error) {
    assert(size_of(uint) >= 4)
    assert(data_type_size == SIZE_OF_WORD || data_type_size == SIZE_OF_HALF || data_type_size == SIZE_OF_BYTE)
    assert(depth <= 1)

    data_type_max: uint = (1 << (data_type_size * 8)) - 1

    keyword_column := line.token_start

    token: string = ---
    ok: bool = ---
    
    if _, ok = optional_token(line, "*"); ok { // auto array size
        if depth == 1 {
            return 0, Unexpected_Token{
                column = line.token_start,
                expected = "integer literal", found = string("'*'")
            }
        }

        // Save room for the array size before writing the data
        array_length_offset := len(file.buffer)
        for _ in 0..<data_type_size {
            append(&file.buffer, 0)
        }

        array_length: uint = ---
        token, _ = tokenizer_next(line)
        mnem := mnem_from_token(token)
        #partial switch mnem {
        case .word: array_length = process_static_data(file, line, SIZE_OF_WORD, depth = 1) or_return
        case .half: array_length = process_static_data(file, line, SIZE_OF_HALF, depth = 1) or_return
        case .byte: array_length = process_static_data(file, line, SIZE_OF_BYTE, depth = 1) or_return
        case .ascii: array_length = process_ascii(file, line) or_return
        case:
            return 0, Unexpected_Token{
                column = line.token_start,
                expected = "'word', 'half', 'byte', or 'ascii'", found = token_str(token)
            }
        }

        // Inject the array size into file.buffer before the array data
        if array_length > data_type_max {
            err := Not_Encodable{
                start_column = keyword_column,
                end_column = keyword_column, // no underline
            }
            switch data_type_size {
            case SIZE_OF_WORD: err.message = STATIC_DATA_LENGTH_NOT_ENCODABLE_WORD_MESSAGE
            case SIZE_OF_HALF: err.message = STATIC_DATA_LENGTH_NOT_ENCODABLE_HALF_MESSAGE
            case SIZE_OF_BYTE: err.message = STATIC_DATA_LENGTH_NOT_ENCODABLE_BYTE_MESSAGE
            }
            return 0, err
        }

        array_length_le := u32le(array_length)
        assign_at(&file.buffer, array_length_offset, ..mem.byte_slice(&array_length_le, data_type_size))

        return 0, nil
    }

    array_length: uint = 0
    for {
        _, negated := optional_token(line, "-")
        value_start_column := line.token_start

        value := expect_integer(line) or_return
        if negated {
            value = ~value + 1
        }

        pos_and_not_representable := (!negated && value > data_type_max)
        neg_and_not_representable := (negated && int(value) >> (data_type_size * 8 - 1) != -1)
        if pos_and_not_representable || neg_and_not_representable {
            err := Not_Encodable{
                start_column = value_start_column,
                end_column = line.token_end,
            }
            switch data_type_size {
            case SIZE_OF_WORD: err.message = STATIC_DATA_VALUE_NOT_ENCODABLE_WORD_MESSAGE
            case SIZE_OF_HALF: err.message = STATIC_DATA_VALUE_NOT_ENCODABLE_HALF_MESSAGE
            case SIZE_OF_BYTE: err.message = STATIC_DATA_VALUE_NOT_ENCODABLE_BYTE_MESSAGE
            }
            return 0, err
        }

        value_le := u32le(value)
        append(&file.buffer, ..mem.byte_slice(&value_le, data_type_size))

        array_length += 1

        if token, ok = optional_token(line, ",", eol = true); !ok {
            return 0, Unexpected_Token{
                column = line.token_start,
                expected = "','", found = token_str(token)
            }
        }
        if token[0] == '\n' {
            break
        }
    }

    return array_length, nil
}

@(private = "file")
process_ascii :: proc(file: ^Source_File, line: ^Tokenizer) -> (size: uint, err: Line_Error) {
    if _, err = expect_token(line, "\""); err != nil {
        err := err.(Unexpected_Token)
        err.expected = "string literal"
        return 0, err
    }

    string_length: uint = 0 
    for column := line.token_end; column < len(line.line); column += 1 {
        ch := line.line[column]
        if ch == '"' {
            column += 1
            line.token_end = column
            if token, ok := tokenizer_next(line); ok {
                return 0, Unexpected_Token{
                    column = line.token_start,
                    expected = "'eol'", found = token_str(token)
                }
            }
            return string_length, nil
        }
        if ch == '\\' {
            column += 1
            if column == len(line.line) {
                return 0, Unexpected_EOL{ column = column }
            }
            ch = line.line[column]
            switch ch {
                case 'n':
                    append(&file.buffer, '\n')
                    continue
                case 't':
                    append(&file.buffer, '\t')
                    continue
            }
        }
        append(&file.buffer, ch)
        string_length += 1
    }

    return 0, Unexpected_EOL{ column = len(line.line) }
}


@(private = "file") ALIGN_LESS_THAN_FOUR_MESSAGE :: "alignment value must be four or greater"
@(private = "file") ALIGN_NON_POWER_OF_TWO_MESSAGE :: "alignment value must be a power of two"

@(private = "file")
process_align :: proc(file: ^Source_File, line: ^Tokenizer) -> (err: Line_Error) {
    alignment := expect_integer(line) or_return

    if token, ok := tokenizer_next(line); ok {
        return Unexpected_Token{
            column = line.token_start,
            expected = "'eol'", found = token_str(token)
        }
    }

    if alignment < 4 {
        return Not_Encodable{
            start_column = line.token_start,
            end_column = line.token_end,
            message = ALIGN_LESS_THAN_FOUR_MESSAGE
        }
    }
    if !bits.is_power_of_two(alignment) {
        return Not_Encodable{
            start_column = line.token_start,
            end_column = line.token_end,
            message = ALIGN_NON_POWER_OF_TWO_MESSAGE
        }
    }

    alignment_padding := alignment - (len(file.buffer) % alignment)
    for _ in 0..<alignment_padding {
        append(&file.buffer, 0)
    }

    return nil
}

@(private = "file")
process_instruction :: proc(file: ^Source_File, line: ^Tokenizer, mnem: Mnemonic) -> (err: Line_Error) {
    instr := encode_instruction(line, mnem) or_return

    if token, ok := tokenizer_next(line); ok {
        return Unexpected_Token{
            column = line.token_start,
            expected = "'eol'", found = token_str(token)
        }
    }

    if relocation_symbol, ok := instr.relocation_symbol.(string); ok {
        symbol_index, ok := file.symbol_map[relocation_symbol]
        if !ok { // create symbol table entry
            symbol_index = u32(len(file.symbol_table))
            file.symbol_map[relocation_symbol] = symbol_index
            symbol_entry := Symbol_Table_Entry{
                offset = max(u32), // unknown at this time
                name = u32(len(file.string_table))
            }
            append(&file.symbol_table, symbol_entry)
            append(&file.string_table, relocation_symbol)
            append(&file.string_table, 0)
        }
        relocation_entry := Relocation_Table_Entry{
            offset = u32(len(file.buffer)),
            symbol = symbol_index
        }
        append(&file.relocation_table, relocation_entry)
    }

    append(&file.buffer, ..mem.byte_slice(&instr.machine_word, size_of(u32le)))
    if machine_word2, ok := instr.machine_word2.(u32le); ok {
        append(&file.buffer, ..mem.byte_slice(&machine_word2, size_of(u32le)))
    }

    return nil
}
