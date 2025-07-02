#+private

package auras

import "core:bytes"
import "core:mem"
import "core:testing"

@(private = "file")
produces_unexpected_token_error :: #force_inline proc(section: ^Code_Section, str: string, object_strings: ^Object_Strings) -> bool {
    directive, err := process_line(section, str, object_strings)
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
    object_strings := Object_Strings{}
    directive, err := process_line(nil, "anything", &object_strings)
    testing.expect(t, directive == false)
    _, ok := err.(Missing_Section_Declaration)
    testing.expect(t, ok)

}

@(test)
test_empty_line :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    directive, err := process_line(&section, "", &object_strings)
    testing.expect(t, directive == false)
    testing.expect(t, err == nil)
    directive, err = process_line(&section, "    ", &object_strings)
    testing.expect(t, directive == false)
    testing.expect(t, err == nil)
    directive, err = process_line(&section, "; some comment", &object_strings)
    testing.expect(t, directive == false)
    testing.expect(t, err == nil)
    directive, err = process_line(&section, "    ; some comment", &object_strings)
    testing.expect(t, directive == false)
    testing.expect(t, err == nil)
}

@(test)
test_local_label_non_label_character :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    string_table := make([dynamic]u8, 1, 64, context.temp_allocator) // index 0 is empty string
    object_strings := Object_Strings{ string_table = &string_table, string_map = make(map[string]u32, context.temp_allocator) }
    defer free_all(context.temp_allocator)

    testing.expect(t, produces_unexpected_token_error(&section, "0", &object_strings))
}

@(test)
test_local_label_missing_colon :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    string_table := make([dynamic]u8, 1, 64, context.temp_allocator) // index 0 is empty string
    object_strings := Object_Strings{ string_table = &string_table, string_map = make(map[string]u32, context.temp_allocator) }
    defer free_all(context.temp_allocator)

    testing.expect(t, produces_unexpected_token_error(&section, "L1", &object_strings))
}

@(test)
test_local_label_unexpected_token :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    string_table := make([dynamic]u8, 1, 64, context.temp_allocator) // index 0 is empty string
    object_strings := Object_Strings{ string_table = &string_table, string_map = make(map[string]u32, context.temp_allocator) }
    defer free_all(context.temp_allocator)

    testing.expect(t, produces_unexpected_token_error(&section, "L1:!", &object_strings))
}

@(test)
test_local_label :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    string_table := make([dynamic]u8, 1, 64, context.temp_allocator) // index 0 is empty string
    object_strings := Object_Strings{ string_table = &string_table, string_map = make(map[string]u32, context.temp_allocator) }
    defer free_all(context.temp_allocator)

    directive, err := process_line(&section, "L1:", &object_strings)
    testing.expect(t, directive == false)
    testing.expect(t, err == nil)

    // section.buffer
    testing.expect_value(t, len(section.buffer), 0)
    // section.symbol_table
    testing.expect_value(t, len(section.symbol_table), 1)
    testing.expect_value(t, section.symbol_table[0], Symbol_Table_Entry{ offset = 0, name_index = 1 })
    // section.relocation_table
    testing.expect_value(t, len(section.relocation_table), 0)
    // section.symbol_map
    index, ok := section.symbol_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 0)
    // object_strings.string_map
    index, ok = object_strings.string_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 1)
    // object_strings.string_table
    testing.expect_value(t, string(object_strings.string_table[:]), "\x00\x02L1")
}

@(test)
test_local_label_redefinition :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    string_table := make([dynamic]u8, 1, 64, context.temp_allocator) // index 0 is empty string
    object_strings := Object_Strings{ string_table = &string_table, string_map = make(map[string]u32, context.temp_allocator) }
    defer free_all(context.temp_allocator)

    err: Line_Error
    _, err = process_line(&section, "L1:", &object_strings)
    _, err = process_line(&section, "L1:", &object_strings)
    _, ok := err.(Redefinition)
    testing.expect(t, ok)
}

