#include <stdint.h>

#include "endianness.h"

union {
    uint32_t uint32;
    uint8_t uint8[4];
} endianness_test_word = { .uint32 = 0x00000001 };

uint32_t htole32(uint32_t v) {
    if (endianness_test_word.uint8[0] == 0x01) return v;
    return v << 24
         | ((v << 8) & 0x00FF0000)
         | ((v >> 8) & 0x0000FF00)
         | v >> 24;
}
