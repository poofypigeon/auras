package auras

import "core:math/bits"

Instruction :: struct {
    machine_word: u32le,
    machine_word2: Maybe(u32le),
    relocation_symbol: Maybe(string),
}


//===----------------------------------------------------------===//
//    Data Transfer Instruction
//===----------------------------------------------------------===//


Data_Transfer_Encoding :: bit_field u32 {
    offset: uint | 10,
    n:      bool | 1,
    shift:  uint | 4,
    b:      bool | 1,
    h:      bool | 1,
    w:      bool | 1,
    m:      bool | 1,
    p:      bool | 1,
    rm:     uint | 4,
    rd:     uint | 4,
    i:      bool | 1,
    s:      bool | 1,
    _:      uint | 2,
}

@(private = "file") DATA_TRANSFER_OFFSET_NOT_ENCODABLE_MESSAGE  :: "value is not encodable as a 10 bit unsigned integer"
@(private = "file") DATA_TRANSFER_SHIFT_MULTIPLE_OF_TWO_MESSAGE :: "shift value must be a multiple of 2"
@(private = "file") DATA_TRANSFER_SHIFT_OUT_OF_RANGE_MESSAGE    :: "shift value must be in the range 0 to 30"

@(private = "file")
encode_data_transfer :: proc(line: ^Tokenizer, flags: Data_Transfer_Encoding) -> (instr: Instruction, err: Line_Error) {
    token: string = ---
    ok: bool = ---

    machine_word := Data_Transfer_Encoding(0x00000000) | flags
   
    machine_word.rd = expect_register(line) or_return

    _ = expect_token(line, ",") or_return
    _ = expect_token(line, "[") or_return

    machine_word.rm = expect_register(line) or_return
  
    token = expect_token(line, "+", "-", "]") or_return
    switch token[0] {
    case '+':
    case '-':
        machine_word.n = true
    case ']':
        if token, ok = optional_token(line, "+", "-", eol = true); !ok {
            return Instruction{}, Unexpected_Token{
                column = line.token_start,
                expected = "'+' or '-'", found = token_str(token)
            }
        }
        switch token[0] {
        case '-':
            machine_word.n = true
            fallthrough
        case '+':
            machine_word.p = true
        case '\n':
            return Instruction{ machine_word = u32le(machine_word) }, nil
        }
    }

    offset: uint
    offset_start_column: uint
    offset_end_column: uint

    op := expect_register_or_integer(line) or_return
    #partial switch v in op {
    case Register:
        machine_word.offset = uint(v)
    case uint:
        machine_word.i = true
        offset = v
    }

    if !machine_word.p { // ']' not already seen
        if _, ok = optional_token(line, "]"); ok {
            if _, ok = optional_token(line, "!"); ok {
                machine_word.w = true;
            }
            if machine_word.i { // Immediate offset
                if machine_word.offset, machine_word.shift, err = encode_offset_and_shift(offset); err != nil {
                    err := err.(Not_Encodable)
                    err.start_column = offset_start_column
                    err.end_column = offset_end_column
                    return Instruction{}, err
                }
            }
            return Instruction{ machine_word = u32le(machine_word) }, nil
        }
    }

    if token, ok = optional_token(line, "lsl"); !ok {
        if !machine_word.p { // ']' not already seen
            return Instruction{}, Unexpected_Token{
                column = line.token_start,
                expected = machine_word.p ? "'lsl'" : "']' or 'lsl'",
                found = token_str(token)
            }
        }
        if machine_word.i { // Immediate offset
            if machine_word.offset, machine_word.shift, err = encode_offset_and_shift(offset); err != nil {
                err := err.(Not_Encodable)
                err.start_column = offset_start_column
                err.end_column = offset_end_column
                return Instruction{}, err
            }
        }
        return Instruction{ machine_word = u32le(machine_word) }, nil
    }

    if machine_word.i {
        if offset >= (1 << 10) {
            return Instruction{}, Not_Encodable{
                start_column = offset_start_column,
                end_column = offset_end_column,
                message = DATA_TRANSFER_OFFSET_NOT_ENCODABLE_MESSAGE
            }
        }
        machine_word.offset = offset
    }

    shift := expect_integer(line) or_return
    if machine_word.shift,  err = encode_shift(shift); err != nil {
        err := err.(Not_Encodable)
        err.start_column = line.token_start
        err.end_column = line.token_end
        return Instruction{}, err
    }

    if machine_word.p { // ']' seen before shift
        return Instruction{ machine_word = u32le(machine_word) }, nil
    }

    _ = expect_token(line, "]") or_return
    if _, ok = optional_token(line, "!"); ok {
        machine_word.w = true
    }

    return Instruction{ machine_word = u32le(machine_word) }, nil

    encode_offset_and_shift :: proc "contextless" (v: uint) -> (offset: uint, shift: uint, err: Line_Error) {
        // Value is encodable without shift
        if v < (1 << 10) {
            return v, 0, nil
        }

        // Find an encoding using a shift, if possible
        uint_bits :: bits.count_ones(max(uint))
        leading_zeros := bits.count_leading_zeros(v)
        trailing_zeros := bits.count_trailing_zeros(v)
        window_size := uint_bits - leading_zeros - trailing_zeros

        // All set bits must fit within in a 10 bit window
        // If the window is not left-aligned to 2 bits, then it may only be 9 bits wide
        if window_size > 10 || (window_size == 10 && leading_zeros % 2 == 1) {
            return 0, 0, Not_Encodable{ message = DATA_TRANSFER_OFFSET_NOT_ENCODABLE_MESSAGE }
        }

        offset = v >> (trailing_zeros &~ 1)
        shift = trailing_zeros >> 1
        return offset, shift, nil
    }

    encode_shift :: proc "contextless" (shift: uint) -> (new: uint, err: Line_Error) {
        if shift > 0x1E {
            return 0, Not_Encodable{ message = DATA_TRANSFER_SHIFT_OUT_OF_RANGE_MESSAGE }
        }
        if shift % 2 == 1 {
            return 0, Not_Encodable{ message = DATA_TRANSFER_SHIFT_MULTIPLE_OF_TWO_MESSAGE }
        }
        return shift >> 1, nil
    }
}


