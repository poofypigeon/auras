package auras

import "core:fmt"
import "core:os"
import "core:strings"

BSS_Section :: struct {
    size: u32,
    name: [dynamic]u8,
}

Linker_Object :: struct {
    text_sections: [dynamic]Text_Data_Section,
    data_sections: [dynamic]Text_Data_Section,
    bss_sections: [dynamic]BSS_Section,
}

process_file :: proc(object: ^Linker_Object, active_section: ^Text_Data_Section, file_path: string) -> (ok: bool) {
    handle, err := os.open(file_path)
    if err != nil {
        os.print_error(os.stderr, err, "error")
        os.exit(1)
    }

    text, success := os.read_entire_file_from_handle(handle)
    if !success {
        fmt.eprintln("failed to read file")
        os.exit(1)
    }

    _ = os.close(handle)

    return process_text(object, active_section, string(text))
}

process_text :: proc(object: ^Linker_Object, active_section: ^Text_Data_Section, text: string) -> (ok: bool) {
    text := text

    line_number: uint = 0
    for line in strings.split_lines_iterator(&text) {
        directive, err := process_line(active_section, line)
        if err != nil {
            print_line_error(line, line_number, err)
            return false
        }
        if directive {
            process_directive(object, line)
        }
        line_number += 1
    }

    return true
}

process_directive :: proc(object: ^Linker_Object, line: string) -> (err: Line_Error) {
    token: string = ---
    ok: bool = ---

    line := Tokenizer{ line = line }

    token, ok = tokenizer_next(&line)
    assert(ok && token == ".", "expected directive line")

    if token, ok = tokenizer_next(&line); !ok {
        return Unexpected_Token{
            column = line.token_start,
            expected = "directive",
            found = token_str(token)
        }
    }

    switch {
    case token == "include":
    case token == "export":
    case token == "text":
    case token == "data":
    case token == "bss":
    // case token == "def": TODO
    case:
        return Unexpected_Token{
            column = line.token_start,
            expected = "directive",
            found = token_str(token)
        }
    }

    
}

// code_from_text :: proc(text: string) -> (code: Code_Section, ok: bool) {
//     code = code_section_init()
//     text_ref := text // preserve original text string
//
//     line_number: uint = 1
//     for line_text in strings.split_lines_iterator(&text_ref) {
//         if err := process_line(&code, line_text); err != nil {
//             // code_from_text is going to be replaced so just passing empty file_path for now
//             print_line_error("", line_number, line_text, err)
//             code_section_cleanup(&code)
//             return Code_Section{}, false
//         }
//         line_number += 1
//     }
//
//     for relocation_entry in code.relocation_table {
//         symbol_entry := code.symbol_table[relocation_entry.symbol]
//         if symbol_entry.offset == UNDEFINED_OFFSET {
//             symbol_string := runtime.cstring_to_string(cstring(&code.string_table[symbol_entry.name]))
//             text_ref = text
//             line_number = 1
//             for line_text in strings.split_lines_iterator(&text_ref) {
//                 line := Tokenizer{ line = line_text }
//                 for token in tokenizer_next(&line) {
//                     if token == symbol_string {
//                         err := Undefined_Symbol{ symbol = symbol_string, column = line.token_start }
//                         print_line_error("", line_number, line_text, err)
//                         code_section_cleanup(&code)
//                         return Code_Section{}, false
//                     }
//                 }
//                 line_number += 1
//             }
//             fmt.println("line_number:", line_number)
//             panic("undefined symbol not found in text")
//         }
//
//         highest_nybble := code.buffer[relocation_entry.offset + SIZE_OF_WORD-1] >> 4
//         switch highest_nybble {
//         case 0b1001, 0b1011: // branch
//             b_word := (^u32le)(&code.buffer[relocation_entry.offset])
//             assert(b_word^ & 0x00FF_FFFF == 0, "unexpected instruction at branch relocation offset")
//
//             offset := int(symbol_entry.offset) - int(relocation_entry.offset)
//             assert(offset % SIZE_OF_WORD == 0, "misaligned jump offset")
//             offset >>= 2
//             if (offset >> 23 != 0 && offset >> 23 != 1 && offset >> 23 != -1) {
//                 // TODO trampoline once sections are supported
//                 panic("branch offset out of range")
//             }
//
//             offset_le := u32le(offset)
//             b_word^ |= offset_le & 0x00FF_FFFF
//
//         case 0b1100: // m32
//             mvi_word := (^u32le)(&code.buffer[relocation_entry.offset])
//             assert(mvi_word^ & 0x00FF_FFFF == 0x0000_0000, "unexpected instruction at m32 relocation offset")
//
//             add_word := (^u32le)(&code.buffer[relocation_entry.offset + SIZE_OF_WORD])
//             assert(add_word^ & 0x000F_FFFF == 0x0001_6000, "unexpected instruction at m32 relocation offset + 4")
//
//             offset_le := u32le(symbol_entry.offset)
//             mvi_word^ |= offset_le & 0x00FF_FFFF
//             add_word^ |= offset_le >> 24
//
//         case:
//             panic("unexpected instruction at relocation offset")
//         }
//     }
//
//     return code, true
// }

