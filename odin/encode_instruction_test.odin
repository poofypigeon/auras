#+private

package auras

import "core:testing"

@(private = "file")
produces_unexpected_token_error :: proc(str: string) -> bool {
    line := Tokenizer{ line = str }
    token, eol, err := tokenizer_next(&line)
    if eol || err != nil {
        return false
    }
    mnem := mnem_from_token(token)
    _, err = encode_instruction_from_mnemonic(&line, mnem)
    if err == nil {
        return false
    }
    e, ok := err.(Unexpected_Token)
    if ok {
        #partial switch expected in e.expected {
        case [dynamic]string: delete(expected)
        }
    }
    return ok
}

@(private = "file")
produces_not_encodable_error :: proc(str: string) -> bool {
    line := Tokenizer{ line = str }
    token, eol, err := tokenizer_next(&line)
    if eol || err != nil {
        return false
    }
    mnem := mnem_from_token(token)
    _, err = encode_instruction_from_mnemonic(&line, mnem)
    if err == nil {
        return false
    }
    _, ok := err.(Not_Encodable)
    return ok
}

@(private = "file")
machine_word :: proc(str: string) -> u32 {
    line := Tokenizer{ line = str }
    token, eol, err := tokenizer_next(&line)
    if eol || err != nil {
        return ~u32(0)
    }
    mnem := mnem_from_token(token)
    instr: Instruction = ---
    instr, err = encode_instruction_from_mnemonic(&line, mnem)
    if err != nil {
        return ~u32(0)
    }
    return instr.machine_word
}

@(private = "file")
instruction :: proc(str: string) -> Instruction {
    line := Tokenizer{ line = str }
    token, eol, err := tokenizer_next(&line)
    if eol || err != nil {
        return Instruction{ machine_word = ~u32(0) }
    }
    mnem := mnem_from_token(token)
    instr: Instruction = ---
    instr, err = encode_instruction_from_mnemonic(&line, mnem)
    if err != nil {
        return Instruction{ machine_word = ~u32(0) }
    }
    return instr
}

@(private = "file")
instruction_and_error :: proc(str: string) -> (instr: Instruction, err: Line_Error) {
    line := Tokenizer{ line = str }
    token: string = ---
    eol: bool = ---
    token, eol, err = tokenizer_next(&line)
    if eol || err != nil {
        return Instruction{ machine_word = ~u32(0) }, nil
    }
    mnem := mnem_from_token(token)
    return encode_instruction_from_mnemonic(&line, mnem)
}


// --- Data Transfer


@(test)
test_data_transfer_unexpected_eol :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("ld"))
    testing.expect(t, produces_unexpected_token_error("ld r1"))
    testing.expect(t, produces_unexpected_token_error("ld r1,"))
    testing.expect(t, produces_unexpected_token_error("ld r1, ["))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2 +"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2 -"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2 + 4"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2 + r3"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2 + r3 lsl"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2 + r3 lsl 4"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2] +"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2] -"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2] + r3 lsl"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2] + 4 lsl"))
}

@(test)
test_data_transfer_unexpected_token :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("ld!"))
    testing.expect(t, produces_unexpected_token_error("ld r1!"))
    testing.expect(t, produces_unexpected_token_error("ld r1,!"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [!"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2!"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2 +!"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2 -!"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2 + 4!"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2 + r3!"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2 + r3 lsl!"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2 + r3 lsl 4!"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2] +!"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2] -!"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2] + r3 lsl!"))
    testing.expect(t, produces_unexpected_token_error("ld r1, [r2]!"))
}

@(test)
test_data_transfer_unencodable_offset :: proc(t: ^testing.T) {
    testing.expect(t, produces_not_encodable_error("ld r1, [r2 + 0b111_1111_1111]"))
    testing.expect(t, produces_not_encodable_error("ld r1, [r2] + 0b111_1111_1111"))
    testing.expect(t, produces_not_encodable_error("ld r1, [r2 + 0b111_1111_1110]"))
    testing.expect(t, produces_not_encodable_error("ld r1, [r2] + 0b111_1111_1110"))
}

