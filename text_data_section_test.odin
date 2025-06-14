#+private

package auras

import "core:bytes"
import "core:mem"
import "core:testing"

@(private = "file")
produces_unexpected_token_error :: #force_inline proc(file: ^Text_Data_Section, str: string) -> bool {
    directive, err := process_line(file, str)
    if directive do return false
    e, ok := err.(Unexpected_Token)
    if ok {
        #partial switch expected in e.expected {
        case [dynamic]string: delete(expected)
        }
    }
    return ok
}

@(test)
test_missing_section_declaration :: proc(t: ^testing.T) {
    directive, err := process_line(nil, "anything")
    testing.expect(t, directive == false)
    _, ok := err.(Missing_Section_Declaration)
    testing.expect(t, ok)

}

@(test)
test_empty_line :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    directive, err := process_line(&file, "")
    testing.expect(t, directive == false)
    testing.expect(t, err == nil)
    directive, err = process_line(&file, "    ")
    testing.expect(t, directive == false)
    testing.expect(t, err == nil)
    directive, err = process_line(&file, "; some comment")
    testing.expect(t, directive == false)
    testing.expect(t, err == nil)
    directive, err = process_line(&file, "    ; some comment")
    testing.expect(t, directive == false)
    testing.expect(t, err == nil)
}

@(test)
test_local_label_non_label_character :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    testing.expect(t, produces_unexpected_token_error(&file, "0"))
}

@(test)
test_local_label_missing_colon :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    testing.expect(t, produces_unexpected_token_error(&file, "L1"))
}

@(test)
test_local_label_unexpected_token :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    testing.expect(t, produces_unexpected_token_error(&file, "L1:!"))
}

@(test)
test_local_label :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    directive, err := process_line(&file, "L1:")
    testing.expect(t, directive == false)
    testing.expect(t, err == nil)

    // file.buffer
    testing.expect_value(t, len(file.buffer), 0)
    // file.symbol_table
    testing.expect_value(t, len(file.symbol_table), 1)
    testing.expect_value(t, file.symbol_table[0], Symbol_Table_Entry{ offset = 0, name_index = 0 })
    // file.relocation_table
    testing.expect_value(t, len(file.relocation_table), 0)
    // file.string_table
    expected_string_table := []u8{ 'L', '1', 0 }
    testing.expect(t, bytes.compare(file.string_table[:], expected_string_table) == 0)
    // file.symbol_map
    index, ok := file.symbol_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 0)
}

@(test)
test_local_label_redefinition :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    err: Line_Error
    _, err = process_line(&file, "L1:")
    _, err = process_line(&file, "L1:")
    _, ok := err.(Redefinition)
    testing.expect(t, ok)
}

@(test)
test_invalid_mnemonic :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    testing.expect(t, produces_unexpected_token_error(&file, "    bad"))
}

@(test)
test_instruction_extraneous_token :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    testing.expect(t, produces_unexpected_token_error(&Text_Data_Section{}, "    b label!"))
    testing.expect(t, produces_unexpected_token_error(&Text_Data_Section{}, "    nop!"))
    testing.expect(t, produces_unexpected_token_error(&Text_Data_Section{}, "    add r1, r2, r3 lsl r4!"))
    testing.expect(t, produces_unexpected_token_error(&Text_Data_Section{}, "    add r1, r2, r3 lsl 4!"))
    testing.expect(t, produces_unexpected_token_error(&Text_Data_Section{}, "    lsl r1, r2, r3!"))
    testing.expect(t, produces_unexpected_token_error(&Text_Data_Section{}, "    b r1!"))
    testing.expect(t, produces_unexpected_token_error(&Text_Data_Section{}, "    mov r1, r2!"))
    testing.expect(t, produces_unexpected_token_error(&Text_Data_Section{}, "    smv r1!"))
    testing.expect(t, produces_unexpected_token_error(&Text_Data_Section{}, "    m32 r1, 0!"))
    testing.expect(t, produces_unexpected_token_error(&Text_Data_Section{}, "    swi 0xAA!"))
    testing.expect(t, produces_unexpected_token_error(&Text_Data_Section{}, "    mvi r1, 0!"))
    testing.expect(t, produces_unexpected_token_error(&Text_Data_Section{}, "    scl r1!"))
    testing.expect(t, produces_unexpected_token_error(&Text_Data_Section{}, "    sst r1!"))
}

