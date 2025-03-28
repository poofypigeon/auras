package auras

import "core:strings"
import "core:fmt"

decode_instruction :: proc(machine_word: u32) -> (instr_str: string, ok: bool) {
    switch machine_word >> 30 {
    case 0b00:
        if machine_word >> 15 & 0b11 == 0b11 {
            if ((machine_word >> 29) & 1) == 0b1 {
                if machine_word &~ 0x1F02_03FF != 0x2001_8000 {
                    return "", false
                }
                return decode_set_clear_psr_bits(machine_word)
            }
            if machine_word &~ 0x0F02_0000 != 0x0001_8000 {
                return "", false
            }
            return decode_move_from_psr(machine_word)
        }
        return decode_data_transfer(machine_word)
    case 0b01:
        return decode_data_processing(machine_word)
    case 0b10:
        return decode_branch(machine_word)
    case 0b11:
        if machine_word >> 29 & 0b1 == 0b0 {
            return decode_move_immediate(machine_word)
        }
        return decode_software_interrupt(machine_word)
    }
    panic("unreachable")
}

@(private = "file")
decode_data_transfer :: proc(machine_word: u32) -> (instr_str: string, ok: bool) {
    instr := Data_Transfer_Encoding(machine_word)
    if instr.offset > 15 { return "", false }

    sb := strings.builder_make()

    switch instr.s {
        case false: strings.write_string(&sb, "ld")
        case true:  strings.write_string(&sb, "st")
    }
    if instr.m {
        strings.write_byte(&sb, 's')
    }
    if instr.h {
        assert(!instr.b)
        strings.write_byte(&sb, 'h')
    } else if instr.b {
        strings.write_byte(&sb, 'b')
    }
    strings.write_byte(&sb, ' ')

    strings.write_string(&sb, "r")
    strings.write_uint(&sb, uint(instr.rd))
    strings.write_string(&sb, ", [r");
    strings.write_uint(&sb, uint(instr.rm))
    if instr.p {
        strings.write_byte(&sb, ']')
    }
    switch instr.n {
        case false: strings.write_string(&sb, " + ")
        case true:  strings.write_string(&sb, " - ")
    }
    if instr.i {
        strings.write_uint(&sb, uint(instr.offset << (instr.shift * 2)))
    } else {
        strings.write_byte(&sb, 'r')
        strings.write_uint(&sb, uint(instr.offset))
        if instr.shift > 0 {
            strings.write_string(&sb, " lsl ");
            strings.write_uint(&sb, uint(instr.shift * 2))
        }
    }
    if !instr.p {
        strings.write_byte(&sb, ']')
        if instr.w {
            strings.write_byte(&sb, '!')
        }
    }

    return strings.to_string(sb), true
}

@(private = "file")
decode_move_from_psr :: proc(machine_word: u32) -> (instr_str: string, ok: bool) {
    instr := Move_From_PSR_Encoding(machine_word)

    sb := strings.builder_make()

    strings.write_string(&sb, "smv r")
    strings.write_uint(&sb, uint(instr.rd))

    return strings.to_string(sb), true
}

@(private = "file")
decode_set_clear_psr_bits :: proc(machine_word: u32) -> (instr_str: string, ok: bool) {
    instr := Set_Clear_PSR_Bits_Encoding(machine_word)

    sb := strings.builder_make()

    switch instr.s {
        case false: strings.write_string(&sb, "scl ")
        case true:  strings.write_string(&sb, "sst ")
    }
    if !instr.i {
        strings.write_byte(&sb, 'r')
    }
    strings.write_uint(&sb, uint(instr.operand))

    return strings.to_string(sb), true
}

