#+private

package auras

import "core:fmt"
import "core:mem"
import "core:os"
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
    push, pop,
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

Register :: distinct u32
Symbol :: distinct string

Operand :: union {
    Register,
    uint,
    Symbol,
    string,
}

quoted_string :: distinct string

expect_register :: proc(line: ^Tokenizer) -> (register: u32, err: Line_Error) {
    token: string = ---
    ok: bool = ---

    eol: bool = ---
    token, eol = tokenizer_next(line) or_return
    if eol {
        return 0, Unexpected_Token{
            column = line.token_start,
            expected = "register", found = quoted_string(token),
        }
    }

    op: Operand = ---
    if op, err = parse_operand(token); err != nil {
        #partial switch e in err {
        case Unexpected_Token:
            err := e
            err.column = line.token_start
            err.expected = "register or integer literal"
            err.found = quoted_string(token)
            return 0, err
        case Unknown_Escape_Sequence:
            err := e
            err.column += line.token_start
            return 0, err
        case: unreachable()
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

    eol: bool = ---
    token, eol = tokenizer_next(line) or_return
    if eol {
        return 0, Unexpected_Token{
            column = line.token_start,
            expected = "integer literal", found = quoted_string(token),
        }
    }

    op: Operand = ---
    if op, err = parse_operand(token); err != nil {
        #partial switch e in err {
        case Unexpected_Token:
            err := e
            err.column = line.token_start
            err.expected = "register or integer literal"
            err.found = quoted_string(token)
            return 0, err
        case Unknown_Escape_Sequence:
            err := e
            err.column += line.token_start
            return 0, err
        case: unreachable()
        }
    }

    imm: uint = ---
    if imm, ok = op.(uint); !ok {
        return 0, Unexpected_Token{
            column = line.token_start,
            expected = "integer literal", found = operand_str(op),
        }
    }

    return uint(imm), nil
}

expect_register_or_integer :: proc(line: ^Tokenizer) -> (op: Operand, err: Line_Error) {
    token: string = ---
    ok: bool = ---

    eol: bool = ---
    token, eol = tokenizer_next(line) or_return
    if eol {
        return nil, Unexpected_Token{
            column = line.token_start,
            expected = "register or integer literal", found = quoted_string(token),
        }
    }

    if op, err = parse_operand(token); err != nil {
        #partial switch e in err {
        case Unexpected_Token:
            err := e
            err.column = line.token_start
            err.expected = "register or integer literal"
            err.found = quoted_string(token)
            return nil, err
        case Unknown_Escape_Sequence:
            err := e
            err.column += line.token_start
            return nil, err
        case: unreachable()
        }
    }

    #partial switch v in op {
    case string, Symbol:
        return nil, Unexpected_Token{
            column = line.token_start,
            expected = "register or integer literal", found = operand_str(op),
        }
    }

    return op, nil
}

expect_identifier :: proc(line: ^Tokenizer, allow_eol: bool = false) -> (identifier: string, err: Line_Error) {
    token, eol := tokenizer_next(line) or_return
    if eol {
        if !allow_eol {
            return "", Unexpected_EOL{ column = line.token_start }
        }
        return "", nil
    } else if !is_symbol_char(token[0]) {
        return "", Unexpected_Token{
            column = line.token_start,
            expected = "identifier", found = token_str(token)
        }
    }
    return token, nil
}