@(test)
test_instruction_alignment :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    _, err := process_line(&file, "    byte 0x11, 0x22")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    nop")
    expected_buffer_words := []u32le{ 0x0000_2211, 0x4001_0000 }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_words)) == 0)
}

@(test)
test_general_instruction :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    directive, err := process_line(&file, "    mvi r1, 0xAA")
    testing.expect(t, directive == false)
    testing.expect(t, err == nil)

    // file.buffer
    expected_buffer_word: u32le = 0xC100_00AA
    testing.expect(t, bytes.compare(file.buffer[:], mem.ptr_to_bytes(&expected_buffer_word)) == 0)
    // file.symbol_table
    testing.expect_value(t, len(file.symbol_table), 0)
    // file.relocation_table
    testing.expect_value(t, len(file.relocation_table), 0)
    // file.string_table
    testing.expect_value(t, len(file.string_table), 0)
    // file.symbol_map
    testing.expect_value(t, len(file.symbol_map), 0)
}

@(test)
test_m32_integer_literal :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    _, err := process_line(&file, "    m32 r1, 0xDEAD_BEEF")
    testing.expect(t, err == nil)

    // file.buffer
    expected_buffer_words := []u32le{ 0xC1AD_BEEF, 0x7111_60DE }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_words)) == 0)
    // file.symbol_table
    testing.expect_value(t, len(file.symbol_table), 0)
    // file.relocation_table
    testing.expect_value(t, len(file.relocation_table), 0)
    // file.string_table
    testing.expect_value(t, len(file.string_table), 0)
    // file.symbol_map
    testing.expect_value(t, len(file.symbol_map), 0)
}

@(test)
test_m32_relocation :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    _, err := process_line(&file, "    m32 r1, L1")
    testing.expect(t, err == nil)

    // file.buffer
    expected_buffer_words := []u32le{ 0xC100_0000, 0x7111_6000 }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_words)) == 0)
    // file.symbol_table
    testing.expect_value(t, len(file.symbol_table), 1)
    testing.expect_value(t, file.symbol_table[0], Symbol_Table_Entry{ offset = max(u32), name_index = 0 })
    // file.relocation_table
    testing.expect_value(t, len(file.relocation_table), 1)
    testing.expect_value(t, file.relocation_table[0], Relocation_Table_Entry{ offset = 0, symbol_index = 0 })
    // file.string_table
    expected_string_table := []u8{ 'L', '1', 0 }
    testing.expect(t, bytes.compare(file.string_table[:], expected_string_table) == 0)
    // file.symbol_map
    index, ok := file.symbol_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 0)
}

@(test)
test_branch_relocation :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    _, err := process_line(&file, "    beq L1")
    testing.expect(t, err == nil)

    // file.buffer
    expected_buffer_word: u32le = 0x9000_0000
    testing.expect(t, bytes.compare(file.buffer[:], mem.ptr_to_bytes(&expected_buffer_word)) == 0)
    // file.symbol_table
    testing.expect_value(t, len(file.symbol_table), 1)
    testing.expect_value(t, file.symbol_table[0], Symbol_Table_Entry{ offset = max(u32), name_index = 0 })
    // file.relocation_table
    testing.expect_value(t, len(file.relocation_table), 1)
    // file.string_table
    expected_string_table := []u8{ 'L', '1', 0 }
    testing.expect(t, bytes.compare(file.string_table[:], expected_string_table) == 0)
    // file.symbol_map
    index, ok := file.symbol_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 0)
}

@(test)
test_addr_relocation :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    _, err := process_line(&file, "    addr L1")
    testing.expect(t, err == nil)

    // file.buffer
    expected_buffer_word: u32le = 0x0000_0000
    testing.expect(t, bytes.compare(file.buffer[:], mem.ptr_to_bytes(&expected_buffer_word)) == 0)
    // file.symbol_table
    testing.expect_value(t, len(file.symbol_table), 1)
    testing.expect_value(t, file.symbol_table[0], Symbol_Table_Entry{ offset = max(u32), name_index = 0 })
    // file.relocation_table
    testing.expect_value(t, len(file.relocation_table), 1)
    // file.string_table
    expected_string_table := []u8{ 'L', '1', 0 }
    testing.expect(t, bytes.compare(file.string_table[:], expected_string_table) == 0)
    // file.symbol_map
    index, ok := file.symbol_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 0)
}

