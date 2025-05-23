package auras

import "base:runtime"
import "core:fmt"
import "core:os"
import "core:path/filepath"
import "core:strings"

BSS_Section :: struct {
    size: u32,
    label: string,
}

Linker_Object :: struct {
    text_sections: [dynamic]Text_Data_Section,
    data_sections: [dynamic]Text_Data_Section,
    bss_sections: [dynamic]BSS_Section,
    exported_symbols: [dynamic]string,
}

linker_object_cleanup :: proc(object: ^Linker_Object) {
    for &section in object.text_sections {
        text_data_section_cleanup(&section)
    }
    delete(object.text_sections)

    for &section in object.data_sections {
        text_data_section_cleanup(&section)
    }
    delete(object.data_sections)

    for &section in object.bss_sections {
        delete(section.label)
    }
    delete(object.bss_sections)

    for &symbol in object.exported_symbols {
        delete(symbol)
    }
    delete(object.exported_symbols)
}

process_file :: proc(file_path: string) -> (object: Linker_Object, ok: bool) {
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

process_text :: proc(text: string, file_path: string = "") -> (object: Linker_Object, ok: bool) {
    text := text
    directory := filepath.dir(file_path, allocator = context.temp_allocator)

    object = Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defines := make(map[string]uint, context.temp_allocator)
    defer free_all(context.temp_allocator)

    line_number: uint = 0
    for line in strings.split_lines_iterator(&text) {
        directive, err := process_line(active_section, line)
        assert(!(err != nil && directive), "directive with error")
        if directive {
            err = process_directive(&object, line, directory, &defines, &active_section)
        }
        if err != nil {
            print_line_error(file_path, line_number, err, line)
            return Linker_Object{}, false
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
) -> (err: Line_Error) {
    assert(active_section != nil, "nil double pointer to active section")
    token: string = ---
    eol: bool = ---

    line := Tokenizer{ line = line }

    token, eol = tokenizer_next(&line) or_return
    assert(!eol && token == ".", "expected directive line")

    token, eol = tokenizer_next(&line) or_return
    if eol {
        return Unexpected_EOL{ column = line.token_start, }
    }

    switch {
    case token == "export":
        identifier := expect_identifier(&line) or_return
        append(&object.exported_symbols, strings.clone(identifier))
    case token == "text":
        identifier := expect_identifier(&line, allow_eol = true) or_return
        append(&object.text_sections, text_data_section_init())
        active_section^ = &object.text_sections[len(object.text_sections)-1]
        append(&(active_section^).string_table, identifier)
        append(&(active_section^).string_table, 0)
    case token == "data":
        identifier := expect_identifier(&line, allow_eol = true) or_return
        append(&object.data_sections, text_data_section_init())
        active_section^ = &object.data_sections[len(object.data_sections)-1]
        append(&(active_section^).string_table, identifier)
        append(&(active_section^).string_table, 0)
    case token == "bss":
        identifier := expect_identifier(&line) or_return
        size := expect_integer(&line) or_return
        bss_section := BSS_Section{ size = u32(size), label = strings.clone(identifier) }
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
