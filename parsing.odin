#+private

package auras

import "base:runtime"

import "core:fmt"
import "core:mem"
import "core:strconv"
import "core:strings"
import "core:unicode"

import "core:c"
foreign import "gperf/perfect_hash.a"

MAX_MNEMONIC_LEN :: 5

SIZE_OF_WORD :: 4
SIZE_OF_HALF :: 2
SIZE_OF_BYTE :: 1


Mnemonic :: enum c.int32_t {
    invalid,
    // Data arrays
    word, half, byte, ascii, align,
    // Instructions
    ld,   ldb,  ldh,  ldsb, ldsh,
    st,   stb,  sth,
    smv,  scl,  sst,  
    add,  adc,  sub,  sbc,  and,  or,  xor,  btc, 
    addk, adck, subk, sbck, andk, ork, xork, btck,
    nop, 
    tst,  teq,  cmp,  cpn, 
    lsl,  lsr,  asr,  lslk,
    mov,  not,  notk, 
    b,    beq,  bne,  bcs,  bcc,  bmi,  bpl,  bvs,  bvc,  bhi,  bls,  bge,  blt,  bgt,  ble, 
    bl,   bleq, blne, blcs, blcc, blmi, blpl, blvs, blvc, blhi, blls, blge, bllt, blgt, blle,
    mvi,
    swi, 
    m32, 
}

foreign perfect_hash {
    parse_mnemonic :: proc "c" (str: cstring, len: c.size_t) -> Mnemonic ---
}

mnem_from_token :: proc(token: string) -> Mnemonic {
    if len(token) > MAX_MNEMONIC_LEN {
        return .invalid
    }

    buffer: [MAX_MNEMONIC_LEN+1]u8 = ---
    mem.copy_non_overlapping(&buffer, raw_data(token), len(token))
    buffer[len(token)] = 0
    return parse_mnemonic(cstring(raw_data(buffer[:])), len(token))
}

Operand :: union {
    Register,
    uint,
    Symbol,
    string,
}

Register :: distinct u32

Symbol :: distinct string

Line_Error :: union {
    Unexpected_EOL,
    Unexpected_Token,
    Not_Encodable,
    Redefinition,
}

quoted_string :: distinct string

Unexpected_EOL :: struct {
    line: uint,
    column: uint,
}

Unexpected_Token :: struct {
    line: uint,
    column: uint,
    expected: union { string, [dynamic]string },
    found: union { string, quoted_string},
}

Not_Encodable :: struct {
    line: uint,
    start_column: uint,
    end_column: uint,
    message: string,
}

Redefinition :: struct {
    line: uint,
    label: string,
}

expect_register :: proc(line: ^Tokenizer) -> (register: u32, err: Line_Error) {
    token: string = ---
    ok: bool = ---
    
    if token, ok = tokenizer_next(line); !ok {
        return 0, Unexpected_Token{
            column = line.token_start,
            expected = "register", found = quoted_string(token),
        }
    }

    op: Operand = ---
    if op, ok = parse_operand(token); !ok {
        return 0, Unexpected_Token{
            column = line.token_start,
            expected = "register", found = quoted_string(token),
        }
    }

    reg: Register = ---
    if reg, ok = op.(Register); !ok {
        return 0, Unexpected_Token{
            column = line.token_start,
            expected = "register", found = operand_str(op),
        }
    }
    
    return u32(reg), nil
}

expect_integer :: proc(line: ^Tokenizer) -> (value: uint, err: Line_Error) {
    token: string = ---
    ok: bool = ---

    if token, ok = tokenizer_next(line); !ok {
        return 0, Unexpected_Token{
            column = line.token_start,
            expected = "integer literal", found = quoted_string(token)
        }
    }

    op: Operand = ---
    if op, ok = parse_operand(token); !ok {
        return 0, Unexpected_Token{
            column = line.token_start,
            expected = "integer literal", found = quoted_string(token)
        }
    }

    imm: uint = ---
    if imm, ok = op.(uint); !ok {
        return 0, Unexpected_Token{
            column = line.token_start,
            expected = "integer literal", found = operand_str(op)
        }
    }

    return uint(imm), nil
}

expect_register_or_integer :: proc(line: ^Tokenizer) -> (op: Operand, err: Line_Error) {
    token: string = ---
    ok: bool = ---

    if token, ok = tokenizer_next(line); !ok {
        return nil, Unexpected_Token{
            column = line.token_start,
            expected = "register or integer literal", found = quoted_string(token)
        }
    }

    if op, ok = parse_operand(token); !ok {
        return nil, Unexpected_Token{
            column = line.token_start,
            expected = "register or integer literal", found = quoted_string(token)
        }
    }

    #partial switch v in op {
    case string, Symbol:
        return nil, Unexpected_Token{
            column = line.token_start,
            expected = "register or integer literal", found = operand_str(op) }
    }

    return op, nil
}

