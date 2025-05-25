#+private

package auras

import "core:testing"

@(private = "file")
produces_unexpected_eol_error :: proc(str: string) -> bool {
    line := Tokenizer{ line = str }
    _, eol, err := tokenizer_next(&line)
    if eol {
        return false
    }
    _, ok := err.(Unexpected_EOL)
    return ok
}

@(private = "file")
produces_unknown_escape_sequence_error :: proc(str: string) -> bool {
    _, err := parse_operand(str)
    _, ok := err.(Unknown_Escape_Sequence)
    return ok
}


// --- Tokenizer character literals


@(test)
test_tokenizer_character_literal_unexpected_eol :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_eol_error("' foo"))
    testing.expect(t, produces_unexpected_eol_error("'\\' foo"))
}

@(test)
test_tokenizer_1_byte_character_literal :: proc(t: ^testing.T) {
    tokenizer := Tokenizer{ line = "'a' foo" }
    token, eol, err := tokenizer_next(&tokenizer)
    testing.expect(t, err == nil)
    testing.expect(t, eol == false)
    testing.expect_value(t, token, "'a'")
}

@(test)
test_tokenizer_multi_byte_character_literal :: proc(t: ^testing.T) {
    tokenizer := Tokenizer{ line = "'a boy named goo' foo" }
    token, eol, err := tokenizer_next(&tokenizer)
    testing.expect(t, err == nil)
    testing.expect(t, eol == false)
    testing.expect_value(t, token, "'a boy named goo'")
}

@(test)
test_tokenizer_backslash_character_literal :: proc(t: ^testing.T) {
    tokenizer := Tokenizer{ line = "'\\\\' foo" }
    token, eol, err := tokenizer_next(&tokenizer)
    testing.expect(t, err == nil)
    testing.expect(t, eol == false)
    testing.expect_value(t, token, "'\\\\'")
}

@(test)
test_tokenizer_single_quote_character_literal :: proc(t: ^testing.T) {
    tokenizer := Tokenizer{ line = "'\\'' foo" }
    token, eol, err := tokenizer_next(&tokenizer)
    testing.expect(t, err == nil)
    testing.expect(t, eol == false)
    testing.expect_value(t, token, "'\\''")
}


// parse_operand character literals


// @(test)
test_tokenizer_character_literal_unknown_escape_sequence :: proc(t: ^testing.T) {
    testing.expect(t, produces_unknown_escape_sequence_error("'\\a'"))
    testing.expect(t, produces_unknown_escape_sequence_error("'\\!'"))
    testing.expect(t, produces_unknown_escape_sequence_error("'\\X'"))
}

@(test)
test_parse_operand_1_byte_character_literal :: proc(t: ^testing.T) {
    op, err := parse_operand("'a'")
    testing.expect(t, err == nil)
    v, ok := op.(uint)
    testing.expect(t, ok)
    testing.expect_value(t, v, 'a')
}

@(test)
test_parse_operand_multi_byte_character_literal :: proc(t: ^testing.T) {
    expected: uint = 'a'|(' '<<8)|('b'<<(8*2))|('o'<<(8*3))|('y'<<(8*4))|(' '<<(8*5))|('n'<<(8*6))|('a'<<(8*7))
    op, err := parse_operand("'a boy named goo'")
    testing.expect(t, err == nil)
    v, ok := op.(uint)
    testing.expect(t, ok)
    testing.expect_value(t, v, expected)
}

@(test)
test_parse_operand_backslash_character_literal :: proc(t: ^testing.T) {
    op, err := parse_operand("'\\\\'")
    testing.expect(t, err == nil)
    v, ok := op.(uint)
    testing.expect(t, ok)
    testing.expect_value(t, v, '\\')
}

@(test)
test_parse_operand_single_quote_character_literal :: proc(t: ^testing.T) {
    op, err := parse_operand("'\\''")
    testing.expect(t, err == nil)
    v, ok := op.(uint)
    testing.expect(t, ok)
    testing.expect_value(t, v, '\'')
}

@(test)
test_parse_operand_newline_character_literal :: proc(t: ^testing.T) {
    op, err := parse_operand("'\\n'")
    testing.expect(t, err == nil)
    v, ok := op.(uint)
    testing.expect(t, ok)
    testing.expect_value(t, v, '\n')
}

@(test)
test_parse_operand_tab_character_literal :: proc(t: ^testing.T) {
    op, err := parse_operand("'\\t'")
    testing.expect(t, err == nil)
    v, ok := op.(uint)
    testing.expect(t, ok)
    testing.expect_value(t, v, '\t')
}
