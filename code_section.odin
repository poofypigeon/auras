package auras

import "core:fmt"
import "core:math/bits"
import "core:mem"
import "core:slice"
import "core:strings"
import "core:unicode"
import "base:runtime"

Relocation_Table_Entry :: struct {
    offset: u32, // offset from beginning of section
    symbol: u32, // index into symbol table
}

Symbol_Table_Entry :: struct {
    offset: u32, // offset from beginning of section
    name:   u32, // index into string table
}

@(private = "file")
UNDEFINED_OFFSET :: max(u32)

Code_Section :: struct {
    buffer: [dynamic]u8,
    symbol_table: [dynamic]Symbol_Table_Entry,
    relocation_table: [dynamic]Relocation_Table_Entry,
    string_table: [dynamic]u8,
    symbol_map: map[string]u32,
}

code_section_init :: proc() -> Code_Section {
    return Code_Section{
        buffer           = make([dynamic]u8, 0, 256),
        string_table     = make([dynamic]u8, 0, 256),
        symbol_map       = make(map[string]u32),
        relocation_table = make([dynamic]Relocation_Table_Entry, 0, 64),
        symbol_table     = make([dynamic]Symbol_Table_Entry, 0, 64)
    }
}

code_section_cleanup :: proc(code: ^Code_Section) {
    delete(code.buffer)
    delete(code.string_table)
    delete(code.symbol_map)
    delete(code.relocation_table)
    delete(code.symbol_table)
}

code_from_text :: proc(text: string) -> (code: Code_Section, ok: bool) {
    code = code_section_init()
    text_ref := text // preserve original text string

    line_number: uint = 1
    for line_text in strings.split_lines_iterator(&text_ref) {
        if err := process_line(&code, line_text); err != nil {
            // code_from_text is going to be replaced so just passing empty file_path for now
            print_line_error("", line_number, line_text, err)
            code_section_cleanup(&code)
            return Code_Section{}, false
        }
        line_number += 1
    }

    for relocation_entry in code.relocation_table {
        symbol_entry := code.symbol_table[relocation_entry.symbol]
        if symbol_entry.offset == UNDEFINED_OFFSET {
            symbol_string := runtime.cstring_to_string(cstring(&code.string_table[symbol_entry.name]))
            text_ref = text
            line_number = 1
            for line_text in strings.split_lines_iterator(&text_ref) {
                line := Tokenizer{ line = line_text }
                for token in tokenizer_next(&line) {
                    if token == symbol_string {
                        err := Undefined_Symbol{ symbol = symbol_string, column = line.token_start }
                        print_line_error("", line_number, line_text, err)
                        code_section_cleanup(&code)
                        return Code_Section{}, false
                    }
                }
                line_number += 1
            }
            fmt.println("line_number:", line_number)
            panic("undefined symbol not found in text")
        }

        highest_nybble := code.buffer[relocation_entry.offset + SIZE_OF_WORD-1] >> 4
        switch highest_nybble {
        case 0b1001, 0b1011: // branch
            b_word := (^u32le)(&code.buffer[relocation_entry.offset])
            assert(b_word^ & 0x00FF_FFFF == 0, "unexpected instruction at branch relocation offset")

            offset := int(symbol_entry.offset) - int(relocation_entry.offset)
            assert(offset % SIZE_OF_WORD == 0, "misaligned jump offset")
            offset >>= 2
            if (offset >> 23 != 0 && offset >> 23 != 1 && offset >> 23 != -1) {
                // TODO trampoline once sections are supported
                panic("branch offset out of range")
            }

            offset_le := u32le(offset)
            b_word^ |= offset_le & 0x00FF_FFFF

        case 0b1100: // m32
            mvi_word := (^u32le)(&code.buffer[relocation_entry.offset])
            assert(mvi_word^ & 0x00FF_FFFF == 0x0000_0000, "unexpected instruction at m32 relocation offset")

            add_word := (^u32le)(&code.buffer[relocation_entry.offset + SIZE_OF_WORD])
            assert(add_word^ & 0x000F_FFFF == 0x0001_6000, "unexpected instruction at m32 relocation offset + 4")

            offset_le := u32le(symbol_entry.offset)
            mvi_word^ |= offset_le & 0x00FF_FFFF
            add_word^ |= offset_le >> 24

        case:
            panic("unexpected instruction at relocation offset")
        }
    }

    return code, true
}

process_line :: proc(code: ^Code_Section, line: string) -> (err: Line_Error) {
    token: string = ---
    ok: bool = ---

    line := Tokenizer{ line = line }

    if token, ok = optional_token(&line, eol = true); ok {
        // if token[0] == '.' {
        //     // process_directive(code, &line) or_return
        // }
        return nil
    }

    if !unicode.is_space(rune(line.line[0])) {
        process_local_label(code, &line) or_return
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
    case .word: process_static_data(code, &line, SIZE_OF_WORD) or_return
    case .half: process_static_data(code, &line, SIZE_OF_HALF) or_return
    case .byte: process_static_data(code, &line, SIZE_OF_BYTE) or_return
    case .ascii: process_ascii(code, &line) or_return
    case .align: process_align(code, &line) or_return
    case: process_instruction(code, &line, mnem)  or_return
    }
   
    return nil
}