//===----------------------------------------------------------===//
//    Move From PSR Instruction
//===----------------------------------------------------------===//


Move_From_PSR_Encoding :: bit_field u32 {
    _:  uint | 24,
    rd: uint | 4,
    _:  uint | 4,
}

@(private = "file")
encode_move_from_psr :: proc(line: ^Tokenizer) -> (instr: Instruction, err: Line_Error) {
    machine_word := Move_From_PSR_Encoding(0x00018000)

    machine_word.rd = expect_register(line) or_return
    return Instruction{ machine_word = u32le(machine_word) }, nil
}


//===----------------------------------------------------------===//
//    Set/Clear PSR Bits Instruction
//===----------------------------------------------------------===//


Set_Clear_PSR_Bits_Encoding :: bit_field u32 {
    operand: uint | 10,
    _:       uint | 7,
    s:       bool | 1,
    _:       uint | 10,
    i:       bool | 1,
    _:       uint | 3,
}

@(private = "file") SET_CLEAR_PSR_BITS_NOT_ENCODABLE_MESSAGE :: "value is not encodable as a 10 bit unsigned integer"

@(private = "file")
encode_set_clear_psr_bits :: proc(line: ^Tokenizer, flags: Set_Clear_PSR_Bits_Encoding) -> (instr: Instruction, err: Line_Error) {
    token: string = ---
    ok: bool = ---

    machine_word := Set_Clear_PSR_Bits_Encoding(0x20018000) | flags
    
    op := expect_register_or_integer(line) or_return
    #partial switch v in op {
    case Register:
        machine_word.operand = uint(v)
    case uint:
        machine_word.i = true
        if v > 0x3ff {
            return Instruction{}, Not_Encodable{
                start_column = line.token_start,
                end_column = line.token_end,
                message = SET_CLEAR_PSR_BITS_NOT_ENCODABLE_MESSAGE,
            }
        }
        machine_word.operand = v
    }
   
    return Instruction{ machine_word = u32le(machine_word) }, nil
}


