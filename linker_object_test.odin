#+private

package auras

import "core:testing"

import "core:bytes"
import "core:slice"

@(test)
test_linker_object_expected_directive :: proc(t: ^testing.T) {
    object := Linker_Object{}
    object_strings := Object_Strings{}
    active_section: ^Code_Section = nil

    line := ".foo"
    err := process_directive(&object, line, "", nil, &active_section, &object_strings)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}


// .export


@(test)
test_linker_object_export_unexpected_eol :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".export"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    _, ok := err.(Unexpected_EOL)
    testing.expect(t, ok)
}

@(test)
test_linker_object_export_unexpected_token :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".export!"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_export_extra_token :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".export label!"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_export :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".export label"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    testing.expect(t, err == nil)
    testing.expect_value(t, object_strings.string_map["label"], 1)
    testing.expect(t, string(object.string_table[:]) == "\x00\x85label")
}


// .text


@(test)
test_linker_object_text_unexpected_token :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".text!"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_text_extra_token :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".text symbol!"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_text :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".text"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    testing.expect(t, err == nil)
    testing.expect_value(t, len(object.code_sections), 1)
    testing.expect(t, active_section != nil)
    testing.expect(t, active_section == &object.code_sections[0])
    testing.expect_value(t, active_section.type, Section_Type.TEXT)
    testing.expect_value(t, active_section.name_index, 0)
    testing.expect_value(t, string(object.string_table[:]), "\x00")
}

@(test)
test_linker_object_text_with_symbol :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".text symbol"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    testing.expect(t, err == nil)
    testing.expect_value(t, len(object.code_sections), 1)
    testing.expect(t, active_section != nil)
    testing.expect(t, active_section == &object.code_sections[0])
    testing.expect_value(t, active_section.type, Section_Type.TEXT)
    testing.expect_value(t, active_section.name_index, 1)
    testing.expect_value(t, object_strings.string_map["symbol"], 1)
    testing.expect_value(t, string(object.string_table[:]), "\x00\x06symbol")
}


// .data


@(test)
test_linker_object_data_unexpected_token :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".data!"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_data_extra_token :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".data symbol!"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_data :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".data"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    testing.expect(t, err == nil)
    testing.expect_value(t, len(object.code_sections), 1)
    testing.expect(t, active_section != nil)
    testing.expect(t, active_section == &object.code_sections[0])
    testing.expect(t, active_section == &object.code_sections[0])
    testing.expect_value(t, active_section.type, Section_Type.DATA)
    testing.expect_value(t, active_section.name_index, 0)
    testing.expect_value(t, string(object.string_table[:]), "\x00")
}

@(test)
test_linker_object_data_with_symbol :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".data symbol"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    testing.expect(t, err == nil)
    testing.expect_value(t, len(object.code_sections), 1)
    testing.expect(t, active_section != nil)
    testing.expect(t, active_section == &object.code_sections[0])
    testing.expect_value(t, active_section.type, Section_Type.DATA)
    testing.expect_value(t, active_section.name_index, 1)
    testing.expect_value(t, object_strings.string_map["symbol"], 1)
    testing.expect_value(t, string(object.string_table[:]), "\x00\x06symbol")
}


// .rodata


@(test)
test_linker_object_rodata_unexpected_token :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".rodata!"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_rodata_extra_token :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".rodata symbol!"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_rodata :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".rodata"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    testing.expect(t, err == nil)
    testing.expect_value(t, len(object.code_sections), 1)
    testing.expect(t, active_section != nil)
    testing.expect(t, active_section == &object.code_sections[0])
    testing.expect(t, active_section == &object.code_sections[0])
    testing.expect_value(t, active_section.type, Section_Type.RODATA)
    testing.expect_value(t, active_section.name_index, 0)
    testing.expect_value(t, string(object.string_table[:]), "\x00")
}

@(test)
test_linker_object_rodata_with_symbol :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".rodata symbol"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    testing.expect(t, err == nil)
    testing.expect_value(t, len(object.code_sections), 1)
    testing.expect(t, active_section != nil)
    testing.expect(t, active_section == &object.code_sections[0])
    testing.expect_value(t, active_section.type, Section_Type.RODATA)
    testing.expect_value(t, active_section.name_index, 1)
    testing.expect_value(t, object_strings.string_map["symbol"], 1)
    testing.expect_value(t, string(object.string_table[:]), "\x00\x06symbol")
}


// .bss


@(test)
test_linker_object_bss_unexpected_token_pos1 :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".bss!"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_bss_unexpected_token_pos2 :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".bss label!"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_bss_extra_token :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".bss label 256!"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_bss :: proc(t: ^testing.T) {
    object, object_strings := linker_object_init()
    defer linker_object_cleanup(object)
    defer free_all(context.temp_allocator)
    active_section: ^Code_Section = nil

    line := ".bss label 256"
    err := process_directive(object, line, "", nil, &active_section, &object_strings)
    testing.expect(t, err == nil)
    testing.expect(t, len(object.bss_sections) == 1)
    testing.expect_value(t, object.bss_sections[0].name_index, 1)
    testing.expect_value(t, string(object.string_table[:]), "\x00\x05label")
    testing.expect(t, active_section == nil)
}