@(test)
test_data_transfer_unencodable_shift :: proc(t: ^testing.T) {
    testing.expect(t, produces_not_encodable_error("ld r1, [r2 + r3 lsl 1]"))
    testing.expect(t, produces_not_encodable_error("ld r1, [r2 + r3 lsl 32]"))
}

@(test)
test_data_transfer_mnemonic_variants :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("ld   r1, [r2]"), 0x0120_0000)
    testing.expect_value(t, machine_word("ldb  r1, [r2]"), 0x0120_8000)
    testing.expect_value(t, machine_word("ldsb r1, [r2]"), 0x0124_8000)
    testing.expect_value(t, machine_word("ldh  r1, [r2]"), 0x0121_0000)
    testing.expect_value(t, machine_word("ldsh r1, [r2]"), 0x0125_0000)
    testing.expect_value(t, machine_word("st   r1, [r2]"), 0x2120_0000)
    testing.expect_value(t, machine_word("stb  r1, [r2]"), 0x2120_8000)
    testing.expect_value(t, machine_word("sth  r1, [r2]"), 0x2121_0000)
}

@(test)
test_data_transfer_no_writeback_register_offset :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("ld r1, [r2 + r3]"),       0x0120_0003)
    testing.expect_value(t, machine_word("ld r1, [r2 - r3]"),       0x0120_0403)
    testing.expect_value(t, machine_word("ld r1, [r2 + r3 lsl 4]"), 0x0120_1003)
    testing.expect_value(t, machine_word("ld r1, [r2 - r3 lsl 4]"), 0x0120_1403)
}

@(test)
test_data_transfer_no_writeback_immediate :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("ld r1, [r2 + 0x02AA]"),       0x1120_02AA)
    testing.expect_value(t, machine_word("ld r1, [r2 - 0x02AA]"),       0x1120_06AA)
    testing.expect_value(t, machine_word("ld r1, [r2 + 0x0011 lsl 2]"), 0x1120_0811)
}

@(test)
test_data_transfer_pre_increment_register_offset :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("ld r1, [r2 + r3]!"),       0x0122_0003)
    testing.expect_value(t, machine_word("ld r1, [r2 - r3]!"),       0x0122_0403)
    testing.expect_value(t, machine_word("ld r1, [r2 + r3 lsl 4]!"), 0x0122_1003)
    testing.expect_value(t, machine_word("ld r1, [r2 - r3 lsl 4]!"), 0x0122_1403)
}

@(test)
test_data_transfer_pre_increment_writeback_immediate :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("ld r1, [r2 + 0x02AA]!"), 0x1122_02AA)
    testing.expect_value(t, machine_word("ld r1, [r2 - 0x02AA]!"), 0x1122_06AA)
}

@(test)
test_data_transfer_post_increment_register_offset :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("ld r1, [r2] + r3"),       0x0128_0003)
    testing.expect_value(t, machine_word("ld r1, [r2] - r3"),       0x0128_0403)
    testing.expect_value(t, machine_word("ld r1, [r2] + r3 lsl 4"), 0x0128_1003)
    testing.expect_value(t, machine_word("ld r1, [r2] - r3 lsl 4"), 0x0128_1403)
}

@(test)
test_data_transfer_post_increment_writeback_immediate :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("ld r1, [r2] + 0x02AA"), 0x1128_02AA)
    testing.expect_value(t, machine_word("ld r1, [r2] - 0x02AA"), 0x1128_06AA)
}

@(test)
test_data_transfer_implicitly_shifted_immediate :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("ld r1, [r2 + 0x2AA0]"),  0x1120_12AA)
    testing.expect_value(t, machine_word("ld r1, [r2 + 0x14400]"), 0x1120_2851)
    testing.expect_value(t, machine_word("ld r1, [r2 + 0x6480]"),  0x1120_1992)
    testing.expect_value(t, machine_word("ld r1, [r2 + 0x34100]"), 0x1120_2341)
}