//===----------------------------------------------------------===//
//    Data Processing Instruction
//===----------------------------------------------------------===//


Data_Processing_Encoding :: bit_field u32 {
    operand2: uint   | 10,
    shift:    uint   | 5,
    d:        bool   | 1,
    a:        bool   | 1,
    opcode:   Opcode | 3,
    rm:       uint   | 4,
    rd:       uint   | 4,
    i:        bool   | 1,
    h:        bool   | 1,
    _:        uint   | 2,
}

Opcode :: enum {
    add = 0b000,
    adc = 0b001,
    sub = 0b010,
    sbc = 0b011,
    and = 0b100,
    or  = 0b101,
    xor = 0b110,
    btc = 0b111,
}

@(private = "file")
Data_Processing_Variant :: enum {
    Generic = 0,
    No_Operation,
    No_Writeback,
    No_Rm,
    No_Rn,
}

@(private = "file") DATA_PROCESSING_RIGHT_SHIFT_WITH_K_MESSAGE                :: "right shifts cannot be used with 'k' instruction variants"
@(private = "file") DATA_PROCESSING_IMMEDIATE_AND_SHIFT_NOT_ENCODABLE_MESSAGE :: "value is not encodable as a 10 bit signed integer and shift"
@(private = "file") DATA_PROCESSING_IMMEDIATE_NOT_ENCODABLE_MESSAGE           :: "value is not encodable as a 10 bit signed integer"
@(private = "file") DATA_PROCESSING_SHIFT_NOT_ENCODABLE_MESSAGE               :: "shift value is not encodable as a 5 bit unsigned integer"

