#+private

package auras

import "base:runtime"

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
            expected = "integer literal", found = quoted_string(token),
        }
    }

    op: Operand = ---
    if op, ok = parse_operand(token); !ok {
        return 0, Unexpected_Token{
            column = line.token_start,
            expected = "integer literal", found = quoted_string(token),
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

    if token, ok = tokenizer_next(line); !ok {
        return nil, Unexpected_Token{
            column = line.token_start,
            expected = "register or integer literal", found = quoted_string(token),
        }
    }

    if op, ok = parse_operand(token); !ok {
        return nil, Unexpected_Token{
            column = line.token_start,
            expected = "register or integer literal", found = quoted_string(token),
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
