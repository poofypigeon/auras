package auras

import "base:runtime"

import "core:fmt"
import "core:mem"
import "core:strconv"
import "core:strings"
import "core:unicode"

import "core:c"
foreign import "gperf/perfect_hash.a"

Mnemonic :: enum c.int32_t {
    invalid,
    // Data arrays
    word, half, byte, ascii, align,
    // Instructions
    ld,   ldb,  ldh,  ldsb, ldsh,
    st,   stb,  sth,  stsb, stsh,
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

MAX_MNEMONIC_LEN :: 5

SIZE_OF_WORD :: 4
SIZE_OF_HALF :: 2
SIZE_OF_BYTE :: 1

foreign perfect_hash {
    parse_mnemonic :: proc(str: cstring, len: c.size_t) -> Mnemonic ---
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

Register :: enum {
    R0  = 0b0000,
    R1  = 0b0001,
    R2  = 0b0010,
    R3  = 0b0011,
    R4  = 0b0100,
    R5  = 0b0101,
    R6  = 0b0110,
    R7  = 0b0111,
    R8  = 0b1000,
    R9  = 0b1001,
    R10 = 0b1010,
    R11 = 0b1011,
    R12 = 0b1100,
    R13 = 0b1101,
    SP  = 0b1110,
    LR  = 0b1111
}

Symbol :: distinct string

Line_Error :: union {
    Unexpected_EOL,
    Unexpected_Token,
    Not_Encodable,
    Redefinition,
}

quoted_string :: distinct string

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
    message: string
}

Redefinition :: struct {
    label: string
}

expect_register :: proc(line: ^Tokenizer) -> (reg: Register, err: Line_Error) {
    token: string = ---
    ok: bool = ---
    
    if token, ok = tokenizer_next(line); !ok {
        return Register(0), Unexpected_Token{
            column = line.token_start,
            expected = "register", found = quoted_string(token),
        }
    }

    op: Operand = ---
    if op, ok = parse_operand(token); !ok {
        return Register(0), Unexpected_Token{
            column = line.token_start,
            expected = "register", found = quoted_string(token),
        }
    }

    if reg, ok = op.(Register); !ok {
        return Register(0), Unexpected_Token{
            column = line.token_start,
            expected = "register", found = operand_str(op),
        }
    }
    
    return reg, nil
}

expect_integer :: proc(line: ^Tokenizer) -> (val: uint, err: Line_Error) {
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

expect_token :: proc(line: ^Tokenizer, one_of: ..string) -> (operator: string, err: Line_Error) {
    token: string = ---
    ok: bool = ---

    if token, ok = tokenizer_next(line); ok {
        for c in one_of {
            if c == token {
                return token, nil
            }
        }
    }

    // Token not in list
    expected := make([dynamic]string, 0, len(one_of))
    append(&expected, ..one_of)

    return token, Unexpected_Token{
        column = line.token_start,
        expected = expected, found = quoted_string(token),
    }
}

optional_token :: proc(line: ^Tokenizer, one_of: ..string, eol: bool = false) -> (operator: string, ok: bool) {
    token: string = ---

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
         return Register.SP, true
    case token == "lr":
         return Register.LR, true
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

print_line_error :: proc(line: string, err: Line_Error) {
    line := long_string_trail_off(line)
    defer delete(line)

    fmt.print("\x1B[31merror: \x1B[0m", flush = false)

    switch e in err {
    case Unexpected_EOL:
        fmt.print("unexpected 'eol'", flush = false)
        fmt.println(line, flush = false)
        for i in 0..<e.column {
            fmt.print(" ", sep = "", flush = false)
        }
        fmt.println("\x1B[32m^\x1B[0m")
    case Unexpected_Token:
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
        fmt.println(line, flush = false)
        for i in 0..<e.column {
            fmt.print(" ", sep = "", flush = false)
        }
        fmt.println("\x1B[32m^\x1B[0m")
    case Not_Encodable:
        fmt.println(e.message, flush = false)
        fmt.println(line, flush = false)
        for i in 0..<e.start_column {
            fmt.print(" ", sep = "", flush = false)
        }
        fmt.print("\x1B[32m^", flush = false)
        for i in e.start_column+1..<e.end_column {
            fmt.print("~", sep = "", flush = false)
        }
        fmt.println("\x1B[0m")
    case Redefinition:
        fmt.printfln("redefinition of '%s'", e.label, flush = false)
    }

    long_string_trail_off :: #force_inline proc(str: string) -> string {
        if len(str) > 64 {
            return strings.concatenate([]string{ str[:64], "\x1B[90m...\x1B[0m" })
        }
        return strings.clone(str)
    }
}