@(private = "file")
encode_data_processing :: proc(line: ^Tokenizer, flags: Data_Processing_Encoding, variant: Data_Processing_Variant = .Generic) -> (instr: Instruction, err: Line_Error) {
    token: string = ---
    ok: bool = ---

    machine_word := Data_Processing_Encoding(0x40000000) | flags

    if variant == .No_Operation { // nop
        return Instruction{ machine_word = u32le(machine_word) }, nil
    }

    if variant != .No_Writeback { // All except tst, teq, cmp, cpn
        machine_word.rd = expect_register(line) or_return
        _ = expect_token(line, ",") or_return
    }

    immediate: uint
    immediate_start_column: uint
    immediate_end_column: uint

    if variant == .No_Rm { // lsl, lsr, asr, lslx
        rn := expect_register(line) or_return
        machine_word.operand2 = uint(rn)
        _ = expect_token(line, ",") or_return
    } else {
        machine_word.rm = expect_register(line) or_return

        if variant == .No_Rn { // mov, not, notk
            return Instruction{ machine_word = u32le(machine_word) }, nil
        }

        _ = expect_token(line, ",") or_return
    
        _, negated := optional_token(line, "-")
        immediate_start_column = line.token_start 

        op := expect_register_or_integer(line) or_return

        if !negated {
            immediate_start_column = line.token_start 
        }
        immediate_end_column = line.token_end

        #partial switch v in op {
        case Register:
            if negated {
                return Instruction{}, Unexpected_Token{
                    column = line.token_start,
                    expected = "integer literal following '-'", found = string("register")
                }
            }
            machine_word.operand2 = uint(v)
        case uint:
            machine_word.i = true
            immediate = negated ? ~v  + 1 : v
        }

        if token, ok = optional_token(line, "lsl", "lsr", "asr", eol = true); !ok {
            return Instruction{}, Unexpected_Token{
                column = line.token_start,
                expected = "'lsl', 'lsr', or 'asr'", found = token_str(token)
            }
        }

        switch token {
        case "\n": // No explicit shift
            if machine_word.i { // Immediate operand
                if machine_word.operand2, machine_word.shift, err = encode_immediate_and_shift(immediate); err != nil {
                    err := err.(Not_Encodable)
                    err.start_column = immediate_start_column
                    err.end_column = immediate_end_column
                    return Instruction{}, err
                }
            }
            return Instruction{ machine_word = u32le(machine_word) }, nil
        case "lsl":
        case "asr":
            machine_word.a = true
            fallthrough
        case "lsr":
            machine_word.d = true
            if flags.a { // k instruction variant
                return Instruction{}, Not_Encodable{
                    start_column = line.token_start,
                    end_column = line.token_end,
                    message = DATA_PROCESSING_RIGHT_SHIFT_WITH_K_MESSAGE
                }
            }
        }
    }

    if machine_word.i { // Immediate operand
        if immediate >= (1 << 9) && (int(immediate) >> 9) != -1 { // Not encodable without shift
            return Instruction{}, Not_Encodable{
                start_column = immediate_start_column,
                end_column = immediate_end_column,
                message = DATA_PROCESSING_IMMEDIATE_NOT_ENCODABLE_MESSAGE
            }
        }
        machine_word.operand2 = immediate
    }

    shift_operand := expect_register_or_integer(line) or_return
    #partial switch shift in shift_operand {
    case Register:
        machine_word.shift = uint(shift)
    case uint:
        machine_word.h = true
        if machine_word.shift, err = encode_shift(shift, machine_word.d); err != nil {
            err := err.(Not_Encodable)
            err.start_column = line.token_start
            err.end_column = line.token_end
            return Instruction{}, err
        }
    }

    return Instruction{ machine_word = u32le(machine_word) }, nil

    encode_immediate_and_shift :: proc(v: uint) -> (immediate: uint, shift: uint, err: Line_Error) {
        if v < (1 << 9) || (int(v) >> 9) == -1 { // Encodable without shift
            return v, 0, nil
        }

        // Find an encoding using a shift, if possible
        leading_zeros := bits.count_leading_zeros(v)
        trailing_zeros := bits.count_trailing_zeros(v)

        if leading_zeros == 0 { // negative
            leading_ones := bits.count_leading_zeros(~v)
            window_size := leading_ones - trailing_zeros
            if window_size > 9 {
                return 0, 0, Not_Encodable{
                    message = DATA_PROCESSING_IMMEDIATE_AND_SHIFT_NOT_ENCODABLE_MESSAGE
                }
            }
            
            immediate = uint(int(v) >> trailing_zeros)
            shift = trailing_zeros
            return immediate, shift, nil
        }

        // positive
        uint_bits :: bits.count_ones(max(uint))
        window_size := uint_bits - leading_zeros - trailing_zeros
        if window_size > 9 {
            return 0, 0, Not_Encodable{
                message = DATA_PROCESSING_IMMEDIATE_AND_SHIFT_NOT_ENCODABLE_MESSAGE
            }
        }

        immediate = v >> trailing_zeros
        shift = trailing_zeros
        return immediate, shift, nil
    }

    encode_shift :: proc(shift: uint, right: bool) -> (new: uint, err: Line_Error) {
        // Must determine if the shift is representable as a 5-bit value
        if shift > 32 || (!right && shift == 32) {
            return 0, Not_Encodable{
                message = DATA_PROCESSING_SHIFT_NOT_ENCODABLE_MESSAGE
            }
        }

        // 32-bit right shift is encoded as 0
        return (shift < 32 ? shift : 0), nil
    }
}


//===----------------------------------------------------------===//
//    Software Interrupt Instruction
//===----------------------------------------------------------===//


Software_Interrupt_Encoding :: bit_field u32 {
    comment: uint | 28,
    _:       uint | 4,
}

