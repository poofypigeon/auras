#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    size_t length;
    const char* bytes;
} StringSlice;

StringSlice slice_from_cstring(const char* str);

char* cstring_from_slice(StringSlice slice);
char* quoted_cstring_from_slice(StringSlice slice);

bool slice_eq(StringSlice slice1, StringSlice slice2);