@(test)
test_addr_alignment :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    _, err := process_line(&file, "    byte 0x11, 0x22")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    addr L1")
    expected_buffer_words := []u32le{ 0x0000_2211, 0x0000_0000 }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_words)) == 0)
}

@(test)
test_multiple_labels_and_relocations :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    err: Line_Error
    _, err = process_line(&file, "L1:")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    m32 r1, L2")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "L2:")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    beq L1")
    testing.expect(t, err == nil)

    // file.buffer
    expected_buffer_words := []u32le{ 0xC100_0000, 0x7111_6000, 0x9000_0000 }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_words)) == 0)
    // file.symbol_table
    testing.expect_value(t, len(file.symbol_table), 2)
    testing.expect_value(t, file.symbol_table[0], Symbol_Table_Entry{ offset = 0, name_index = 0 })
    testing.expect_value(t, file.symbol_table[1], Symbol_Table_Entry{ offset = 8, name_index = 3 })
    // file.relocation_table
    testing.expect_value(t, len(file.relocation_table), 2)
    testing.expect_value(t, file.relocation_table[0], Relocation_Table_Entry{ offset = 0, symbol_index = 1 })
    testing.expect_value(t, file.relocation_table[1], Relocation_Table_Entry{ offset = 8, symbol_index = 0 })
    // file.string_table
    expected_string_table := []u8{ 'L', '1', 0, 'L', '2', 0}
    testing.expect(t, bytes.compare(file.string_table[:], expected_string_table) == 0)
    // file.symbol_map
    index, ok := file.symbol_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 0)
    index, ok = file.symbol_map["L2"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 1)
}

@(test)
test_static_data_out_of_range :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    err: Line_Error
    ok: bool

    _, err = process_line(&file, "    word 0x1_0000_0000")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    _, err = process_line(&file, "    half 0x1_0000")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    _, err = process_line(&file, "    byte 0x100")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)

    _, err = process_line(&file, "    word -0x8000_0001")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    _, err = process_line(&file, "    half -0x8001")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    _, err = process_line(&file, "    byte -0x81")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
}

@(test)
test_static_data_unexpected_token :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    testing.expect(t, produces_unexpected_token_error(&file, "    word!"))
    testing.expect(t, produces_unexpected_token_error(&file, "    word 0,!"))
}

@(test)
test_static_data_single_value :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    err: Line_Error
    _, err = process_line(&file, "    word 0xDEAD_BEEF")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    word -1")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    half 0xBEEF")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    half -1")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    byte 0xAA")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    byte -1")
    testing.expect(t, err == nil)

    // file.buffer
    expected_buffer_bytes := []u8{ 0xEF, 0xBE, 0xAD, 0xDE, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xBE, 0xFF, 0xFF, 0xAA, 0xFF }
    testing.expect(t, bytes.compare(file.buffer[:], expected_buffer_bytes) == 0)
    // file.symbol_table
    testing.expect_value(t, len(file.symbol_table), 0)
    // file.relocation_table
    testing.expect_value(t, len(file.relocation_table), 0)
    // file.string_table
    testing.expect_value(t, len(file.string_table), 0)
    // file.symbol_map
    testing.expect_value(t, len(file.symbol_map), 0)
}