@(private = "file")
SOFTWARE_INTERRUPT_NOT_ENCODABLE_MESSAGE :: "value is not encodable as a 16 bit unsigned integer"

@(private = "file")
encode_software_interrupt :: proc(line: ^Tokenizer) -> (instr: Instruction, err: Line_Error) {
    token: string = ---
    ok: bool = ---

    machine_word := Software_Interrupt_Encoding(0xE0000000)

    if token, ok = tokenizer_next(line); !ok { // end of line
        return Instruction{ machine_word = u32le(machine_word) }, nil
    }

    op: Operand = ---
    if op, ok = parse_operand(token); !ok {
        return Instruction{}, Unexpected_Token{
            column = line.token_start,
            expected = "integer literal", found = quoted_string(token)
        }
    }

    relocation_symbol: Maybe(string) = nil
    #partial switch v in op {
    case uint:
        if v >= (1 << 28) {
            return Instruction{}, Not_Encodable{
                start_column = line.token_start,
                end_column = line.token_end,
                message = SOFTWARE_INTERRUPT_NOT_ENCODABLE_MESSAGE
            }
        }
        machine_word.comment = v
    case Register, Symbol, string:
        return Instruction{}, Unexpected_Token{
            column = line.token_start,
            expected = "integer literal", found = operand_str(token)
        }
    }

    return Instruction{ machine_word = u32le(machine_word) }, nil
}


//===----------------------------------------------------------===//
//    Branch Instruction
//===----------------------------------------------------------===//


Branch_Encoding :: bit_field u32 {
    offset:    uint      | 24,
    condition: Condition | 4,
    i:         bool      | 1,
    l:         bool      | 1,
    _:         uint      | 2,
}

Condition :: enum uint {
    eq = 0b0000,
    ne = 0b0001,
    cs = 0b0010,
    cc = 0b0011,
    mi = 0b0100,
    pl = 0b0101,
    vs = 0b0110,
    vc = 0b0111,
    hi = 0b1000,
    ls = 0b1001,
    ge = 0b1010,
    lt = 0b1011,
    gt = 0b1100,
    le = 0b1101,
    al = 0b1110,
}

@(private = "file")
encode_branch :: proc(line: ^Tokenizer, flags: Branch_Encoding) -> (instr: Instruction, err: Line_Error) {
    token: string = ---
    ok: bool = ---

    machine_word := Branch_Encoding(0x80000000) | flags

    if token, ok = tokenizer_next(line); !ok { // end of line
        return Instruction{}, Unexpected_Token{
            column = line.token_start,
            expected = "symbol", found = quoted_string(token)
        }
    }

    op: Operand = ---
    if op, ok = parse_operand(token); !ok {
        return Instruction{}, Unexpected_Token{
            column = line.token_start,
            expected = "symbol", found = quoted_string(token)
        }
    }

    relocation_symbol: Maybe(string) = nil
    #partial switch v in op {
    case Register:
        machine_word.offset = uint(v)
    case Symbol:
        machine_word.i = true
        relocation_symbol = string(v)
    case uint, string:
        return Instruction{}, Unexpected_Token{
            column = line.token_start,
            expected = "symbol", found = operand_str(token)
        }
    }

    return Instruction{ machine_word = u32le(machine_word), relocation_symbol = relocation_symbol }, nil
}


//===----------------------------------------------------------===//
//    Move Immediate Instruction
//===----------------------------------------------------------===//


Move_Immediate_Encoding :: bit_field u32 {
    immediate: uint | 24,
    rd:        uint | 4,
    m:         bool | 1,
    _:         uint | 3,
}

@(private = "file")
MOVE_IMMEDIATE_NOT_ENCODABLE_MESSAGE :: "value is not encodable as a 25 bit signed integer"

