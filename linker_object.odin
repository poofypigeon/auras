package auras

import "base:runtime"
import "core:fmt"
import "core:os"
import "core:path/filepath"
import "core:strings"

BSS_Section :: struct {
    name_index: u32, // index into string table
    size: u32,
}

Linker_Object :: struct {
    bss_sections: [dynamic]BSS_Section,
    text_sections: [dynamic]Text_Data_Section,
    data_sections: [dynamic]Text_Data_Section,
    string_table: [dynamic]u8,
}

Object_Strings :: struct {
    string_table: ^[dynamic]u8,
    string_map: map[string]u32,
}

linker_object_init :: proc() -> (object: ^Linker_Object, object_strings: Object_Strings) {
    object = new(Linker_Object)
    object.bss_sections = make([dynamic]BSS_Section, 0)
    object.text_sections = make([dynamic]Text_Data_Section, 0)
    object.data_sections = make([dynamic]Text_Data_Section, 0)
    object.string_table = make([dynamic]u8, 1, 64) // index 0 is empty string
    object_strings = Object_Strings{
        string_table = &object.string_table,
        string_map = make(map[string]u32, context.temp_allocator),
    }
    object_strings.string_map[""] = 0;
    return
}

linker_object_cleanup :: proc(object: ^Linker_Object) {
    delete(object.bss_sections)
    for &section in object.text_sections {
        text_data_section_cleanup(&section)
    }
    delete(object.text_sections)

    for &section in object.data_sections {
        text_data_section_cleanup(&section)
    }
    delete(object.data_sections)
    delete(object.string_table)
    free(object)
}

process_file :: proc(file_path: string) -> (object: ^Linker_Object, ok: bool) {
    handle, e := os.open(file_path)
    if e != nil {
        os.print_error(os.stderr, e, "error")
        os.exit(1)
    }

    text, success := os.read_entire_file_from_handle(handle)
    if !success {
        fmt.eprintln("failed to read file")
        os.exit(1)
    }

    _ = os.close(handle)

    return process_text(string(text), file_path)
}

process_text :: proc(text: string, file_path: string = "") -> (object: ^Linker_Object, ok: bool) {
    text := text
    directory := filepath.dir(file_path, allocator = context.temp_allocator)

    object_strings: Object_Strings = ---
    object, object_strings = linker_object_init()
    defines := make(map[string]uint, context.temp_allocator)
    defer free_all(context.temp_allocator)
    active_section: ^Text_Data_Section = nil

    line_number: uint = 0
    for line in strings.split_lines_iterator(&text) {
        directive, err := process_line(active_section, line, &object_strings)
        assert(err == nil || !directive, "directive with error")
        if directive {
            err = process_directive(object, line, directory, &defines, &active_section, &object_strings)
        }
        if err != nil {
            print_line_error(file_path, line_number, err, line)
            return nil, false
        }
        line_number += 1
    }

    return object, true
}

process_directive :: proc(
    object: ^Linker_Object,
    line: string,
    directory_path: string,
    defines: ^map[string]uint,
    active_section: ^^Text_Data_Section,
    object_strings: ^Object_Strings,
) -> (err: Line_Error) {
    assert(active_section != nil, "nil double pointer to active section")
    token: string = ---
    eol: bool = ---

    line := Tokenizer{ line = line }

    token, eol = tokenizer_next(&line) or_return
    assert(!eol && token == ".", "expected directive line")

    token, eol = tokenizer_next(&line) or_return
    if eol {
        return Unexpected_EOL{ column = line.token_start }
    }

    switch {
    case token == "export":
        symbol := expect_symbol(&line) or_return
        string_index := get_or_add_string_entry(object_strings, symbol)
        object_strings.string_table[string_index] |= 0x80 // set export bit
    case token == "text":
        symbol := expect_symbol(&line, allow_eol = true) or_return
        append(&object.text_sections, text_data_section_init())
        active_section^ = &object.text_sections[len(object.text_sections)-1]
        active_section^.name_index = get_or_add_string_entry(object_strings, symbol)
    case token == "data":
        symbol := expect_symbol(&line, allow_eol = true) or_return
        append(&object.data_sections, text_data_section_init())
        active_section^ = &object.data_sections[len(object.data_sections)-1]
        active_section^.name_index = get_or_add_string_entry(object_strings, symbol)
    case token == "bss":
        symbol := expect_symbol(&line) or_return
        size := expect_integer(&line) or_return
        bss_section := BSS_Section{
            name_index = get_or_add_string_entry(object_strings, symbol),
            size = u32(size)
        }
        append(&object.bss_sections, bss_section)
        active_section^ = nil
    // case token == "include" && file_path != "":
    // case token == "def":
    case:
        return Unexpected_Token{
            column = line.token_start,
            expected = "directive",
            found = token_str(token)
        }
    }

    token, eol = tokenizer_next(&line) or_return
    if !eol {
        return Unexpected_Token{
            column = line.token_start,
            expected = "'eol'", found = token_str(token)
        }
    }

    return nil
}
