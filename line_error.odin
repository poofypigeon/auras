#+private

package auras

import "core:encoding/ansi"
import "core:fmt"
import "core:strings"

Line_Error :: union {
    Unexpected_EOL,
    Unexpected_Token,
    Not_Encodable,
    Redefinition,
    Undefined_Symbol,
    Unknown_Escape_Sequence,
    Missing_Section_Declaration,
}

Unexpected_EOL :: struct {
    column: uint,
}

Unexpected_Token :: struct {
    column: uint,
    expected: union { string, [dynamic]string },
    found: union { string, quoted_string},
}

Not_Encodable :: struct {
    start_column: uint,
    end_column: uint,
    message: string,
}

Redefinition :: struct {
    column: uint,
    label: string,
}

Undefined_Symbol :: struct {
    column: uint,
    symbol: string,
}

Unknown_Escape_Sequence :: struct {
    column: uint,
}

Missing_Section_Declaration :: struct {
    column: uint,
}

print_line_error :: proc(file_path: string, line_number: uint, err: Line_Error, line_text: string) {
    sb := strings.builder_make()
    defer strings.builder_destroy(&sb)

    start_column: uint = ---
    switch e in err {
    case Unexpected_EOL: start_column = e.column
    case Unexpected_Token: start_column = e.column
    case Not_Encodable: start_column = e.start_column
    case Redefinition: start_column = e.column
    case Undefined_Symbol: start_column = e.column
    case Unknown_Escape_Sequence: start_column = e.column
    case Missing_Section_Declaration: start_column = e.column
    }

    fmt.sbprintf(&sb, "%s(%d:%d) " , file_path, line_number, start_column)
    fmt.sbprint(&sb, ansi.CSI+ansi.FG_BRIGHT_RED+ansi.SGR+"error: "+ansi.CSI+ansi.RESET+ansi.SGR)

    switch e in err {
    case Unexpected_EOL:
        fmt.sbprintln(&sb, "unexpected 'eol'")
        underline(&sb, line_text, start_column)
    case Unexpected_Token:
        fmt.sbprint(&sb, "expected ")
        switch expected in e.expected {
        case string:
            fmt.sbprint(&sb, expected)
        case [dynamic]string:
            switch len(expected) {
            case 0: panic("Unexpected_Token.expected dynamic array is empty")
            case 1: fmt.sbprintf(&sb, "'%s'", expected[0])
            case 2: fmt.sbprintf(&sb, "'%s' or '%s'", expected[0], expected[1])
            case:
                for token, i in expected {
                    if i == len(expected)-1 {
                        fmt.sbprintf(&sb, "or '%s'", token)
                        continue
                    }
                    fmt.sbprintf(&sb, "'%s', ", token)
                }
            }
            delete(expected)
        }
        fmt.sbprint(&sb, ", found ")
        switch found in e.found {
            case string: fmt.sbprintln(&sb, found)
            case quoted_string: fmt.sbprintfln(&sb, "'%s'", found)
        }
        underline(&sb, line_text, start_column)
    case Not_Encodable:
        fmt.sbprintln(&sb, e.message)
        underline(&sb, line_text, start_column, e.end_column)
    case Redefinition:
        fmt.sbprintfln(&sb, "redefinition of '%s'", e.label)
    case Undefined_Symbol:
        fmt.sbprintfln(&sb, "'%s' referenced but not defined", e.symbol)
        underline(&sb, line_text, start_column)
    case Unknown_Escape_Sequence:
        fmt.sbprintln(&sb, "unknown escape sequence")
        underline(&sb, line_text, start_column, start_column + 2)
    case Missing_Section_Declaration:
        fmt.sbprintln(&sb, "expected section declaration")
        underline(&sb, line_text, start_column)
    }

    fmt.eprint(strings.to_string(sb))

    underline :: proc(b: ^strings.Builder, line: string, start_column: uint, end_column: uint = 0) {
        fmt.sbprintln(b, line)
        for i in 0..<start_column {
            strings.write_byte(b, ' ')
        }
        fmt.sbprint(b, ansi.CSI+ansi.FG_YELLOW+ansi.SGR+"^")
        for i in start_column+1..<end_column {
            strings.write_byte(b, '~')
        }
        fmt.sbprintln(b, ansi.CSI+ansi.RESET+ansi.SGR)
    }
}

operand_str :: #force_inline proc(op: Operand) -> union { string, quoted_string } {
    switch v in op {
    case Register: return string("register")
    case uint:     return string("integer literal")
    case Symbol:   return quoted_string(v)
    case string:   return quoted_string(v)
    case:          panic("invalid operand")
    }
}

token_str :: proc(token: string) -> union { string, quoted_string } {
    if token[0] == '"' {
        return string("string literal")
    }
    if token[0] == '\'' {
        return string("character literal")
    }
    op, _ := parse_operand(token)
    if symbol, ok := op.(Symbol); ok {
        return quoted_string(op.(Symbol))
    }
    return operand_str(op)
}
