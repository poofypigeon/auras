#+private

package auras

import "core:fmt"
import "core:slice"

HELLO_WORLD :: `
_vec_none:
    mvi sp, 0x3FFF
_vec_reset:
    b hello_world
_vec_syscall:
    nop
_vec_bus_fault:
    nop
_vec_usage_fault:
    nop
_vec_instruction:
    nop
_vec_systick:
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
_vec_irq0:
    nop
_vec_irq1:
    nop
_vec_irq2:
    nop
_vec_irq3:
    nop
_vec_irq4:
    nop
_vec_irq5:
    nop
_vec_irq6:
    nop
_vec_irq7:
    nop

_handler_syscall:
    scl 0xC0

hello_world:
    mvi r1, 0
    m32 r2, hello_world_string
    ld  r3, [r2] + 4
    swi 0
done:
    b done
    
hello_world_string:
    word * ascii "Hello world!"

    align 0x4000
`

main :: proc() {
    file := create_source_file()
    defer cleanup_source_file(&file)

    if ok := process_text(&file, HELLO_WORLD); !ok {
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