@(test)
test_data_transfer_push:: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("push lr"), 0x3FE2_0404)
}

@(test)
test_data_transfer_pop :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("pop lr"), 0x1FE8_0004)
}


// --- Move From PSR


@(test)
test_move_from_psr_unexpected_eol :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("smv"))
}

@(test)
test_move_from_psr_unexpected_token :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("smv!"))
}

@(test)
test_move_from_psr_register_value :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("smv r1"), 0x0101_8000)
}


// --- Set/Clear PSR Bits


@(test)
test_set_clear_psr_bits_unexpected_eol :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("sst"))
    testing.expect(t, produces_unexpected_token_error("scl"))
}

@(test)
test_set_clear_psr_bits_unexpected_token :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("sst!"))
    testing.expect(t, produces_unexpected_token_error("scl!"))
}

@(test)
test_set_clear_psr_bits_unencodable_immediate_value :: proc(t: ^testing.T) {
    testing.expect(t, produces_not_encodable_error("sst 0xFFF"))
}

@(test)
test_set_clear_psr_bits_register_value :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("sst r1"), 0x2003_8001)
    testing.expect_value(t, machine_word("scl r1"), 0x2001_8001)
}

@(test)
test_set_clear_psr_bits_immediate_value :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("sst 0xAA"), 0x3003_80AA)
    testing.expect_value(t, machine_word("scl 0xAA"), 0x3001_80AA)
}


// --- Data Processing


@(test)
test_data_processing_unexpected_eol :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("add"))
    testing.expect(t, produces_unexpected_token_error("add r1"))
    testing.expect(t, produces_unexpected_token_error("add r1,"))
    testing.expect(t, produces_unexpected_token_error("add r1, r2"))
    testing.expect(t, produces_unexpected_token_error("add r1, r2,"))
    testing.expect(t, produces_unexpected_token_error("add r1, r2, r3 lsl"))
    testing.expect(t, produces_unexpected_token_error("add r1, r2, 4 lsl"))
}

@(test)
test_data_processing_unexpected_token :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("add!"))
    testing.expect(t, produces_unexpected_token_error("add r1!"))
    testing.expect(t, produces_unexpected_token_error("add r1,!"))
    testing.expect(t, produces_unexpected_token_error("add r1, r2!"))
    testing.expect(t, produces_unexpected_token_error("add r1, r2,!"))
    testing.expect(t, produces_unexpected_token_error("add r1, r2, r3!"))
    testing.expect(t, produces_unexpected_token_error("add r1, r2, r3 lsl!"))
}

@(test)
test_data_processing_unencodable_immediate_value :: proc(t: ^testing.T) {
    testing.expect(t, produces_not_encodable_error("add r1, r2, 0x8001"))
}

@(test)
test_data_processing_unencodable_shift_value :: proc(t: ^testing.T) {
    testing.expect(t, produces_not_encodable_error("add r1, r2, r3 lsl 32"))
    testing.expect(t, produces_not_encodable_error("add r1, r2, r3 lsr 33"))
}

@(test)
test_data_processing_opcode_variants :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("add  r1, r2, r3"), 0x4120_0003)
    testing.expect_value(t, machine_word("adc  r1, r2, r3"), 0x4122_0003)
    testing.expect_value(t, machine_word("sub  r1, r2, r3"), 0x4124_0003)
    testing.expect_value(t, machine_word("sbc  r1, r2, r3"), 0x4126_0003)
    testing.expect_value(t, machine_word("and  r1, r2, r3"), 0x4128_0003)
    testing.expect_value(t, machine_word("or   r1, r2, r3"), 0x412A_0003)
    testing.expect_value(t, machine_word("xor  r1, r2, r3"), 0x412C_0003)
    testing.expect_value(t, machine_word("btc  r1, r2, r3"), 0x412E_0003)
    testing.expect_value(t, machine_word("addk r1, r2, r3"), 0x4121_0003)
    testing.expect_value(t, machine_word("adck r1, r2, r3"), 0x4123_0003)
    testing.expect_value(t, machine_word("subk r1, r2, r3"), 0x4125_0003)
    testing.expect_value(t, machine_word("sbck r1, r2, r3"), 0x4127_0003)
    testing.expect_value(t, machine_word("andk r1, r2, r3"), 0x4129_0003)
    testing.expect_value(t, machine_word("ork  r1, r2, r3"), 0x412B_0003)
    testing.expect_value(t, machine_word("xork r1, r2, r3"), 0x412D_0003)
    testing.expect_value(t, machine_word("btck r1, r2, r3"), 0x412F_0003)
}