@(test)
test_invalid_mnemonic :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    testing.expect(t, produces_unexpected_token_error(&section, "    bad", &object_strings))
}

@(test)
test_instruction_extraneous_token :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    testing.expect(t, produces_unexpected_token_error(&Code_Section{}, "    b label!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&Code_Section{}, "    nop!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&Code_Section{}, "    add r1, r2, r3 lsl r4!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&Code_Section{}, "    add r1, r2, r3 lsl 4!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&Code_Section{}, "    lsl r1, r2, r3!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&Code_Section{}, "    b r1!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&Code_Section{}, "    mov r1, r2!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&Code_Section{}, "    smv r1!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&Code_Section{}, "    m32 r1, 0!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&Code_Section{}, "    swi 0xAA!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&Code_Section{}, "    mvi r1, 0!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&Code_Section{}, "    scl r1!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&Code_Section{}, "    sst r1!", &object_strings))
}

@(test)
test_instruction_alignment :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    _, err := process_line(&section, "    byte 0x11, 0x22", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    nop", &object_strings)
    expected_buffer_words := []u32le{ 0x0000_2211, 0x4001_0000 }
    testing.expect(t, bytes.compare(section.buffer[:], mem.slice_to_bytes(expected_buffer_words)) == 0)
}

@(test)
test_general_instruction :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    directive, err := process_line(&section, "    mvi r1, 0xAA", &object_strings)
    testing.expect(t, directive == false)
    testing.expect(t, err == nil)

    // section.buffer
    expected_buffer_word: u32le = 0xC100_00AA
    testing.expect(t, bytes.compare(section.buffer[:], mem.ptr_to_bytes(&expected_buffer_word)) == 0)
    // section.symbol_table
    testing.expect_value(t, len(section.symbol_table), 0)
    // section.relocation_table
    testing.expect_value(t, len(section.relocation_table), 0)
    // section.symbol_map
    testing.expect_value(t, len(section.symbol_map), 0)
}

@(test)
test_m32_integer_literal :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    _, err := process_line(&section, "    m32 r1, 0xDEAD_BEEF", &object_strings)
    testing.expect(t, err == nil)

    // section.buffer
    expected_buffer_words := []u32le{ 0xC1AD_BEEF, 0x7111_60DE }
    testing.expect(t, bytes.compare(section.buffer[:], mem.slice_to_bytes(expected_buffer_words)) == 0)
    // section.symbol_table
    testing.expect_value(t, len(section.symbol_table), 0)
    // section.relocation_table
    testing.expect_value(t, len(section.relocation_table), 0)
    // section.symbol_map
    testing.expect_value(t, len(section.symbol_map), 0)
}

@(test)
test_m32_relocation :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    string_table := make([dynamic]u8, 1, 64, context.temp_allocator) // index 0 is empty string
    object_strings := Object_Strings{ string_table = &string_table, string_map = make(map[string]u32, context.temp_allocator) }
    defer free_all(context.temp_allocator)

    _, err := process_line(&section, "    m32 r1, L1", &object_strings)
    testing.expect(t, err == nil)

    // section.buffer
    expected_buffer_words := []u32le{ 0xC100_0000, 0x7111_6000 }
    testing.expect(t, bytes.compare(section.buffer[:], mem.slice_to_bytes(expected_buffer_words)) == 0)
    // section.symbol_table
    testing.expect_value(t, len(section.symbol_table), 1)
    testing.expect_value(t, section.symbol_table[0], Symbol_Table_Entry{ offset = max(u32), name_index = 1 })
    // section.relocation_table
    testing.expect_value(t, len(section.relocation_table), 1)
    testing.expect_value(t, section.relocation_table[0], Relocation_Table_Entry{ offset = 0, symbol_index = 0 })
    // section.symbol_map
    index, ok := section.symbol_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 0)
    // object_strings.string_map
    index, ok = object_strings.string_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 1)
    // object_strings.string_table
    testing.expect_value(t, string(object_strings.string_table[:]), "\x00\x02L1")
}