@(private = "file")
process_local_label :: proc(code: ^Code_Section, line: ^Tokenizer) -> (err: Line_Error) {
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
    current_alignment := len(code.buffer) % SIZE_OF_WORD
    alignment_padding := current_alignment == 0 ? 0 : SIZE_OF_WORD - current_alignment
    for _ in 0..<alignment_padding {
        append(&code.buffer, 0)
    }

    symbol_index: u32 = ---
    if symbol_index, ok = code.symbol_map[token]; ok {
        if code.symbol_table[symbol_index].offset != UNDEFINED_OFFSET {
            return Redefinition{
                column = line.token_start,
                label = token,
            }
        }
        code.symbol_table[symbol_index].offset = u32(len(code.buffer))
    } else { // create symbol table entry
        code.symbol_map[token] = u32(len(code.symbol_table))
        symbol_entry := Symbol_Table_Entry{
            offset = u32(len(code.buffer)),
            name = u32(len(code.string_table))
        }
        append(&code.symbol_table, symbol_entry)
        append(&code.string_table, token)
        append(&code.string_table, 0)
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
process_static_data :: proc(code: ^Code_Section, line: ^Tokenizer, data_type_size: uint, depth: uint = 0) -> (size: uint, err: Line_Error) {
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
        array_length_offset := len(code.buffer)
        for _ in 0..<data_type_size {
            append(&code.buffer, 0)
        }

        array_length: uint = ---
        token, _ = tokenizer_next(line)
        mnem := mnem_from_token(token)
        #partial switch mnem {
        case .word: array_length = process_static_data(code, line, SIZE_OF_WORD, depth = 1) or_return
        case .half: array_length = process_static_data(code, line, SIZE_OF_HALF, depth = 1) or_return
        case .byte: array_length = process_static_data(code, line, SIZE_OF_BYTE, depth = 1) or_return
        case .ascii: array_length = process_ascii(code, line) or_return
        case:
            return 0, Unexpected_Token{
                column = line.token_start,
                expected = "'word', 'half', 'byte', or 'ascii'", found = token_str(token)
            }
        }

        // Inject the array size into code.buffer before the array data
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
        assign_at(&code.buffer, array_length_offset, ..mem.byte_slice(&array_length_le, data_type_size))

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
        append(&code.buffer, ..mem.byte_slice(&value_le, data_type_size))

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
process_ascii :: proc(code: ^Code_Section, line: ^Tokenizer) -> (size: uint, err: Line_Error) {
    if _, err = expect_token(line, "\"", no_alloc = true); err != nil {
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
                    append(&code.buffer, '\n') // TODO add test for string length with newline
                case 't':
                    append(&code.buffer, '\t')
            }
            string_length += 1
            continue
        }
        append(&code.buffer, ch)
        string_length += 1
    }

    return 0, Unexpected_EOL{ column = len(line.line) }
}


@(private = "file") ALIGN_LESS_THAN_FOUR_MESSAGE :: "alignment value must be four or greater"
@(private = "file") ALIGN_NON_POWER_OF_TWO_MESSAGE :: "alignment value must be a power of two"

@(private = "file")
process_align :: proc(code: ^Code_Section, line: ^Tokenizer) -> (err: Line_Error) {
    alignment := expect_integer(line) or_return

    if alignment < SIZE_OF_WORD {
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

    alignment_padding := alignment - (len(code.buffer) % alignment)
    for _ in 0..<alignment_padding {
        append(&code.buffer, 0)
    }

    if token, ok := tokenizer_next(line); ok {
        return Unexpected_Token{
            column = line.token_start,
            expected = "'eol'", found = token_str(token)
        }
    }

    return nil
}

@(private = "file")
process_instruction :: proc(code: ^Code_Section, line: ^Tokenizer, mnem: Mnemonic) -> (err: Line_Error) {
    instr := encode_instruction_from_mnemonic(line, mnem) or_return

    if token, ok := tokenizer_next(line); ok {
        return Unexpected_Token{
            column = line.token_start,
            expected = "'eol'", found = token_str(token)
        }
    }

    if relocation_symbol, ok := instr.relocation_symbol.(string); ok {
        symbol_index, ok := code.symbol_map[relocation_symbol]
        if !ok { // create symbol table entry
            symbol_index = u32(len(code.symbol_table))
            code.symbol_map[relocation_symbol] = symbol_index
            symbol_entry := Symbol_Table_Entry{
                offset = UNDEFINED_OFFSET, // unknown at this time
                name = u32(len(code.string_table))
            }
            append(&code.symbol_table, symbol_entry)
            append(&code.string_table, relocation_symbol)
            append(&code.string_table, 0)
        }
        relocation_entry := Relocation_Table_Entry{
            offset = u32(len(code.buffer)),
            symbol = symbol_index
        }
        append(&code.relocation_table, relocation_entry)
    }

    machine_word_le := u32le(instr.machine_word)
    append(&code.buffer, ..mem.byte_slice(&machine_word_le, size_of(u32le)))
    if machine_word2, ok := instr.machine_word2.(u32); ok {
        machine_word_le = u32le(machine_word2)
        append(&code.buffer, ..mem.byte_slice(&machine_word_le, size_of(u32le)))
    }

    return nil
}
