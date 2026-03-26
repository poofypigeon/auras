
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <check.h>

#include "../src/string_slice.h"
#include "../src/string_to_int_map.h"

// ================================================================
//  map
// ================================================================

// --- init ---

START_TEST(test_map_init) {
    StringToIntMap map = map_init();

    ck_assert_uint_eq(map.size, 0);
    ck_assert_uint_eq(map.times_grown, 0);
    ck_assert_ptr_nonnull(map.key);
    ck_assert_ptr_nonnull(map.value);
    for (size_t i = 0; i < 53; i++) {
        ck_assert_uint_eq(map.key[i].length, 0);
        ck_assert_ptr_null(map.key[i].bytes);
    }
    map_free(&map);
} END_TEST

// --- insert ---

START_TEST(test_map_insert_one) {
    StringToIntMap map = map_init();
    StringSlice key = slice_from_cstring("key");

    bool success = map_insert(&map, key, 1);
    ck_assert(success);
    ck_assert_uint_eq(map.size, 1);
    ck_assert_uint_eq(map.times_grown, 0);

    size_t entries_found = 0;
    for (size_t i = 0; i < 53; i++) {
        if (map.key[i].length != 0) {
            ck_assert(slice_eq(map.key[i], key));
            ck_assert_int_eq(map.value[i], 1);
            entries_found++;
        }
    }
    ck_assert_uint_eq(entries_found, 1);

    map_free(&map);
} END_TEST

START_TEST(test_map_insert_duplicate) {
    StringToIntMap map = map_init();
    StringSlice key = slice_from_cstring("key");
    bool success = false;

    success = map_insert(&map, key, 1);
    ck_assert(success);
    ck_assert_uint_eq(map.size, 1);
    ck_assert_uint_eq(map.times_grown, 0);

    success = map_insert(&map, key, 2);
    ck_assert(!success);
    ck_assert_uint_eq(map.size, 1);
    ck_assert_uint_eq(map.times_grown, 0);

    size_t entries_found = 0;
    for (size_t i = 0; i < 53; i++) {
        if (map.key[i].length != 0) {
            ck_assert(slice_eq(map.key[i], key));
            ck_assert_int_eq(map.value[i], 1);
            entries_found++;
        }
    }
    ck_assert_uint_eq(entries_found, 1);

    map_free(&map);
} END_TEST

const char* key_strings[64] = {
    "TOIBKL7", "bY4",     "Yjup6b",  "YmC_Hdgf",
    "DvYY",    "A_w",     "i",       "cfC_fd",
    "JIGFMq",  "s82Q",    "WeZvxx",  "Uj4KSIz",
    "es40xf",  "_tdN",    "XCRsGf6", "W0EOzw",
    "RLmH",    "mi7rrW",  "ApxNO23", "PXFdc6I",
    "Ctr",     "YUv4k1",  "zn0qz",   "KfoqgL",
    "tKHJrX8", "BF7z",    "xDx5nSq", "uxuFKkW",
    "y",       "p_L",     "bUywLH",  "QKlEwpX",
    "nSg3l",   "VR02aIE", "P0Z6XDr", "qr69",
    "At6ahv3", "c3gWEcK", "yzTWvPD", "naqRll",
    "AK7PFd0", "rAJugvw", "NFLH70o", "W19GDzJ",
    "AYmrSec", "h8gl",    "iRM8I2h", "n349ZDy",
    "ezHj3y",  "Yl9ucJJ", "ioxIGj2", "Q",
    "wjOtJF",  "nvw1pVH", "Sj4J",    "Il3sjYJ",
    "R9Hjwh",  "RcWmKr",  "3eJn2Fs", "F9OoIJY",
    "gof",     "PkKj2A",  "W1fM8",   "pJ9coyO",
};

START_TEST(test_map_insert_many) {
    StringToIntMap map = map_init();

    for (size_t i = 0; i < 16; i++) {
        bool success = map_insert(&map, slice_from_cstring(key_strings[i]), i);
        ck_assert(success);
    }
    ck_assert_uint_eq(map.size, 16);
    ck_assert_uint_eq(map.times_grown, 0);

    size_t entries_found = 0;
    for (size_t i = 0; i < 53; i++) {
        if (map.key[i].length != 0) {
            int64_t value = map.value[i];
            ck_assert_int_lt(value, 16);
            ck_assert(slice_eq(map.key[i], slice_from_cstring(key_strings[value])));
            entries_found++;
        }
    }
    ck_assert_uint_eq(entries_found, 16);

    map_free(&map);
} END_TEST