@(private = "file")
encode_move_immediate :: proc(line: ^Tokenizer) -> (instr: Instruction, err: Line_Error) {
    token: string = ---
    ok: bool = ---

    machine_word := Move_Immediate_Encoding(0xC0000000)

    machine_word.rd = expect_register(line) or_return

    _ = expect_token(line, ",") or_return

    _, machine_word.m = optional_token(line, "-")
    immediate_start_column := line.token_start

    imm := expect_integer(line) or_return
    if imm > uint(max(u32)) {
        return Instruction{}, Not_Encodable{
            start_column = line.token_start,
            end_column = line.token_end,
            message = MOVE_IMMEDIATE_NOT_ENCODABLE_MESSAGE,
        }
    }

    if machine_word.m { // negative
        imm = ~imm + 1 // two's complement
        if u32(imm) < 0xFF000000 {
            return Instruction{}, Not_Encodable{
                start_column = immediate_start_column,
                end_column = line.token_end,
                message = MOVE_IMMEDIATE_NOT_ENCODABLE_MESSAGE,
            }
        }
        machine_word.immediate = imm
        return Instruction{ machine_word = u32le(machine_word) }, nil
    }

    immediate_start_column = line.token_start

    top_byte := (imm >> 24) & 0xFF
    if top_byte == 0xFF { // positive value with top byte all set
        machine_word.m = true
    } else if top_byte != 0x00 {
        return Instruction{}, Not_Encodable{
            start_column = immediate_start_column,
            end_column = line.token_end,
            message = MOVE_IMMEDIATE_NOT_ENCODABLE_MESSAGE,
        }
    }
  
    machine_word.immediate = imm
    return Instruction{ machine_word = u32le(machine_word) }, nil
}


//===----------------------------------------------------------===//
//    m32 Pseudo-Instruction
//===----------------------------------------------------------===//


@(private = "file")
M32_NOT_ENCODABLE_MESSAGE :: "value is not encodable in 32 bits"

@(private = "file")
encode_m32 :: proc(line: ^Tokenizer) -> (instr: Instruction, err: Line_Error) {
    token: string = ---
    ok: bool = ---

    machine_word := Move_Immediate_Encoding(0xC0000000)
    machine_word2 := Data_Processing_Encoding(0x40000000) | Data_Processing_Encoding{ h = true, i = true, a = true, shift = 24 }

    rd := expect_register(line) or_return
    machine_word.rd = rd
    machine_word2.rd = rd

    _ = expect_token(line, ",") or_return

    _, negated := optional_token(line, "-")
    immediate_start_column := line.token_start 

    if token, ok = tokenizer_next(line); !ok { // end of line
        return Instruction{}, Unexpected_Token{
            column = line.token_start,
            expected = "integer literal or symbol", found = quoted_string(token)
        }
    }

    if !negated {
        immediate_start_column = line.token_start 
    }
    immediate_end_column := line.token_end

    op: Operand = ---
    if op, ok = parse_operand(token); !ok {
        return Instruction{}, Unexpected_Token{
            column = line.token_start,
            expected = "integer literal or symbol", found = quoted_string(token)
        }
    }

    #partial switch v in op {
    case uint:
        imm := (negated) ? ~v + 1 : v;
        switch {
        // single instruction
        case int(imm) >> 24 == -1:
            machine_word.m = true
            fallthrough
        case imm < (1 << 24):
            machine_word.immediate = imm 
            instr.machine_word = u32le(machine_word)
            return instr, nil
        // double instruction
        case (negated && int(imm) >> 31 == -1) || imm < (1 << 32):
            machine_word.immediate = imm & 0xFFFFFF;
            machine_word2.operand2 = (imm >> 24) & 0xFF;
        case:
            return Instruction{}, Not_Encodable{
                start_column = immediate_start_column,
                end_column = immediate_end_column,
                message = M32_NOT_ENCODABLE_MESSAGE
            }
        }
    case Symbol:
        instr.relocation_symbol = string(v)
    case Register, string:
        return Instruction{}, Unexpected_Token{
            column = line.token_start,
            expected = "integer literal or symbol", found = operand_str(token)
        }
    }

    instr.machine_word = u32le(machine_word)
    instr.machine_word2 = u32le(machine_word2)
    return instr, nil
}


