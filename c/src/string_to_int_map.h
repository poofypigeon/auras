#pragma once

#include <stdlib.h>

#include "string_slice.h"

typedef struct {
    size_t times_grown;
    size_t size;
    StringSlice* key;
    int64_t* value;
} StringToIntMap;

StringToIntMap map_init();
void map_free(StringToIntMap* map);
bool map_insert(StringToIntMap* map, StringSlice key, int64_t value);
bool map_find(StringToIntMap* map, StringSlice key, int64_t* value);

// debug routine
void map_print(StringToIntMap* map);