@(test)
test_branch_relocation :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    string_table := make([dynamic]u8, 1, 64, context.temp_allocator) // index 0 is empty string
    object_strings := Object_Strings{ string_table = &string_table, string_map = make(map[string]u32, context.temp_allocator) }
    defer free_all(context.temp_allocator)

    _, err := process_line(&section, "    beq L1", &object_strings)
    testing.expect(t, err == nil)

    // section.buffer
    expected_buffer_word: u32le = 0x9000_0000
    testing.expect(t, bytes.compare(section.buffer[:], mem.ptr_to_bytes(&expected_buffer_word)) == 0)
    // section.symbol_table
    testing.expect_value(t, len(section.symbol_table), 1)
    testing.expect_value(t, section.symbol_table[0], Symbol_Table_Entry{ offset = max(u32), name_index = 1 })
    // section.relocation_table
    testing.expect_value(t, len(section.relocation_table), 1)
    // section.symbol_map
    index, ok := section.symbol_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 0)
    // object_strings.string_map
    index, ok = object_strings.string_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 1)
    // object_strings.string_table
    testing.expect_value(t, string(object_strings.string_table[:]), "\x00\x02L1")
}

@(test)
test_addr_relocation :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    string_table := make([dynamic]u8, 1, 64, context.temp_allocator) // index 0 is empty string
    object_strings := Object_Strings{ string_table = &string_table, string_map = make(map[string]u32, context.temp_allocator) }
    defer free_all(context.temp_allocator)

    _, err := process_line(&section, "    addr L1", &object_strings)
    testing.expect(t, err == nil)

    // section.buffer
    expected_buffer_word: u32le = 0x0000_0000
    testing.expect(t, bytes.compare(section.buffer[:], mem.ptr_to_bytes(&expected_buffer_word)) == 0)
    // section.symbol_table
    testing.expect_value(t, len(section.symbol_table), 1)
    testing.expect_value(t, section.symbol_table[0], Symbol_Table_Entry{ offset = max(u32), name_index = 1 })
    // section.relocation_table
    testing.expect_value(t, len(section.relocation_table), 1)
    // section.symbol_map
    index, ok := section.symbol_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 0)
    // object_strings.string_map
    index, ok = object_strings.string_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 1)
    // object_strings.string_table
    testing.expect_value(t, string(object_strings.string_table[:]), "\x00\x02L1")
}

@(test)
test_addr_alignment :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    string_table := make([dynamic]u8, 1, 64, context.temp_allocator) // index 0 is empty string
    object_strings := Object_Strings{ string_table = &string_table, string_map = make(map[string]u32, context.temp_allocator) }
    defer free_all(context.temp_allocator)

    _, err := process_line(&section, "    byte 0x11, 0x22", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    addr L1", &object_strings)
    expected_buffer_words := []u32le{ 0x0000_2211, 0x0000_0000 }
    testing.expect(t, bytes.compare(section.buffer[:], mem.slice_to_bytes(expected_buffer_words)) == 0)
}

@(test)
test_multiple_labels_and_relocations :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    string_table := make([dynamic]u8, 1, 64, context.temp_allocator) // index 0 is empty string
    object_strings := Object_Strings{ string_table = &string_table, string_map = make(map[string]u32, context.temp_allocator) }
    defer free_all(context.temp_allocator)

    err: Line_Error
    _, err = process_line(&section, "L1:", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    m32 r1, L2", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "L2:", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    beq L1", &object_strings)
    testing.expect(t, err == nil)

    // section.buffer
    expected_buffer_words := []u32le{ 0xC100_0000, 0x7111_6000, 0x9000_0000 }
    testing.expect(t, bytes.compare(section.buffer[:], mem.slice_to_bytes(expected_buffer_words)) == 0)
    // section.symbol_table
    testing.expect_value(t, len(section.symbol_table), 2)
    testing.expect_value(t, section.symbol_table[0], Symbol_Table_Entry{ offset = 0, name_index = 1 })
    testing.expect_value(t, section.symbol_table[1], Symbol_Table_Entry{ offset = 8, name_index = 4 })
    // section.relocation_table
    testing.expect_value(t, len(section.relocation_table), 2)
    testing.expect_value(t, section.relocation_table[0], Relocation_Table_Entry{ offset = 0, symbol_index = 1 })
    testing.expect_value(t, section.relocation_table[1], Relocation_Table_Entry{ offset = 8, symbol_index = 0 })
    // section.symbol_map
    index, ok := section.symbol_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 0)
    index, ok = section.symbol_map["L2"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 1)
    // object_strings.string_map
    index, ok = object_strings.string_map["L1"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 1)
    index, ok = object_strings.string_map["L2"]
    testing.expect(t, ok)
    testing.expect_value(t, index, 4)
    // object_strings.string_table
    testing.expect_value(t, string(object_strings.string_table[:]), "\x00\x02L1\x02L2")
}

