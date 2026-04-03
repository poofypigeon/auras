#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "string_to_int_map.h"
#include "string_slice.h"

const size_t map_capacity_primes[] = {
    53,        97,         193,       389,
    769,       1543,       3079,      6151,
    12289,     24593,      49157,     98317,
    196613,    393241,     786433,    1572869,
    3145739,   6291469,    12582917,  25165843,
    50331653,  100663319,  201326611, 402653189,
    805306457, 1610612741,
};

static StringToIntMap init(size_t times_grown) {
    StringToIntMap map = { .times_grown = times_grown };

    map.key = calloc(map_capacity_primes[times_grown], sizeof(StringSlice));
    if (map.key == nullptr) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    map.value = malloc(map_capacity_primes[times_grown]*sizeof(int64_t));
    if (map.value == nullptr) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    return map;
}

StringToIntMap map_init() {
    return init(0);
}

void map_free(StringToIntMap* map) {
    free(map->key);
    free(map->value);
    map->size = 0;
    map->times_grown = 0;
}

static size_t hash(StringSlice key, size_t m) {
    assert(key.length > 0);

    static const size_t P = 67;
    static const uint8_t remap[] = {
        ['0'] = 1,  ['1'] = 2,  ['2'] = 3,  ['3'] = 4,  ['4'] = 5,  ['5'] = 6,  ['6'] = 7,  ['7'] = 8,  ['8'] = 9,  ['9'] = 10,
        ['A'] = 11, ['B'] = 12, ['C'] = 13, ['D'] = 14, ['E'] = 15, ['F'] = 16, ['G'] = 17, ['H'] = 18, ['I'] = 19, ['J'] = 20,
        ['K'] = 21, ['L'] = 22, ['M'] = 23, ['N'] = 24, ['O'] = 25, ['P'] = 26, ['Q'] = 27, ['R'] = 28, ['S'] = 29, ['T'] = 30,
        ['U'] = 31, ['V'] = 32, ['W'] = 33, ['X'] = 34, ['Y'] = 35, ['Z'] = 36,
        ['a'] = 41, ['b'] = 42, ['c'] = 43, ['d'] = 44, ['e'] = 45, ['f'] = 46, ['g'] = 47, ['h'] = 48, ['i'] = 49, ['j'] = 50,
        ['k'] = 51, ['l'] = 52, ['m'] = 53, ['n'] = 54, ['o'] = 55, ['p'] = 56, ['q'] = 57, ['r'] = 58, ['s'] = 59, ['t'] = 60,
        ['u'] = 61, ['v'] = 62, ['w'] = 63, ['x'] = 64, ['y'] = 65, ['z'] = 66,
        ['_'] = 67,
    };

    size_t p = 1;
    size_t hash_value = 0;
    for (size_t i = 0; i < key.length; i++) {
        hash_value = (hash_value + (size_t)remap[(size_t)key.bytes[i]] * p) % m;
        p = (P * p) % m;
    }
    return hash_value;
}

// fast insert without safegaurds for growing table
// assumes no replicate keys, and that a space is guaranteed to exist
static void insert(StringToIntMap* map, StringSlice key, int64_t value) {
    size_t map_capacity = map_capacity_primes[map->times_grown];

    size_t hash_value = hash(key, map_capacity);
    for (size_t i = 0;; i++) {
        size_t probe_hash_value = (hash_value + i*i) % map_capacity;
        if (map->key[probe_hash_value].length == 0) {
            map->key[probe_hash_value] = key;
            map->value[probe_hash_value] = value;
            return;
        }
    }
}

void map_grow(StringToIntMap* map) {
    size_t map_capacity = map_capacity_primes[map->times_grown];

    if (map->times_grown == sizeof(map_capacity_primes)/sizeof(size_t) - 1) {
        fprintf(stderr, "cannot grow map");
        exit(EXIT_FAILURE);
    }

    StringToIntMap new_map = init(map->times_grown + 1);
    for (size_t i = 0; i < map_capacity; i++) {
        StringSlice key = map->key[i];
        if (key.length != 0) {
            int64_t value = map->value[i];
            insert(&new_map, key, value);
        }
    }
    new_map.size = map->size;

    free(map->key);
    free(map->value);
    *map = new_map;
}

bool map_insert(StringToIntMap* map, StringSlice key, int64_t value) {
    size_t map_capacity = map_capacity_primes[map->times_grown];

    // grow table if load factor would exceed 0.5 after inserting
    if (map_capacity/2 < (map->size + 1)) {
        map_grow(map);
    }

    size_t hash_value = hash(key, map_capacity);
    for (size_t i = 0; i < map_capacity; i++) {
        size_t probe_hash_value = (hash_value + i*i) % map_capacity;
        if (map->key[probe_hash_value].length == 0) {
            map->key[probe_hash_value] = key;
            map->value[probe_hash_value] = value;
            map->size++;
            return true;
        }
        if (slice_eq(key, map->key[probe_hash_value])) return false;
    }

    // could not insert after max iterations
    if (map->times_grown == sizeof(map_capacity_primes)/sizeof(size_t) - 1) {
        fprintf(stderr, "no room to insert into map");
        exit(EXIT_FAILURE);
    }

    assert(false);
    unreachable();
}

bool map_find(StringToIntMap* map, StringSlice key, int64_t* value) {
    if (map->size == 0) return false;

    size_t map_capacity = map_capacity_primes[map->times_grown];

    size_t hash_value = hash(key, map_capacity);
    for (size_t i = 0; i < map_capacity; i++) {
        size_t probe_hash_value = (hash_value + i*i) % map_capacity;
        if (map->key[probe_hash_value].length == 0) return false;
        if (slice_eq(key, map->key[probe_hash_value])) {
            *value = map->value[probe_hash_value];
            return true;
        }
    }

    // not found after max iterations
    if (map->times_grown == sizeof(map_capacity_primes)/sizeof(size_t) - 1) {
        return false;
    }

    assert(false);
    unreachable();
}

void map_print(StringToIntMap* map) {
    size_t map_capacity = map_capacity_primes[map->times_grown];
    printf("capacity:    %zu\n",  map_capacity);
    printf("size:        %zu\n",  map->size);
    printf("load factor: %.2f\n", (float)map->size / (float)map_capacity);
    for (size_t i = 0; i < map_capacity; i++) {
        char* key = strndup(map->key[i].bytes, map->key[i].length);
        printf("[%zu]: \"%s\" -> %lld\n", i, key, map->value[i]);
        free(key);
    }
    printf("\n");
}