expect_token :: proc(line: ^Tokenizer, one_of: ..string, no_alloc: bool = false) -> (token: string, err: Line_Error) {
    ok: bool = ---

    if token, ok = tokenizer_next(line); ok {
        for c in one_of {
            if c == token {
                return token, nil
            }
        }
    }

    // Token not in list
    if no_alloc {
        return token, Unexpected_Token{
            column = line.token_start,
            found = quoted_string(token),
        }
    }

    expected := make([dynamic]string, 0, len(one_of))
    append(&expected, ..one_of)

    return token, Unexpected_Token{
        column = line.token_start,
        expected = expected, found = quoted_string(token),
    }
}

optional_token :: proc(line: ^Tokenizer, one_of: ..string, eol: bool = false) -> (token: string, ok: bool) {
    if token, ok = tokenizer_next(line); !ok {
        if eol {
            return "\n", true
        }
        return token, false
    }

    for c in one_of {
        if c == token {
            return token, true
        }
    }

    // Token not in list
    tokenizer_put_back(line);
    return token, false
}

parse_operand :: proc(token: string) -> (op: Operand, ok: bool) {
    assert(len(token) > 0)

    switch {
    case token[0] == 'r':
        reg := strconv.parse_uint(token[1:]) or_return
        (reg <= 0b1111) or_return
        return Register(reg), true
    case token == "sp":
         return Register(14), true
    case token == "lr":
         return Register(15), true
    case unicode.is_number(rune(token[0])):
        imm := strconv.parse_uint(token) or_return
        return imm, true
    case unicode.is_alpha(rune(token[0])), token[0] == '_':
        return Symbol(token), true
    }

    return token, false
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

token_str :: #force_inline proc(token: string) -> union { string, quoted_string } {
    op, _ := parse_operand(token)
    if symbol, ok := op.(Symbol); ok {
        #partial switch mnem_from_token(string(symbol)) {
        case .invalid, .word, .half, .byte, .ascii, .align:
            return quoted_string(op.(Symbol))
        case:
            return string("mnemonic")
        }
    }
    return operand_str(op)
}

// TODO can I make this less of a mess?
print_line_error :: proc(line_text: string, line_number: uint, err: Line_Error) {
    line_text := long_string_trail_off(line_text)
    defer delete(line_text)

    switch e in err {
    case Unexpected_EOL:
        fmt.printf("%d:%d: ", line_number, e.column, flush = false)
        fmt.print("\x1B[31merror: \x1B[0m", flush = false)
        fmt.println("unexpected 'eol'", flush = false)
        fmt.println(line_text, flush = false)
        for i in 0..<e.column {
            fmt.print(" ", sep = "", flush = false)
        }
        fmt.println("\x1B[32m^\x1B[0m")
    case Unexpected_Token:
        fmt.printf("%d:%d: ", line_number, e.column, flush = false)
        fmt.print("\x1B[31merror: \x1B[0m", flush = false)
        fmt.print("expected ", flush = false)
        switch expected in e.expected {
        case string:
            fmt.print(expected, flush = false)
        case [dynamic]string:
            switch len(expected) {
            case 0: panic("Unexpected_Token.expected dynamic array is empty")
            case 1: fmt.printf("'%s'", expected[0], flush = false)
            case 2: fmt.printf("'%s' or '%s'", expected[0], expected[1], flush = false)
            case:
                for token, i in expected {
                    if i == len(expected)-1 {
                        fmt.printf("or '%s'", token, flush = false)
                        continue
                    }
                    fmt.printf("'%s', ", token, flush = false)
                }
            }
            delete(expected)
        }
        fmt.print(", found ", flush = true)
        switch found in e.found {
            case string: fmt.println(found, flush = false)
            case quoted_string: fmt.printfln("'%s'", found, flush = false)
        }
        fmt.println(line_text, flush = false)
        for i in 0..<e.column {
            fmt.print(" ", sep = "", flush = false)
        }
        fmt.println("\x1B[32m^\x1B[0m")
    case Not_Encodable:
        fmt.printf("%d:%d: ", line_number, e.start_column, flush = false)
        fmt.print("\x1B[31merror: \x1B[0m", flush = false)
        fmt.println(e.message, flush = false)
        fmt.println(line_text, flush = false)
        for i in 0..<e.start_column {
            fmt.print(" ", sep = "", flush = false)
        }
        fmt.print("\x1B[32m^", flush = false)
        for i in e.start_column+1..<e.end_column {
            fmt.print("~", sep = "", flush = false)
        }
        fmt.println("\x1B[0m")
    case Redefinition:
        fmt.printf("%d: ", line_number, flush = false)
        fmt.print("\x1B[31merror: \x1B[0m", flush = false)
        fmt.printfln("redefinition of '%s'", e.label, flush = false)
    }

    long_string_trail_off :: #force_inline proc(str: string) -> string {
        if len(str) > 64 {
            return strings.concatenate([]string{ str[:64], "\x1B[90m...\x1B[0m" })
        }
        return strings.clone(str)
    }
}