@(test)
test_data_processing_immediate_value :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("add r1, r2, 0xAA"),  0x5120_00AA)
    testing.expect_value(t, machine_word("add r1, r2, -0xAA"), 0x5120_0356)
}

@(test)
test_data_processing_register_value_register_shift :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("add r1, r2, r3 lsl r4"), 0x4120_1003)
    testing.expect_value(t, machine_word("add r1, r2, r3 lsr r4"), 0x4120_9003)
    testing.expect_value(t, machine_word("add r1, r2, r3 asr r4"), 0x4121_9003)
}

@(test)
test_data_processing_immediate_value_register_shift :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("add r1, r2, 0xAA lsl r4"), 0x5120_10AA)
    testing.expect_value(t, machine_word("add r1, r2, 0xAA lsr r4"), 0x5120_90AA)
    testing.expect_value(t, machine_word("add r1, r2, 0xAA asr r4"), 0x5121_90AA)
}

@(test)
test_data_processing_register_value_immediate_shift :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("add r1, r2, r3 lsl 4"),  0x6120_1003)
    testing.expect_value(t, machine_word("add r1, r2, r3 lsr 4"),  0x6120_9003)
    testing.expect_value(t, machine_word("add r1, r2, r3 asr 4"),  0x6121_9003)
    testing.expect_value(t, machine_word("add r1, r2, r3 lsr 32"), 0x6120_8003)
}

@(test)
test_data_processing_immediate_value_immediate_shift :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("add r1, r2, 0xAA lsl 4"),  0x7120_10AA)
    testing.expect_value(t, machine_word("add r1, r2, 0xAA lsr 4"),  0x7120_90AA)
    testing.expect_value(t, machine_word("add r1, r2, 0xAA asr 4"),  0x7121_90AA)
    testing.expect_value(t, machine_word("add r1, r2, 0xAA lsr 32"), 0x7120_80AA)
}


// --- Data Processing Pseudo-Instructions


@(test)
test_data_processing_pseudo_unexpected_eol :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("tst"))
    testing.expect(t, produces_unexpected_token_error("tst r1"))
    testing.expect(t, produces_unexpected_token_error("tst r1,"))
    testing.expect(t, produces_unexpected_token_error("not"))
    testing.expect(t, produces_unexpected_token_error("not r1"))
    testing.expect(t, produces_unexpected_token_error("not r1,"))
    testing.expect(t, produces_unexpected_token_error("lsl, r1"))
    testing.expect(t, produces_unexpected_token_error("lsl, r1,"))
    testing.expect(t, produces_unexpected_token_error("lsl, r1, r2,"))
    testing.expect(t, produces_unexpected_token_error("mov"))
    testing.expect(t, produces_unexpected_token_error("mov,"))
    testing.expect(t, produces_unexpected_token_error("mov, r1"))
    testing.expect(t, produces_unexpected_token_error("mov, r1,"))
}

@(test)
test_data_processing_pseudo_unexpected_token :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("tst!"))
    testing.expect(t, produces_unexpected_token_error("tst r1!"))
    testing.expect(t, produces_unexpected_token_error("tst r1,!"))
    testing.expect(t, produces_unexpected_token_error("not!"))
    testing.expect(t, produces_unexpected_token_error("not r1!"))
    testing.expect(t, produces_unexpected_token_error("not r1,!"))
    testing.expect(t, produces_unexpected_token_error("lsl r1!"))
    testing.expect(t, produces_unexpected_token_error("lsl r1,!"))
    testing.expect(t, produces_unexpected_token_error("lsl r1, r2,!"))
    testing.expect(t, produces_unexpected_token_error("mov!"))
    testing.expect(t, produces_unexpected_token_error("mov,!"))
    testing.expect(t, produces_unexpected_token_error("mov r1!"))
    testing.expect(t, produces_unexpected_token_error("mov r1,!"))
}

