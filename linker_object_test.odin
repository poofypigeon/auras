#+private

package auras

import "core:testing"

import "core:bytes"
import "core:slice"

@(test)
test_linker_object_expected_directive :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    line := ".foo"
    err := process_directive(&object, line, "", nil, &active_section)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}


// .export


@(test)
test_linker_object_export_unexpected_eol :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    line := ".export"
    err := process_directive(&object, line, "", nil, &active_section)
    _, ok := err.(Unexpected_EOL)
    testing.expect(t, ok)
}

@(test)
test_linker_object_export_unexpected_token :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    line := ".export!"
    err := process_directive(&object, line, "", nil, &active_section)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_export_extra_token :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".export label!"
    err := process_directive(&object, line, "", nil, &active_section)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_export :: proc(t: ^testing.T) { object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".export label"
    err := process_directive(&object, line, "", nil, &active_section)
    testing.expect(t, err == nil)
    testing.expect_value(t, len(object.exported_symbols), 1)
    testing.expect_value(t, object.exported_symbols[0], "label")
}


// .text


@(test)
test_linker_object_text_unexpected_token :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".text!"
    err := process_directive(&object, line, "", nil, &active_section)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_text_extra_token :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".text identifier!"
    err := process_directive(&object, line, "", nil, &active_section)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_text :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".text"
    err := process_directive(&object, line, "", nil, &active_section)
    testing.expect(t, err == nil)
    testing.expect_value(t, len(object.text_sections), 1)
    testing.expect(t, active_section != nil)
    testing.expect(t, active_section == &object.text_sections[0])
    testing.expect_value(t, string(object.text_sections[0].string_table[:]), "\x00")
}

@(test)
test_linker_object_text_with_identifier :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".text identifier"
    err := process_directive(&object, line, "", nil, &active_section)
    testing.expect(t, err == nil)
    testing.expect_value(t, len(object.text_sections), 1)
    testing.expect(t, active_section != nil)
    testing.expect(t, active_section == &object.text_sections[0])
    testing.expect_value(t, string(object.text_sections[0].string_table[:]), "identifier\x00")
}


// .data


@(test)
test_linker_object_data_unexpected_token :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".data!"
    err := process_directive(&object, line, "", nil, &active_section)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_data_extra_token :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".data identifier!"
    err := process_directive(&object, line, "", nil, &active_section)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_data :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".data"
    err := process_directive(&object, line, "", nil, &active_section)
    testing.expect(t, err == nil)
    testing.expect_value(t, len(object.data_sections), 1)
    testing.expect(t, active_section != nil)
    testing.expect(t, active_section == &object.data_sections[0])
    testing.expect_value(t, string(object.data_sections[0].string_table[:]), "\x00")
}

@(test)
test_linker_object_data_with_identifier :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".data identifier"
    err := process_directive(&object, line, "", nil, &active_section)
    testing.expect(t, err == nil)
    testing.expect_value(t, len(object.data_sections), 1)
    testing.expect(t, active_section != nil)
    testing.expect(t, active_section == &object.data_sections[0])
    testing.expect_value(t, string(object.data_sections[0].string_table[:]), "identifier\x00")
}


// .bss


@(test)
test_linker_object_bss_unexpected_token_pos1 :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".bss!"
    err := process_directive(&object, line, "", nil, &active_section)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_bss_unexpected_token_pos2 :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".bss label!"
    err := process_directive(&object, line, "", nil, &active_section)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_bss_extra_token :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".bss label 256!"
    err := process_directive(&object, line, "", nil, &active_section)
    _, ok := err.(Unexpected_Token)
    testing.expect(t, ok)
}

@(test)
test_linker_object_bss :: proc(t: ^testing.T) {
    object := Linker_Object{}
    active_section: ^Text_Data_Section = nil
    defer linker_object_cleanup(&object)
    line := ".bss label 256"
    err := process_directive(&object, line, "", nil, &active_section)
    testing.expect(t, err == nil)
    testing.expect(t, len(object.bss_sections) == 1)
    testing.expect_value(t, object.bss_sections[0].label, "label")
    testing.expect(t, active_section == nil)
}
