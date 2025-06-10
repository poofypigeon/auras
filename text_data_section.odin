package auras

import "core:fmt"
import "core:math/bits"
import "core:mem"
import "core:slice"
import "core:strings"
import "core:unicode"

Relocation_Table_Entry :: struct {
    offset: u32, // offset from beginning of section
    symbol_index: u32, // index into symbol table
}

Symbol_Table_Entry :: struct {
    offset: u32, // offset from beginning of section
    name_index:   u32, // index into string table
}

UNDEFINED_OFFSET :: max(u32)

Text_Data_Section :: struct {
    buffer: [dynamic]u8,
    symbol_table: [dynamic]Symbol_Table_Entry,
    relocation_table: [dynamic]Relocation_Table_Entry,
    string_table: [dynamic]u8, // first entry is section name
    symbol_map: map[string]u32,
}

text_data_section_init :: proc() -> Text_Data_Section {
    return Text_Data_Section{
        buffer           = make([dynamic]u8, 0, 256),
        string_table     = make([dynamic]u8, 0, 256),
        symbol_map       = make(map[string]u32),
        relocation_table = make([dynamic]Relocation_Table_Entry, 0, 64),
        symbol_table     = make([dynamic]Symbol_Table_Entry, 0, 64),
    }
}

text_data_section_cleanup :: proc(section: ^Text_Data_Section) {
    delete(section.buffer)
    delete(section.string_table)
    delete(section.symbol_map)
    delete(section.relocation_table)
    delete(section.symbol_table)
}

process_line :: proc(section: ^Text_Data_Section, line: string) -> (directive: bool, err: Line_Error) {
    token: string = ---
    ok: bool = ---

    line := Tokenizer{ line = line }

    // Ignore empty lines and defer directives
    token, ok = optional_token(&line, ".", opt_eol = true) or_return
    if ok {
        if token[0] == '.' {
            return true, nil
        }
        return false, nil
    }

    if section == nil {
        return false, Missing_Section_Declaration{ column = line.token_start }
    }

    if !unicode.is_space(rune(line.line[0])) {
        process_local_label(section, &line) or_return
        return false, nil
    }

    token, _ = tokenizer_next(&line) or_return

    mnem := mnem_from_token(token)
    #partial switch mnem {
    case .invalid:
        return false, Unexpected_Token{
            column = line.token_start,
            expected = "mnemonic", found = token_str(token)
        }
    case .addr: process_addr(section, &line) or_return
    case .word: process_static_data(section, &line, SIZE_OF_WORD) or_return
    case .half: process_static_data(section, &line, SIZE_OF_HALF) or_return
    case .byte: process_static_data(section, &line, SIZE_OF_BYTE) or_return
    case .ascii: process_ascii(section, &line) or_return
    case .align: process_align(section, &line) or_return
    case: process_instruction(section, &line, mnem) or_return
    }
   
    return false, nil
}

