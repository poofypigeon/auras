package auras

import "core:fmt"
import "core:slice"

code :: `
;start:
    nop
    nop
    nop
;    m32     r1, string
;    bl      str_to_upper
;done:
;    b       done
;
;str_to_upper:
;    ; read string size into r2
;    ld      r2, [r1] + 4
;str_to_upper_loop:
;    cmp     r2, 0
;    beq     lr
;    sub     r2, r2, 1
;    ldb     r13, [r1] + 1
;    cmp     r13, 0x61 ; 'a'
;    blt     str_to_upper_loop
;    cmp     r13, 0x71 ; 'z'
;    bgt     str_to_upper_loop
;    sub     r13, r13, 0x20 ; 'a' - 'A'
;    stb     r13, [r2 - 1]
;    b       str_to_upper_loop
;
;string:
;    word * ascii "Hello, world!"
;    ;align 4
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
