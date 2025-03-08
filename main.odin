#+private

package auras

import "core:fmt"
import "core:slice"

code :: `
    ld r2, [r0 + 1 lsl 2]
`

main :: proc() {
    file := create_source_file()
    defer cleanup_source_file(&file)

    if ok := process_text(&file, code); !ok {
        return
    }

    for i := 0; i < len(file.buffer); i += SIZE_OF_WORD {
        word := ((^u32le)(&file.buffer[i]))^

        ok: bool

        bytes_left := len(file.buffer) - i
        instr := "    <unknown>"
        if bytes_left >= SIZE_OF_WORD {
            result, ok := decode_instruction(word)
            if ok {
                instr = result
            }
        }

        fmt.printf("%08X:", i, flush = false)
        bytes_to_print := min(bytes_left, SIZE_OF_WORD)
        for instr_byte in file.buffer[i:i+bytes_to_print] {
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