@(test)
test_data_processing_pseudo_instruction_variants :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("nop"            ), 0x4001_0000)
    testing.expect_value(t, machine_word("tst  r1, r2"    ), 0x4018_0002)
    testing.expect_value(t, machine_word("teq  r1, r2"    ), 0x401C_0002)
    testing.expect_value(t, machine_word("cmp  r1, r2"    ), 0x4014_0002)
    testing.expect_value(t, machine_word("cpn  r1, r2"    ), 0x4010_0002)
    testing.expect_value(t, machine_word("not  r1, r2"    ), 0x512C_03FF)
    testing.expect_value(t, machine_word("notk r1, r2"    ), 0x512D_03FF)
    testing.expect_value(t, machine_word("lsl  r1, r2, r3"), 0x410A_0C02)
    testing.expect_value(t, machine_word("lsr  r1, r2, r3"), 0x410A_8C02)
    testing.expect_value(t, machine_word("asr  r1, r2, r3"), 0x410B_8C02)
    testing.expect_value(t, machine_word("lslk r1, r2, r3"), 0x410B_0C02)
    testing.expect_value(t, machine_word("mov  r1, r2"    ), 0x4121_0000)
}


// --- Branch Instructions


@(test)
test_branch_unexpected_eol :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("b"))
}

@(test)
test_branch_unexpected_token :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("b!"))
}

@(test)
test_branch_register_variants :: proc(t: ^testing.T) {
    testing.expect_value(t, instruction("beq  r1"), Instruction{ machine_word = 0x8000_0001 })
    testing.expect_value(t, instruction("bne  r1"), Instruction{ machine_word = 0x8100_0001 })
    testing.expect_value(t, instruction("bcs  r1"), Instruction{ machine_word = 0x8200_0001 })
    testing.expect_value(t, instruction("bcc  r1"), Instruction{ machine_word = 0x8300_0001 })
    testing.expect_value(t, instruction("bmi  r1"), Instruction{ machine_word = 0x8400_0001 })
    testing.expect_value(t, instruction("bpl  r1"), Instruction{ machine_word = 0x8500_0001 })
    testing.expect_value(t, instruction("bvs  r1"), Instruction{ machine_word = 0x8600_0001 })
    testing.expect_value(t, instruction("bvc  r1"), Instruction{ machine_word = 0x8700_0001 })
    testing.expect_value(t, instruction("bhi  r1"), Instruction{ machine_word = 0x8800_0001 })
    testing.expect_value(t, instruction("bls  r1"), Instruction{ machine_word = 0x8900_0001 })
    testing.expect_value(t, instruction("bge  r1"), Instruction{ machine_word = 0x8A00_0001 })
    testing.expect_value(t, instruction("blt  r1"), Instruction{ machine_word = 0x8B00_0001 })
    testing.expect_value(t, instruction("bgt  r1"), Instruction{ machine_word = 0x8C00_0001 })
    testing.expect_value(t, instruction("ble  r1"), Instruction{ machine_word = 0x8D00_0001 })
    testing.expect_value(t, instruction("b    r1"), Instruction{ machine_word = 0x8E00_0001 })
    testing.expect_value(t, instruction("bleq r1"), Instruction{ machine_word = 0xA000_0001 })
    testing.expect_value(t, instruction("blne r1"), Instruction{ machine_word = 0xA100_0001 })
    testing.expect_value(t, instruction("blcs r1"), Instruction{ machine_word = 0xA200_0001 })
    testing.expect_value(t, instruction("blcc r1"), Instruction{ machine_word = 0xA300_0001 })
    testing.expect_value(t, instruction("blmi r1"), Instruction{ machine_word = 0xA400_0001 })
    testing.expect_value(t, instruction("blpl r1"), Instruction{ machine_word = 0xA500_0001 })
    testing.expect_value(t, instruction("blvs r1"), Instruction{ machine_word = 0xA600_0001 })
    testing.expect_value(t, instruction("blvc r1"), Instruction{ machine_word = 0xA700_0001 })
    testing.expect_value(t, instruction("blhi r1"), Instruction{ machine_word = 0xA800_0001 })
    testing.expect_value(t, instruction("blls r1"), Instruction{ machine_word = 0xA900_0001 })
    testing.expect_value(t, instruction("blge r1"), Instruction{ machine_word = 0xAA00_0001 })
    testing.expect_value(t, instruction("bllt r1"), Instruction{ machine_word = 0xAB00_0001 })
    testing.expect_value(t, instruction("blgt r1"), Instruction{ machine_word = 0xAC00_0001 })
    testing.expect_value(t, instruction("blle r1"), Instruction{ machine_word = 0xAD00_0001 })
    testing.expect_value(t, instruction("bl   r1"), Instruction{ machine_word = 0xAE00_0001 })
}