@(test)
test_static_data_out_of_range :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    err: Line_Error
    ok: bool

    _, err = process_line(&section, "    word 0x1_0000_0000", &object_strings)
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    _, err = process_line(&section, "    half 0x1_0000", &object_strings)
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    _, err = process_line(&section, "    byte 0x100", &object_strings)
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)

    _, err = process_line(&section, "    word -0x8000_0001", &object_strings)
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    _, err = process_line(&section, "    half -0x8001", &object_strings)
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    _, err = process_line(&section, "    byte -0x81", &object_strings)
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
}

@(test)
test_static_data_unexpected_token :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    testing.expect(t, produces_unexpected_token_error(&section, "    word!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&section, "    word 0,!", &object_strings))
}

@(test)
test_static_data_single_value :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    err: Line_Error
    _, err = process_line(&section, "    word 0xDEAD_BEEF", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    word -1", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    half 0xBEEF", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    half -1", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    byte 0xAA", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    byte -1", &object_strings)
    testing.expect(t, err == nil)

    // section.buffer
    expected_buffer_bytes := []u8{ 0xEF, 0xBE, 0xAD, 0xDE, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xBE, 0xFF, 0xFF, 0xAA, 0xFF }
    testing.expect(t, bytes.compare(section.buffer[:], expected_buffer_bytes) == 0)
    // section.symbol_table
    testing.expect_value(t, len(section.symbol_table), 0)
    // section.relocation_table
    testing.expect_value(t, len(section.relocation_table), 0)
    // section.symbol_map
    testing.expect_value(t, len(section.symbol_map), 0)
}

@(test)
test_static_data_multiple_values :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    err: Line_Error
    _, err = process_line(&section, "    word 0, 1, 2, 3", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    half 0, 1, 2, 3", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    byte 0, 1, 2, 3", &object_strings)
    testing.expect(t, err == nil)

    expected_buffer_words := []u32le{ 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(section.buffer[:4*SIZE_OF_WORD], mem.slice_to_bytes(expected_buffer_words)) == 0)
    expected_buffer_halfs := []u16le{ 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(section.buffer[4*SIZE_OF_WORD:][:4*SIZE_OF_HALF], mem.slice_to_bytes(expected_buffer_halfs)) == 0)
    expected_buffer_bytes := []u8{ 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(section.buffer[4*SIZE_OF_WORD:][4*SIZE_OF_HALF:][:4*SIZE_OF_BYTE], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_static_data_auto_length_unexpected_token :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    testing.expect(t, produces_unexpected_token_error(&section, "    word *!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&section, "    word * word!", &object_strings))
    testing.expect(t, produces_unexpected_token_error(&section, "    word * word *", &object_strings))
}

@(test)
test_static_data_multiple_values_auto_length :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    err: Line_Error
    _, err = process_line(&section, "    word * word 0, 1, 2, 3", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    half * half 0, 1, 2, 3", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    byte * byte 0, 1, 2, 3", &object_strings)
    testing.expect(t, err == nil)

    expected_buffer_words := []u32le{ 4, 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(section.buffer[:5*SIZE_OF_WORD], mem.slice_to_bytes(expected_buffer_words)) == 0)
    expected_buffer_halfs := []u16le{ 4, 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(section.buffer[5*SIZE_OF_WORD:][:5*SIZE_OF_HALF], mem.slice_to_bytes(expected_buffer_halfs)) == 0)
    expected_buffer_bytes := []u8{ 4, 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(section.buffer[5*SIZE_OF_WORD:][5*SIZE_OF_HALF:][:5*SIZE_OF_BYTE], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_static_data_ascii_unexpected_token :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    _, err := process_line(&section, "    ascii!", &object_strings)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_static_data_ascii_unexpected_eol :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    err: Line_Error
    ok: bool

    _, err = process_line(&section, `    ascii "`, &object_strings)
    _, ok = err.(Unexpected_EOL)
    testing.expect(t, ok)
    _, err = process_line(&section, `    ascii "\"`, &object_strings)
    _, ok = err.(Unexpected_EOL)
    testing.expect(t, ok)
}

@(test)
test_static_data_ascii_unknown_escape_sequence :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    err: Line_Error
    ok: bool

    _, err = process_line(&section, `    ascii "\0"`, &object_strings)
    _, ok = err.(Unknown_Escape_Sequence)
    testing.expect(t, ok)
    _, err = process_line(&section, `    ascii "\x"`, &object_strings)
    _, ok = err.(Unknown_Escape_Sequence)
    testing.expect(t, ok)
    _, err = process_line(&section, `    ascii "\$"`, &object_strings)
    _, ok = err.(Unknown_Escape_Sequence)
    testing.expect(t, ok)
}

@(test)
test_static_data_ascii :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    _, err := process_line(&section, `    ascii "\tabc\n"`, &object_strings)
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ '\t', 'a', 'b', 'c', '\n' }
    testing.expect(t, bytes.compare(section.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_static_data_ascii_auto_length :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    _, err := process_line(&section, `    byte * ascii "ascii"`, &object_strings)
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ 5, 'a', 's', 'c', 'i', 'i' }
    testing.expect(t, bytes.compare(section.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_static_data_ascii_auto_length_escape_characters :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    _, err := process_line(&section, `    byte * ascii "\tabc\n"`, &object_strings)
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ 5, '\t', 'a', 'b', 'c', '\n' }
    testing.expect(t, bytes.compare(section.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_align_non_power_of_two :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    err: Line_Error
    ok: bool

    _, err = process_line(&section, "    align 3", &object_strings)
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    _, err = process_line(&section, "    align 5", &object_strings)
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
}

@(test)
test_align :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    object_strings := Object_Strings{}

    err: Line_Error
    _, err = process_line(&section, "    byte 0xAA", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "    align 4", &object_strings)
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ 0xAA, 0, 0, 0 }
    testing.expect(t, bytes.compare(section.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)

    _, err = process_line(&section, "    align 8", &object_strings)
    testing.expect(t, err == nil)

    expected_buffer_bytes = []u8{ 0xAA, 0, 0, 0, 0, 0, 0, 0 }
    testing.expect(t, bytes.compare(section.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_label_alignment :: proc(t: ^testing.T) {
    section := code_section_init(Section_Type.TEXT)
    defer code_section_cleanup(&section)
    string_table := make([dynamic]u8, 1, 64, context.temp_allocator) // index 0 is empty string
    object_strings := Object_Strings{ string_table = &string_table, string_map = make(map[string]u32, context.temp_allocator) }
    defer free_all(context.temp_allocator)

    err: Line_Error
    _, err = process_line(&section, "    byte 0xAA", &object_strings)
    testing.expect(t, err == nil)
    _, err = process_line(&section, "L1:", &object_strings)
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ 0xAA, 0, 0, 0 }
    testing.expect(t, bytes.compare(section.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)

    testing.expect_value(t, len(section.symbol_table), 1)
    testing.expect_value(t, section.symbol_table[0], Symbol_Table_Entry{ offset = 4, name_index = 1 })
}