@(test)
test_static_data_multiple_values :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    err: Line_Error
    _, err = process_line(&file, "    word 0, 1, 2, 3")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    half 0, 1, 2, 3")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    byte 0, 1, 2, 3")
    testing.expect(t, err == nil)

    expected_buffer_words := []u32le{ 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(file.buffer[:4*SIZE_OF_WORD], mem.slice_to_bytes(expected_buffer_words)) == 0)
    expected_buffer_halfs := []u16le{ 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(file.buffer[4*SIZE_OF_WORD:][:4*SIZE_OF_HALF], mem.slice_to_bytes(expected_buffer_halfs)) == 0)
    expected_buffer_bytes := []u8{ 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(file.buffer[4*SIZE_OF_WORD:][4*SIZE_OF_HALF:][:4*SIZE_OF_BYTE], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_static_data_auto_length_unexpected_token :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    testing.expect(t, produces_unexpected_token_error(&file, "    word *!"))
    testing.expect(t, produces_unexpected_token_error(&file, "    word * word!"))
    testing.expect(t, produces_unexpected_token_error(&file, "    word * word *"))
}

@(test)
test_static_data_multiple_values_auto_length :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    err: Line_Error
    _, err = process_line(&file, "    word * word 0, 1, 2, 3")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    half * half 0, 1, 2, 3")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    byte * byte 0, 1, 2, 3")
    testing.expect(t, err == nil)

    expected_buffer_words := []u32le{ 4, 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(file.buffer[:5*SIZE_OF_WORD], mem.slice_to_bytes(expected_buffer_words)) == 0)
    expected_buffer_halfs := []u16le{ 4, 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(file.buffer[5*SIZE_OF_WORD:][:5*SIZE_OF_HALF], mem.slice_to_bytes(expected_buffer_halfs)) == 0)
    expected_buffer_bytes := []u8{ 4, 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(file.buffer[5*SIZE_OF_WORD:][5*SIZE_OF_HALF:][:5*SIZE_OF_BYTE], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_static_data_ascii_unexpected_token :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    _, err := process_line(&file, "    ascii!")
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_static_data_ascii_unexpected_eol :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    err: Line_Error
    ok: bool

    _, err = process_line(&file, `    ascii "`)
    _, ok = err.(Unexpected_EOL)
    testing.expect(t, ok)
    _, err = process_line(&file, `    ascii "\"`)
    _, ok = err.(Unexpected_EOL)
    testing.expect(t, ok)
}

@(test)
test_static_data_ascii_unknown_escape_sequence :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    err: Line_Error
    ok: bool

    _, err = process_line(&file, `    ascii "\0"`)
    _, ok = err.(Unknown_Escape_Sequence)
    testing.expect(t, ok)
    _, err = process_line(&file, `    ascii "\x"`)
    _, ok = err.(Unknown_Escape_Sequence)
    testing.expect(t, ok)
    _, err = process_line(&file, `    ascii "\$"`)
    _, ok = err.(Unknown_Escape_Sequence)
    testing.expect(t, ok)
}


@(test)
test_static_data_ascii :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    _, err := process_line(&file, `    ascii "\tabc\n"`)
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ '\t', 'a', 'b', 'c', '\n' }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_static_data_ascii_auto_length :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    _, err := process_line(&file, `    byte * ascii "ascii"`)
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ 5, 'a', 's', 'c', 'i', 'i' }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_static_data_ascii_auto_length_escape_characters :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    _, err := process_line(&file, `    byte * ascii "\tabc\n"`)
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ 5, '\t', 'a', 'b', 'c', '\n' }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_align_non_power_of_two :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    err: Line_Error
    ok: bool

    _, err = process_line(&file, "    align 3")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    _, err = process_line(&file, "    align 5")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
}

@(test)
test_align :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    err: Line_Error
    _, err = process_line(&file, "    byte 0xAA")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "    align 4")
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ 0xAA, 0, 0, 0 }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)

    _, err = process_line(&file, "    align 8")
    testing.expect(t, err == nil)

    expected_buffer_bytes = []u8{ 0xAA, 0, 0, 0, 0, 0, 0, 0 }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_label_alignment :: proc(t: ^testing.T) {
    file := text_data_section_init()
    defer text_data_section_cleanup(&file)

    err: Line_Error
    _, err = process_line(&file, "    byte 0xAA")
    testing.expect(t, err == nil)
    _, err = process_line(&file, "L1:")
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ 0xAA, 0, 0, 0 }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)

    testing.expect_value(t, len(file.symbol_table), 1)
    testing.expect_value(t, file.symbol_table[0], Symbol_Table_Entry{ offset = 4, name_index = 0 })
}