@(private = "file")
decode_data_processing :: proc(machine_word: u32) -> (instr_str: string, ok: bool) {
    instr := Data_Processing_Encoding(machine_word)
    if !instr.i && instr.operand2 > 15 { return "", false }

    sb := strings.builder_make()

    switch instr.opcode {
        case .add: strings.write_string(&sb, "add")
        case .adc: strings.write_string(&sb, "adc")
        case .sub: strings.write_string(&sb, "sub")
        case .sbc: strings.write_string(&sb, "sbc")
        case .and: strings.write_string(&sb, "and")
        case .or:  strings.write_string(&sb, "or")
        case .xor: strings.write_string(&sb, "xor")
        case .btc: strings.write_string(&sb, "btc")
    }
    if !instr.d && instr.a {
        strings.write_byte(&sb, 'k')
    }
    strings.write_byte(&sb, ' ')

    strings.write_string(&sb, "r")
    strings.write_uint(&sb, uint(instr.rd))
    strings.write_string(&sb, ", r")
    strings.write_uint(&sb, uint(instr.rm))
    strings.write_string(&sb, ", ")
    if !instr.i {
        strings.write_byte(&sb, 'r')
    }
    operand2 := instr.operand2
    if operand2 >> 9 == 1 {
        operand2 |= 0xFFFFFC00
    }
    strings.write_int(&sb, int(i32(operand2)))

    if instr.shift > 0 {
        if (!instr.d && instr.a) || !instr.a {
            strings.write_string(&sb, " ls")
        } else {
            strings.write_string(&sb, " as")
        }
        switch instr.d {
            case false: strings.write_string(&sb, "l ")
            case true:  strings.write_string(&sb, "r ")
        }
        if !instr.h {
            strings.write_byte(&sb, 'r')
        }
        strings.write_uint(&sb, uint(instr.shift))
    }

    return strings.to_string(sb), true
}

@(private = "file")
decode_branch :: proc(machine_word: u32) -> (instr_str: string, ok: bool) {
    instr := Branch_Encoding(machine_word)

    sb := strings.builder_make()

    strings.write_string(&sb, "b")
    if instr.l {
        strings.write_byte(&sb, 'l')
    }
    switch instr.condition {
        case .eq: strings.write_string(&sb, "eq ")
        case .ne: strings.write_string(&sb, "ne ")
        case .cs: strings.write_string(&sb, "cs ")
        case .cc: strings.write_string(&sb, "cc ")
        case .mi: strings.write_string(&sb, "mi ")
        case .pl: strings.write_string(&sb, "pl ")
        case .vs: strings.write_string(&sb, "vs ")
        case .vc: strings.write_string(&sb, "vc ")
        case .hi: strings.write_string(&sb, "hi ")
        case .ls: strings.write_string(&sb, "ls ")
        case .ge: strings.write_string(&sb, "ge ")
        case .lt: strings.write_string(&sb, "lt ")
        case .gt: strings.write_string(&sb, "gt ")
        case .le: strings.write_string(&sb, "le ")
        case .al: strings.write_string(&sb, " ")
        case:
            strings.builder_destroy(&sb)
            return "", false
    }

    if instr.i {
        offset: u32 = u32(instr.offset)
        if offset >> 23 & 0b1 == 0b1 {
            offset |= 0xFF00_0000
        }
        strings.write_int(&sb, int(i32(offset)) << 2)
    } else {
        strings.write_byte(&sb, 'r')
        strings.write_uint(&sb, uint(instr.offset))
    }

    return strings.to_string(sb), true
}

@(private = "file")
decode_move_immediate :: proc(machine_word: u32) -> (instr_str: string, ok: bool) {
    instr := Move_Immediate_Encoding(machine_word)

    sb := strings.builder_make()

    strings.write_string(&sb, "mvi r")
    strings.write_uint(&sb, uint(instr.rd))
    strings.write_string(&sb, ", ")
    immediate_value: u32 = (instr.m) ? 0xFF00_0000 : 0
    immediate_value |= instr.immediate
    strings.write_uint(&sb, uint(immediate_value))

    return strings.to_string(sb), true
}

@(private = "file")
decode_software_interrupt :: proc(machine_word: u32) -> (instr_str: string, ok: bool) {
    instr := Software_Interrupt_Encoding(machine_word)

    sb := strings.builder_make()

    strings.write_string(&sb, "swi ")
    strings.write_uint(&sb, uint(instr.comment))

    return strings.to_string(sb), true
}
