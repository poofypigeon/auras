#+private

package auras

import "core:bytes"
import "core:mem"
import "core:testing"

@(private = "file")
produces_unexpected_token_error :: #force_inline proc(file: ^Source_File, str: string) -> bool {
    err := process_lin(file, str)
    e, ok := err.(Unexpected_Token)
    if ok {
        #partial switch expected in e.expected {
        case [dynamic]string: delete(expected)
        }
    }
    return ok
}

@(test)
test_empty_line :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    testing.expect(t, process_lin(&file, "")                   == nil)
    testing.expect(t, process_lin(&file, "    ")               == nil)
    testing.expect(t, process_lin(&file, "; some comment")     == nil)
    testing.expect(t, process_lin(&file, "    ; some comment") == nil)
}

@(test)
test_local_label_non_label_character :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    testing.expect(t, produces_unexpected_token_error(&file, "0"))
}

@(test)
test_local_label_missing_colon :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    testing.expect(t, produces_unexpected_token_error(&file, "L1"))
}

@(test)
test_local_label_unexpected_token :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    testing.expect(t, produces_unexpected_token_error(&file, "L1:!"))
}

@(test)
test_local_label :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    err := process_lin(&file, "L1:")
    testing.expect(t, err == nil)

    // file.buffer
    testing.expect_value(t, len(file.buffer), 0)
    // file.symbol_table
    testing.expect_value(t, len(file.symbol_table), 1)
    testing.expect_value(t, file.symbol_table[0], Symbol_Table_Entry{ offset = 0, name = 0 })
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
    file := create_source_file()
    defer cleanup_source_file(&file)

    err: Line_Error
    err = process_lin(&file, "L1:")
    err = process_lin(&file, "L1:")
    _, ok := err.(Redefinition)
    testing.expect(t, ok)
}

@(test)
test_invalid_mnemonic :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    testing.expect(t, produces_unexpected_token_error(&file, "    bad"))
}

@(test)
test_instruction_extraneous_token :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    testing.expect(t, produces_unexpected_token_error(&Source_File{}, "    b label!"))
    testing.expect(t, produces_unexpected_token_error(&Source_File{}, "    nop!"))
    testing.expect(t, produces_unexpected_token_error(&Source_File{}, "    add r1, r2, r3 lsl r4!"))
    testing.expect(t, produces_unexpected_token_error(&Source_File{}, "    add r1, r2, r3 lsl 4!"))
    testing.expect(t, produces_unexpected_token_error(&Source_File{}, "    lsl r1, r2, r3!"))
    testing.expect(t, produces_unexpected_token_error(&Source_File{}, "    b r1!"))
    testing.expect(t, produces_unexpected_token_error(&Source_File{}, "    mov r1, r2!"))
    testing.expect(t, produces_unexpected_token_error(&Source_File{}, "    smv r1!"))
    testing.expect(t, produces_unexpected_token_error(&Source_File{}, "    m32 r1, 0!"))
    testing.expect(t, produces_unexpected_token_error(&Source_File{}, "    swi 0xAA!"))
    testing.expect(t, produces_unexpected_token_error(&Source_File{}, "    mvi r1, 0!"))
    testing.expect(t, produces_unexpected_token_error(&Source_File{}, "    scl r1!"))
    testing.expect(t, produces_unexpected_token_error(&Source_File{}, "    sst r1!"))
}

@(test)
test_general_instruction :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    err := process_lin(&file, "    mvi r1, 0xAA")
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
    file := create_source_file()
    defer cleanup_source_file(&file)

    err := process_lin(&file, "    m32 r1, 0xDEAD_BEEF")
    testing.expect(t, err == nil)

    // file.buffer
    expected_buffer_words := []u32le{ 0xC1AD_BEEF, 0x7101_60DE }
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
    file := create_source_file()
    defer cleanup_source_file(&file)

    err := process_lin(&file, "    m32 r1, L1")
    testing.expect(t, err == nil)

    // file.buffer
    expected_buffer_words := []u32le{ 0xC100_0000, 0x7101_6000 }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_words)) == 0)
    // file.symbol_table
    testing.expect_value(t, len(file.symbol_table), 1)
    testing.expect_value(t, file.symbol_table[0], Symbol_Table_Entry{ offset = max(u32), name = 0 })
    // file.relocation_table
    testing.expect_value(t, len(file.relocation_table), 1)
    testing.expect_value(t, file.relocation_table[0], Relocation_Table_Entry{ offset = 0, symbol = 0 })
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
    file := create_source_file()
    defer cleanup_source_file(&file)

    err := process_lin(&file, "    beq L1")
    testing.expect(t, err == nil)

    // file.buffer
    expected_buffer_word: u32le = 0x9000_0000
    testing.expect(t, bytes.compare(file.buffer[:], mem.ptr_to_bytes(&expected_buffer_word)) == 0)
    // file.symbol_table
    testing.expect_value(t, len(file.symbol_table), 1)
    testing.expect_value(t, file.symbol_table[0], Symbol_Table_Entry{ offset = max(u32), name = 0 })
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
test_multiple_labels_and_relocations :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    err: Line_Error
    err = process_lin(&file, "L1:")
    testing.expect(t, err == nil)
    err = process_lin(&file, "    m32 r1, L2")
    testing.expect(t, err == nil)
    err = process_lin(&file, "L2:")
    testing.expect(t, err == nil)
    err = process_lin(&file, "    beq L1")
    testing.expect(t, err == nil)

    // file.buffer
    expected_buffer_words := []u32le{ 0xC100_0000, 0x7101_6000, 0x9000_0000 }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_words)) == 0)
    // file.symbol_table
    testing.expect_value(t, len(file.symbol_table), 2)
    testing.expect_value(t, file.symbol_table[0], Symbol_Table_Entry{ offset = 0, name = 0 })
    testing.expect_value(t, file.symbol_table[1], Symbol_Table_Entry{ offset = 8, name = 3 })
    // file.relocation_table
    testing.expect_value(t, len(file.relocation_table), 2)
    testing.expect_value(t, file.relocation_table[0], Relocation_Table_Entry{ offset = 0, symbol = 1 })
    testing.expect_value(t, file.relocation_table[1], Relocation_Table_Entry{ offset = 8, symbol = 0 })
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
    file := create_source_file()
    defer cleanup_source_file(&file)

    err: Line_Error
    ok: bool

    err = process_lin(&file, "    word 0x1_0000_0000")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    err = process_lin(&file, "    half 0x1_0000")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    err = process_lin(&file, "    byte 0x100")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)

    err = process_lin(&file, "    word -0x8000_0001")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    err = process_lin(&file, "    half -0x8001")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    err = process_lin(&file, "    byte -0x81")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
}