START_TEST(test_map_grow_once) {
    StringToIntMap map = map_init();

    for (size_t i = 0; i < 32; i++) {
        bool success = map_insert(&map, slice_from_cstring(key_strings[i]), i);
        ck_assert(success);
    }
    ck_assert_uint_eq(map.size, 32);
    ck_assert_uint_eq(map.times_grown, 1);

    size_t entries_found = 0;
    for (size_t i = 0; i < 97; i++) {
        if (map.key[i].length != 0) {
            int64_t value = map.value[i];
            ck_assert_int_lt(value, 32);
            ck_assert(slice_eq(map.key[i], slice_from_cstring(key_strings[value])));
            entries_found++;
        }
    }
    ck_assert_uint_eq(entries_found, 32);

    map_free(&map);
} END_TEST

START_TEST(test_map_grow_twice) {
    StringToIntMap map = map_init();

    for (size_t i = 0; i < 64; i++) {
        bool success = map_insert(&map, slice_from_cstring(key_strings[i]), i);
        ck_assert(success);
    }
    ck_assert_uint_eq(map.size, 64);
    ck_assert_uint_eq(map.times_grown, 2);

    size_t entries_found = 0;
    for (size_t i = 0; i < 193; i++) {
        if (map.key[i].length != 0) {
            int64_t value = map.value[i];
            ck_assert_int_lt(value, 64);
            ck_assert(slice_eq(map.key[i], slice_from_cstring(key_strings[value])));
            entries_found++;
        }
    }
    ck_assert_uint_eq(entries_found, 64);

    map_free(&map);
} END_TEST

// --- find ---

START_TEST(test_map_find_empty) {
    StringToIntMap map = map_init();
    StringSlice key = slice_from_cstring("key");

    int64_t value = 0;
    bool success = map_find(&map, key, &value);
    ck_assert(!success);

    map_free(&map);
} END_TEST

START_TEST(test_map_find_one) {
    StringToIntMap map = map_init();
    StringSlice key = slice_from_cstring("key");
    bool success = false;

    success = map_insert(&map, key, 1);
    ck_assert(success);

    int64_t value = 0;
    success = map_find(&map, key, &value);
    ck_assert(success);
    ck_assert_int_eq(value, 1);

    map_free(&map);
} END_TEST

START_TEST(test_map_find_absent) {
    StringToIntMap map = map_init();
    StringSlice key_present = slice_from_cstring("present");
    StringSlice key_absent = slice_from_cstring("absent");
    bool success = false;

    success = map_insert(&map, key_present, 1);
    ck_assert(success);

    int64_t value = 0;
    success = map_find(&map, key_absent, &value);
    ck_assert(!success);

    map_free(&map);
} END_TEST

Suite* map_suite(void) {
    Suite* suite = suite_create("map");

    TCase* tc_map_init = tcase_create("map_init");
    tcase_add_test(tc_map_init, test_map_init);

    TCase* tc_map_insert = tcase_create("map_insert");
    tcase_add_test(tc_map_insert, test_map_insert_one);
    tcase_add_test(tc_map_insert, test_map_insert_duplicate);
    tcase_add_test(tc_map_insert, test_map_insert_many);
    tcase_add_test(tc_map_insert, test_map_grow_once);
    tcase_add_test(tc_map_insert, test_map_grow_twice);

    TCase* tc_map_find = tcase_create("map_find");
    tcase_add_test(tc_map_find, test_map_find_empty);
    tcase_add_test(tc_map_find, test_map_find_one);
    tcase_add_test(tc_map_find, test_map_find_absent);

    suite_add_tcase(suite, tc_map_init);
    suite_add_tcase(suite, tc_map_insert);
    suite_add_tcase(suite, tc_map_find);

    return suite;
}

// ================================================================
//  main
// ================================================================

 int main(void) {
    SRunner* suite_runner = srunner_create(map_suite());

    srunner_run_all(suite_runner, CK_NORMAL);
    int number_failed = srunner_ntests_failed(suite_runner);
    srunner_free(suite_runner);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }
