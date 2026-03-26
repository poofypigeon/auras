#include <stdlib.h>
#include <string.h>

#include "string_slice.h"

StringSlice slice_from_cstring(const char* str) {
    return (StringSlice){ .length = strlen(str), .bytes = str };
}

char* quoted_cstring_from_slice(StringSlice slice) {
    char* cstring = malloc(slice.length + 2 + 1);
    memcpy(cstring + 1, slice.bytes, slice.length);
    cstring[             0] = '\'';
    cstring[slice.length+1] = '\'';
    cstring[slice.length+2] = '\0';
    return cstring;
}

bool slice_eq(StringSlice slice1, StringSlice slice2) {
    if (slice1.length != slice2.length) return false;
    return (strncmp(slice1.bytes, slice2.bytes, slice1.length) == 0);
}