@(test)
test_static_data_unexpected_token :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    testing.expect(t, produces_unexpected_token_error(&file, "    word!"))
    testing.expect(t, produces_unexpected_token_error(&file, "    word 0,!"))
}

@(test)
test_static_data_single_value :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    err: Line_Error
    err = process_lin(&file, "    word 0xDEAD_BEEF")
    testing.expect(t, err == nil)
    err = process_lin(&file, "    word -1")
    testing.expect(t, err == nil)
    err = process_lin(&file, "    half 0xBEEF")
    testing.expect(t, err == nil)
    err = process_lin(&file, "    half -1")
    testing.expect(t, err == nil)
    err = process_lin(&file, "    byte 0xAA")
    testing.expect(t, err == nil)
    err = process_lin(&file, "    byte -1")
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
    file := create_source_file()
    defer cleanup_source_file(&file)

    err: Line_Error
    err = process_lin(&file, "    word 0, 1, 2, 3")
    testing.expect(t, err == nil)
    err = process_lin(&file, "    half 0, 1, 2, 3")
    testing.expect(t, err == nil)
    err = process_lin(&file, "    byte 0, 1, 2, 3")
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
    file := create_source_file()
    defer cleanup_source_file(&file)

    testing.expect(t, produces_unexpected_token_error(&file, "    word *!"))
    testing.expect(t, produces_unexpected_token_error(&file, "    word * word!"))
    testing.expect(t, produces_unexpected_token_error(&file, "    word * word *"))
}

@(test)
test_static_data_multiple_values_auto_length :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    err: Line_Error
    err = process_lin(&file, "    word * word 0, 1, 2, 3")
    testing.expect(t, err == nil)
    err = process_lin(&file, "    half * half 0, 1, 2, 3")
    testing.expect(t, err == nil)
    err = process_lin(&file, "    byte * byte 0, 1, 2, 3")
    testing.expect(t, err == nil)
   
    expected_buffer_words := []u32le{ 4, 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(file.buffer[:5*SIZE_OF_WORD], mem.slice_to_bytes(expected_buffer_words)) == 0)
    expected_buffer_halfs := []u16le{ 4, 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(file.buffer[5*SIZE_OF_WORD:][:5*SIZE_OF_HALF], mem.slice_to_bytes(expected_buffer_halfs)) == 0)
    expected_buffer_bytes := []u8{ 4, 0, 1, 2, 3 }
    testing.expect(t, bytes.compare(file.buffer[5*SIZE_OF_WORD:][5*SIZE_OF_HALF:][:5*SIZE_OF_BYTE], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_static_data_ascii_unexpected_eol :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    err: Line_Error
    ok: bool

    err = process_lin(&file, "    ascii \"")
    _, ok = err.(Unexpected_EOL)
    testing.expect(t, ok)
    err = process_lin(&file, "    ascii \"\\")
    _, ok = err.(Unexpected_EOL)
    testing.expect(t, ok)
}

@(test)
test_static_data_ascii :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    err := process_lin(&file, "    ascii \"\tabc\n\"")
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ '\t', 'a', 'b', 'c', '\n' }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_static_data_ascii_auto_length :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    err := process_lin(&file, "    byte * ascii \"ascii\"")
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ 5, 'a', 's', 'c', 'i', 'i' }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_align_non_power_of_two :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    err: Line_Error
    ok: bool

    err = process_lin(&file, "    align 3")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
    err = process_lin(&file, "    align 5")
    _, ok = err.(Not_Encodable)
    testing.expect(t, ok)
}

@(test)
test_align :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    err: Line_Error
    err = process_lin(&file, "    byte 0xAA")
    testing.expect(t, err == nil)
    err = process_lin(&file, "    align 4")
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ 0xAA, 0, 0, 0 }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)

    err = process_lin(&file, "    align 8")
    testing.expect(t, err == nil)

    expected_buffer_bytes = []u8{ 0xAA, 0, 0, 0, 0, 0, 0, 0 }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)
}

@(test)
test_label_alignment :: proc(t: ^testing.T) {
    file := create_source_file()
    defer cleanup_source_file(&file)

    err: Line_Error
    err = process_lin(&file, "    byte 0xAA")
    testing.expect(t, err == nil)
    err = process_lin(&file, "L1:")
    testing.expect(t, err == nil)

    expected_buffer_bytes := []u8{ 0xAA, 0, 0, 0 }
    testing.expect(t, bytes.compare(file.buffer[:], mem.slice_to_bytes(expected_buffer_bytes)) == 0)

    testing.expect_value(t, len(file.symbol_table), 1)
    testing.expect_value(t, file.symbol_table[0], Symbol_Table_Entry{ offset = 4, name = 0 })
}