expect_token :: proc(line: ^Tokenizer, one_of: ..string, no_alloc: bool = false) -> (token: string, err: Line_Error) {
    ok: bool = ---

    eol: bool = ---
    token, eol = tokenizer_next(line) or_return
    if !eol {
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

optional_token :: proc(line: ^Tokenizer, one_of: ..string, opt_eol: bool = false) -> (token: string, ok: bool, err: Line_Error) {
    eol: bool = ---
    token, eol = tokenizer_next(line) or_return
    if eol {
        if opt_eol {
            return "\n", true, nil
        }
        return token, false, nil
    }

    for c in one_of {
        if c == token {
            return token, true, nil
        }
    }

    // Token not in list
    tokenizer_put_back(line);
    return token, false, nil
}

parse_operand :: proc(token: string) -> (op: Operand, err: Line_Error) {
    assert(len(token) > 0)
    v: uint = 0
    ok: bool = ---
    switch {
    case token[0] == 'r':
        if v, ok = strconv.parse_uint(token[1:]); !ok || (v > 0b1111) {
            return nil, Unexpected_Token{}
        }
        return Register(v), nil
    case token == "sp":
        return Register(14), nil
    case token == "lr":
        return Register(15), nil
    case unicode.is_number(rune(token[0])):
        if v, ok = strconv.parse_uint(token); !ok {
            return nil, Unexpected_Token{}
        }
        return v, nil
    case token[0] == '\'':
        assert(token[len(token)-1] == '\'', "missing single quote")
        byte_offset: uint = 0
        for i := 1; i < len(token)-1; i += 1 {
            ch := token[i]
            if token[i] == '\\' {
                ch = token[i+1]
                switch ch {
                case '\\': ch = '\\'
                case '\'': ch = '\''
                case 'n': ch = '\n'
                case 't': ch = '\t'
                case: return nil, Unknown_Escape_Sequence{ column = uint(i) }
                }
                i += 1
            }
            v |= uint(ch) << (byte_offset * 8)
            byte_offset += 1
        }
        return v, nil
    case is_symbol_char(token[0]):
        return Symbol(token), nil
    }

    return token, nil
}

Tokenizer :: struct {
    line: string,
    token_start: uint,
    token_end: uint,
}

tokenizer_next :: proc(tokenizer: ^Tokenizer) -> (token: string, eol: bool, err: Line_Error) {
    line := tokenizer.line

    // Skip whitespace
    for tokenizer.token_end < len(line) {
        if !unicode.is_space(rune(line[tokenizer.token_end])) do break
        tokenizer.token_end += 1
    }
    tokenizer.token_start = tokenizer.token_end
    if tokenizer.token_end == len(line) {
        return "eol", true, nil
    }

    // Treat comments as end-of-line
    if (line[tokenizer.token_start] == ';') {
        return "eol", true, nil
    }

    tokenizer.token_end += 1;

    // String literals and character literals are consumed as one token
    if tokenizer.line[tokenizer.token_start] == '"' || tokenizer.line[tokenizer.token_start] == '\'' {
        if tokenizer.token_end == len(tokenizer.line) {
            return "", false, Unexpected_EOL{ column = tokenizer.token_end }
        }
        escaped := false
        for {
            if tokenizer.line[tokenizer.token_end] == tokenizer.line[tokenizer.token_start] && !escaped {
                break
            }
            if tokenizer.line[tokenizer.token_end] == '\\' && !escaped {
                escaped = true
            } else {
                escaped = false
            }
            tokenizer.token_end += 1
            if tokenizer.token_end == len(tokenizer.line) {
                return "", false, Unexpected_EOL{ column = tokenizer.token_end }
            }
        }
        tokenizer.token_end += 1
        return line[tokenizer.token_start:tokenizer.token_end], false, nil
    }

    // Any non-alphanumeric characters besides whitespace and underscores are distinct tokens
    if !(is_symbol_char(line[tokenizer.token_start]) || unicode.is_number(rune(line[tokenizer.token_start]))) {
        return line[tokenizer.token_start:tokenizer.token_end], false, nil
    }

    // Consume characters until a non-label character is encountered
    for tokenizer.token_end < len(line) {
        if !(is_symbol_char(line[tokenizer.token_end]) || unicode.is_number(rune(line[tokenizer.token_end]))) do break
        tokenizer.token_end += 1
    }
    return line[tokenizer.token_start:tokenizer.token_end], false, nil
}

tokenizer_put_back :: #force_inline proc(tokenizer: ^Tokenizer) {
    tokenizer.token_end = tokenizer.token_start
}

tokenizer_curr :: #force_inline proc(tokenizer: ^Tokenizer) -> string {
    return tokenizer.line[tokenizer.token_start:tokenizer.token_end]
}

is_symbol_char :: #force_inline proc(c: u8) -> bool {
    return unicode.is_alpha(rune(c)) || c == '_'
}