//===----------------------------------------------------------===//
//    Encode Instruction By Mnemonic
//===----------------------------------------------------------===//


encode_instruction :: proc(line: ^Tokenizer, mnem: Mnemonic) -> (instr: Instruction, err: Line_Error) {
    switch mnem {
    // Data Transfer
    case .ld:   instr = encode_data_transfer(line, Data_Transfer_Encoding{                              }) or_return
    case .ldb:  instr = encode_data_transfer(line, Data_Transfer_Encoding{ b = true                     }) or_return
    case .ldh:  instr = encode_data_transfer(line, Data_Transfer_Encoding{ h = true                     }) or_return
    case .ldsb: instr = encode_data_transfer(line, Data_Transfer_Encoding{ b = true, m = true           }) or_return
    case .ldsh: instr = encode_data_transfer(line, Data_Transfer_Encoding{ h = true, m = true           }) or_return
    case .st:   instr = encode_data_transfer(line, Data_Transfer_Encoding{                     s = true }) or_return
    case .stb:  instr = encode_data_transfer(line, Data_Transfer_Encoding{ b = true,           s = true }) or_return
    case .sth:  instr = encode_data_transfer(line, Data_Transfer_Encoding{ h = true,           s = true }) or_return
    case .stsb: instr = encode_data_transfer(line, Data_Transfer_Encoding{ b = true, m = true, s = true }) or_return
    case .stsh: instr = encode_data_transfer(line, Data_Transfer_Encoding{ h = true, m = true, s = true }) or_return
    // Move From PSR
    case .smv:  instr = encode_move_from_psr(line) or_return
    // Set/Clear PSR Bits
    case .scl:  instr = encode_set_clear_psr_bits(line, Set_Clear_PSR_Bits_Encoding{          }) or_return
    case .sst:  instr = encode_set_clear_psr_bits(line, Set_Clear_PSR_Bits_Encoding{ s = true }) or_return
    // Data Processing (Generic)
    case .add:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .add           }) or_return
    case .adc:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .adc           }) or_return
    case .sub:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .sub           }) or_return
    case .sbc:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .sbc           }) or_return
    case .and:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .and           }) or_return
    case .or:   instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .or            }) or_return
    case .xor:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .xor           }) or_return
    case .btc:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .btc           }) or_return
    case .addk: instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .add, a = true }) or_return
    case .adck: instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .adc, a = true }) or_return
    case .subk: instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .sub, a = true }) or_return
    case .sbck: instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .sbc, a = true }) or_return
    case .andk: instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .and, a = true }) or_return
    case .ork:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .or,  a = true }) or_return
    case .xork: instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .xor, a = true }) or_return
    case .btck: instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .btc, a = true }) or_return
    // Data Processing (No_Operation)
    case .nop:  instr = encode_data_processing(line, Data_Processing_Encoding{ a = true }, variant = .No_Operation) or_return
    // Data Processing (No_Writeback)
    case .tst:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .and }, variant = .No_Writeback) or_return
    case .teq:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .xor }, variant = .No_Writeback) or_return
    case .cmp:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .sub }, variant = .No_Writeback) or_return
    case .cpn:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .add }, variant = .No_Writeback) or_return
    // Data Processing (No_Rm)
    case .lsl:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .or                     }, variant = .No_Rm ) or_return
    case .lsr:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .or, d = true           }, variant = .No_Rm ) or_return
    case .asr:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .or, d = true, a = true }, variant = .No_Rm ) or_return
    case .lslk: instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .or,           a = true }, variant = .No_Rm ) or_return
    // Data Processing (No_Rn)
    case .mov:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .add,           a = true                   }, .No_Rn ) or_return
    case .not:  instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .xor, i = true,           operand2 = 0x3FF }, .No_Rn ) or_return
    case .notk: instr = encode_data_processing(line, Data_Processing_Encoding{ opcode = .xor, i = true, a = true, operand2 = 0x3FF }, .No_Rn ) or_return
    // Branch
    case .b:    instr = encode_branch(line, Branch_Encoding{ condition = .al           }) or_return
    case .beq:  instr = encode_branch(line, Branch_Encoding{ condition = .eq           }) or_return
    case .bne:  instr = encode_branch(line, Branch_Encoding{ condition = .ne           }) or_return
    case .bcs:  instr = encode_branch(line, Branch_Encoding{ condition = .cs           }) or_return
    case .bcc:  instr = encode_branch(line, Branch_Encoding{ condition = .cc           }) or_return
    case .bmi:  instr = encode_branch(line, Branch_Encoding{ condition = .mi           }) or_return
    case .bpl:  instr = encode_branch(line, Branch_Encoding{ condition = .pl           }) or_return
    case .bvs:  instr = encode_branch(line, Branch_Encoding{ condition = .vs           }) or_return
    case .bvc:  instr = encode_branch(line, Branch_Encoding{ condition = .vc           }) or_return
    case .bhi:  instr = encode_branch(line, Branch_Encoding{ condition = .hi           }) or_return
    case .bls:  instr = encode_branch(line, Branch_Encoding{ condition = .ls           }) or_return
    case .bge:  instr = encode_branch(line, Branch_Encoding{ condition = .ge           }) or_return
    case .blt:  instr = encode_branch(line, Branch_Encoding{ condition = .lt           }) or_return
    case .bgt:  instr = encode_branch(line, Branch_Encoding{ condition = .gt           }) or_return
    case .ble:  instr = encode_branch(line, Branch_Encoding{ condition = .le           }) or_return
    case .bl:   instr = encode_branch(line, Branch_Encoding{ condition = .al, l = true }) or_return
    case .bleq: instr = encode_branch(line, Branch_Encoding{ condition = .eq, l = true }) or_return
    case .blne: instr = encode_branch(line, Branch_Encoding{ condition = .ne, l = true }) or_return
    case .blcs: instr = encode_branch(line, Branch_Encoding{ condition = .cs, l = true }) or_return
    case .blcc: instr = encode_branch(line, Branch_Encoding{ condition = .cc, l = true }) or_return
    case .blmi: instr = encode_branch(line, Branch_Encoding{ condition = .mi, l = true }) or_return
    case .blpl: instr = encode_branch(line, Branch_Encoding{ condition = .pl, l = true }) or_return
    case .blvs: instr = encode_branch(line, Branch_Encoding{ condition = .vs, l = true }) or_return
    case .blvc: instr = encode_branch(line, Branch_Encoding{ condition = .vc, l = true }) or_return
    case .blhi: instr = encode_branch(line, Branch_Encoding{ condition = .hi, l = true }) or_return
    case .blls: instr = encode_branch(line, Branch_Encoding{ condition = .ls, l = true }) or_return
    case .blge: instr = encode_branch(line, Branch_Encoding{ condition = .ge, l = true }) or_return
    case .bllt: instr = encode_branch(line, Branch_Encoding{ condition = .lt, l = true }) or_return
    case .blgt: instr = encode_branch(line, Branch_Encoding{ condition = .gt, l = true }) or_return
    case .blle: instr = encode_branch(line, Branch_Encoding{ condition = .le, l = true }) or_return
    // Move Immediate
    case .mvi:  instr = encode_move_immediate(line) or_return
    // Software Interrupt
    case .swi:  instr = encode_software_interrupt(line) or_return
    // m32 Pseudo-Instruction
    case .m32:  instr = encode_m32(line) or_return
    case .invalid, .word, .half, .byte, .ascii, .align:
    }

    return instr, nil
}