@(private = "file")
process_local_label :: proc(section: ^Text_Data_Section, line: ^Tokenizer) -> (err: Line_Error) {
    token: string = ---
    ok: bool = ---

    token, _ = tokenizer_next(line) or_return

    if !is_symbol_char(token[0]) {
        return Unexpected_Token{
            column = line.token_start,
            expected = "label", found = token_str(token)
        }
    }

    // Ensure labels are word aligned
    current_alignment := len(section.buffer) % SIZE_OF_WORD
    alignment_padding := current_alignment == 0 ? 0 : SIZE_OF_WORD - current_alignment
    for _ in 0..<alignment_padding {
        append(&section.buffer, 0)
    }

    symbol_index: u32 = ---
    if symbol_index, ok = section.symbol_map[token]; ok {
        if section.symbol_table[symbol_index].offset != UNDEFINED_OFFSET {
            return Redefinition{
                column = line.token_start,
                label = token,
            }
        }
        section.symbol_table[symbol_index].offset = u32(len(section.buffer))
    } else { // create symbol table entry
        section.symbol_map[token] = u32(len(section.symbol_table))
        symbol_entry := Symbol_Table_Entry{
            offset = u32(len(section.buffer)),
            name_index = u32(len(section.string_table))
        }
        append(&section.symbol_table, symbol_entry)
        append(&section.string_table, token)
        append(&section.string_table, 0)
    }

    _ = expect_token(line, ":") or_return

    eol: bool = ---
    token, eol = tokenizer_next(line) or_return
    if !eol {
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
process_addr :: proc(section: ^Text_Data_Section, line: ^Tokenizer) -> (err: Line_Error) {
    relocation_symbol := expect_symbol(line) or_return
    add_relocation_symbol(section, relocation_symbol)

    // Align to word boundary
    misalignment := len(section.buffer) % SIZE_OF_WORD
    if misalignment != 0 {
        alignment_padding := SIZE_OF_WORD - misalignment
        for _ in 0..<alignment_padding {
            append(&section.buffer, 0)
        }
    }

    append(&section.buffer, 0, 0, 0, 0)
    return nil
}

@(private = "file")
process_static_data :: proc(section: ^Text_Data_Section, line: ^Tokenizer, data_type_size: uint, depth: uint = 0) -> (size: uint, err: Line_Error) {
    assert(size_of(uint) >= 4)
    assert(data_type_size == SIZE_OF_WORD || data_type_size == SIZE_OF_HALF || data_type_size == SIZE_OF_BYTE)
    assert(depth <= 1)

    data_type_max: uint = (1 << (data_type_size * 8)) - 1

    keyword_column := line.token_start

    token: string = ---
    ok: bool = ---

    _, ok = optional_token(line, "*") or_return
    if ok { // auto array size
        if depth == 1 {
            return 0, Unexpected_Token{
                column = line.token_start,
                expected = "integer literal", found = string("'*'")
            }
        }

        // Save room for the array size before writing the data
        array_length_offset := len(section.buffer)
        for _ in 0..<data_type_size {
            append(&section.buffer, 0)
        }

        array_length: uint = ---
        token, _ = tokenizer_next(line) or_return
        mnem := mnem_from_token(token)
        #partial switch mnem {
        case .word: array_length = process_static_data(section, line, SIZE_OF_WORD, depth = 1) or_return
        case .half: array_length = process_static_data(section, line, SIZE_OF_HALF, depth = 1) or_return
        case .byte: array_length = process_static_data(section, line, SIZE_OF_BYTE, depth = 1) or_return
        case .ascii: array_length = process_ascii(section, line) or_return
        case:
            return 0, Unexpected_Token{
                column = line.token_start,
                expected = "'word', 'half', 'byte', or 'ascii'", found = token_str(token)
            }
        }

        // Inject the array size into section.buffer before the array data
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
        assign_at(&section.buffer, array_length_offset, ..mem.byte_slice(&array_length_le, data_type_size))

        return 0, nil
    }

    array_length: uint = 0
    for {
        _, negated := optional_token(line, "-") or_return
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
        append(&section.buffer, ..mem.byte_slice(&value_le, data_type_size))

        array_length += 1

        token, ok = optional_token(line, ",", opt_eol = true) or_return
        if !ok {
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
process_ascii :: proc(section: ^Text_Data_Section, line: ^Tokenizer) -> (size: uint, err: Line_Error) {
    token, eol := tokenizer_next(line) or_return
    if eol {
        return 0, Unexpected_EOL{ column = line.token_start }
    }
    if token[0] != '"' {
        return 0, Unexpected_Token{
            column = line.token_start,
            expected = "string literal", found = token_str(token),
        }
    }

    assert(token[len(token)-1] == '"', "missing quote")
    size = 0
    for i := 1; i < len(token)-1; i += 1 {
        ch := token[i]
        if token[i] == '\\' {
            ch = token[i+1]
            switch ch {
            case '\\': ch = '\\'
            case '\'': ch = '\''
            case 'n': ch = '\n'
            case 't': ch = '\t'
            case: return 0, Unknown_Escape_Sequence{ column = line.token_start + uint(i) }
            }
            i += 1
        }
        append(&section.buffer, ch)
        size += 1
    }

    return size, nil
}


@(private = "file") ALIGN_LESS_THAN_FOUR_MESSAGE :: "alignment value must be four or greater"
@(private = "file") ALIGN_NON_POWER_OF_TWO_MESSAGE :: "alignment value must be a power of two"

@(private = "file")
process_align :: proc(section: ^Text_Data_Section, line: ^Tokenizer) -> (err: Line_Error) {
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

    alignment_padding := alignment - (len(section.buffer) % alignment)
    for _ in 0..<alignment_padding {
        append(&section.buffer, 0)
    }

    token, eol := tokenizer_next(line) or_return
    if !eol {
        return Unexpected_Token{
            column = line.token_start,
            expected = "'eol'", found = token_str(token)
        }
    }

    return nil
}

@(private = "file")
process_instruction :: proc(section: ^Text_Data_Section, line: ^Tokenizer, mnem: Mnemonic) -> (err: Line_Error) {
    instr := encode_instruction_from_mnemonic(line, mnem) or_return

    token, eol := tokenizer_next(line) or_return
    if !eol {
        return Unexpected_Token{
            column = line.token_start,
            expected = "'eol'", found = token_str(token)
        }
    }

    if relocation_symbol, ok := instr.relocation_symbol.(string); ok {
        add_relocation_symbol(section, relocation_symbol)
    }

    // Align to word boundary
    misalignment := len(section.buffer) % SIZE_OF_WORD
    if misalignment != 0 {
        alignment_padding := SIZE_OF_WORD - misalignment
        for _ in 0..<alignment_padding {
            append(&section.buffer, 0)
        }
    }

    machine_word_le := u32le(instr.machine_word)
    append(&section.buffer, ..mem.byte_slice(&machine_word_le, size_of(u32le)))
    if machine_word2, ok := instr.machine_word2.(u32); ok {
        machine_word_le = u32le(machine_word2)
        append(&section.buffer, ..mem.byte_slice(&machine_word_le, size_of(u32le)))
    }

    return nil
}

@(private = "file")
add_relocation_symbol :: proc(section: ^Text_Data_Section, relocation_symbol: string) {
    symbol_index, ok := section.symbol_map[relocation_symbol]
    if !ok { // create symbol table entry
        symbol_index = u32(len(section.symbol_table))
        section.symbol_map[relocation_symbol] = symbol_index
        symbol_entry := Symbol_Table_Entry{
            offset = UNDEFINED_OFFSET, // unknown at this time
            name_index = u32(len(section.string_table))
        }
        append(&section.symbol_table, symbol_entry)
        append(&section.string_table, relocation_symbol)
        append(&section.string_table, 0)
    }
    relocation_entry := Relocation_Table_Entry{
        offset = u32(len(section.buffer)),
        symbol_index = symbol_index
    }
    append(&section.relocation_table, relocation_entry)
}
