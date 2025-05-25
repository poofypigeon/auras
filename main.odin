#+private

package auras

import "core:fmt"
import "core:slice"

text :: `
label:
    mvi r0, -'\\\'
`

main :: proc() {
    code, ok := code_from_text(text)
    if !ok { return }

    for i := 0; i < len(code.buffer); i += SIZE_OF_WORD {
        word := u32(((^u32le)(&code.buffer[i]))^)

        ok: bool

        bytes_left := len(code.buffer) - i
        instr := "    <unknown>"
        if bytes_left >= SIZE_OF_WORD {
            result, ok := decode_instruction(word)
            if ok {
                instr = result
            }
        }

        fmt.printf("%08X:", i, flush = false)
        bytes_to_print := min(bytes_left, SIZE_OF_WORD)
        for instr_byte in code.buffer[i:i+bytes_to_print] {
            fmt.printf(" %02X", instr_byte, flush = false)
        }
        for _ in 0..<SIZE_OF_WORD - bytes_to_print {
            fmt.printf("   ", flush = false)
        }
        fmt.printfln("  %s", instr)
        if ok {
            delete(instr)
        }
    }
}