@(test)
test_branch_label_variants :: proc(t: ^testing.T) {
    testing.expect_value(t, instruction("beq  symbol"), Instruction{ machine_word = 0x9000_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bne  symbol"), Instruction{ machine_word = 0x9100_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bcs  symbol"), Instruction{ machine_word = 0x9200_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bcc  symbol"), Instruction{ machine_word = 0x9300_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bmi  symbol"), Instruction{ machine_word = 0x9400_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bpl  symbol"), Instruction{ machine_word = 0x9500_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bvs  symbol"), Instruction{ machine_word = 0x9600_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bvc  symbol"), Instruction{ machine_word = 0x9700_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bhi  symbol"), Instruction{ machine_word = 0x9800_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bls  symbol"), Instruction{ machine_word = 0x9900_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bge  symbol"), Instruction{ machine_word = 0x9A00_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("blt  symbol"), Instruction{ machine_word = 0x9B00_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bgt  symbol"), Instruction{ machine_word = 0x9C00_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("ble  symbol"), Instruction{ machine_word = 0x9D00_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("b    symbol"), Instruction{ machine_word = 0x9E00_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bleq symbol"), Instruction{ machine_word = 0xB000_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("blne symbol"), Instruction{ machine_word = 0xB100_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("blcs symbol"), Instruction{ machine_word = 0xB200_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("blcc symbol"), Instruction{ machine_word = 0xB300_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("blmi symbol"), Instruction{ machine_word = 0xB400_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("blpl symbol"), Instruction{ machine_word = 0xB500_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("blvs symbol"), Instruction{ machine_word = 0xB600_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("blvc symbol"), Instruction{ machine_word = 0xB700_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("blhi symbol"), Instruction{ machine_word = 0xB800_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("blls symbol"), Instruction{ machine_word = 0xB900_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("blge symbol"), Instruction{ machine_word = 0xBA00_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bllt symbol"), Instruction{ machine_word = 0xBB00_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("blgt symbol"), Instruction{ machine_word = 0xBC00_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("blle symbol"), Instruction{ machine_word = 0xBD00_0000, relocation_symbol = "symbol" })
    testing.expect_value(t, instruction("bl   symbol"), Instruction{ machine_word = 0xBE00_0000, relocation_symbol = "symbol" })
}


// --- Move Immediate Instruction


@(test)
test_move_immediate_unexpected_eol :: proc(t: ^testing.T)  {
    testing.expect(t, produces_unexpected_token_error("mvi"))
    testing.expect(t, produces_unexpected_token_error("mvi r1"))
    testing.expect(t, produces_unexpected_token_error("mvi r1,"))
}

@(test)
test_move_immediate_unexpected_token :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("mvi!"))
    testing.expect(t, produces_unexpected_token_error("mvi r1!"))
    testing.expect(t, produces_unexpected_token_error("mvi r1,!"))
}

@(test)
test_move_immediate_unencodable_value :: proc(t: ^testing.T) {
    testing.expect(t, produces_not_encodable_error("mvi r1, 0x01FF_FFFF"))
    testing.expect(t, produces_not_encodable_error("mvi r1, -0x01FF_FFFF"))
}

@(test)
test_move_immediate_encoding :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("mvi r1, 0xAA" ),       0xC100_00AA)
    testing.expect_value(t, machine_word("mvi r1, -0xAA"),       0xD1FF_FF56)
    testing.expect_value(t, machine_word("mvi r1, 0xFF00_0001"), 0xD100_0001)
}


// --- Software Interrupt Instruction


@(test)
test_software_interrupt_unexpected_token :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("swi!"))
}

@(test)
test_software_interrupt_unencodable_value :: proc(t: ^testing.T) {
    testing.expect(t, produces_not_encodable_error("swi 0xDEAD_BEEF"))
}

@(test)
test_software_interrupt_encoding :: proc(t: ^testing.T) {
    testing.expect_value(t, machine_word("swi"),        0xE000_0000)
    testing.expect_value(t, machine_word("swi 0xBEEF"), 0xE000_BEEF)
}


// --- m32 Pseudo-Instruction

@(test)
test_m32_unexpected_eol :: proc(t: ^testing.T)  {
    testing.expect(t, produces_unexpected_token_error("m32"))
    testing.expect(t, produces_unexpected_token_error("m32 r1"))
    testing.expect(t, produces_unexpected_token_error("m32 r1,"))
}

@(test)
test_m32_unexpected_token :: proc(t: ^testing.T) {
    testing.expect(t, produces_unexpected_token_error("m32!"))
    testing.expect(t, produces_unexpected_token_error("m32 r1!"))
    testing.expect(t, produces_unexpected_token_error("m32 r1,!"))
}

@(test)
test_m32_unencodable_value :: proc(t: ^testing.T) {
    testing.expect(t, produces_not_encodable_error("m32 r1, 0x1FFFF_FFFF"))
    testing.expect(t, produces_not_encodable_error("m32 r1, -0x8000_0001"))
}

@(test)
test_m32_encoding :: proc(t: ^testing.T) {
    instr, err := instruction_and_error("m32 r1, 0xAA_AAAA")
    testing.expect(t, err == nil)
    testing.expect_value(t, instr.machine_word,      0xC1AA_AAAA)
    testing.expect_value(t, instr.machine_word2,     nil)
    testing.expect_value(t, instr.relocation_symbol, nil)

    instr, err = instruction_and_error("m32 r1, -1")
    testing.expect(t, err == nil)
    testing.expect_value(t, instr.machine_word,      0xD1FF_FFFF)
    testing.expect_value(t, instr.machine_word2,     nil)
    testing.expect_value(t, instr.relocation_symbol, nil)

    instr, err = instruction_and_error("m32 r1, 0xDEAD_BEEF")
    testing.expect(t, err == nil)
    testing.expect_value(t, instr.machine_word,  0xC1AD_BEEF)
    testing.expect_value(t, instr.machine_word2, 0x7111_60DE)
    testing.expect_value(t, instr.relocation_symbol, nil)

    instr, err = instruction_and_error("m32 r1, -0x5555_5555")
    testing.expect(t, err == nil)
    testing.expect_value(t, instr.machine_word,  0xC1AA_AAAB)
    testing.expect_value(t, instr.machine_word2, 0x7111_60AA)
    testing.expect_value(t, instr.relocation_symbol, nil)

    instr, err = instruction_and_error("m32 r1, symbol")
    testing.expect(t, err == nil)
    testing.expect_value(t, instr.machine_word,  0xC100_0000)
    testing.expect_value(t, instr.machine_word2, 0x7111_6000)
    testing.expect_value(t, instr.relocation_symbol, "symbol")
}
