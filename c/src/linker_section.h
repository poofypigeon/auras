#pragma once

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "line_error.h"
#include "string_to_int_map.h"

constexpr size_t UNDEFINED_OFFSET = SIZE_T_MAX;

constexpr size_t SIZE_OF_WORD = 4;
constexpr size_t SIZE_OF_HALF = 2;
constexpr size_t SIZE_OF_BYTE = 1;

typedef struct {
    size_t section_offset;
    size_t symbol_table_index;
} RelocationTableEntry;

typedef struct {
    size_t section_offset;
    size_t string_table_start_index;
} SymbolTableEntry;

typedef struct {
    size_t length;
    size_t capacity;
    char* items;
} CharBuffer;

typedef struct {
    size_t length;
    size_t capacity;
    RelocationTableEntry* items;
} RelocationTable;

typedef struct {
    size_t length;
    size_t capacity;
    SymbolTableEntry* items;
} SymbolTable;

typedef struct LinkerSection {
    CharBuffer buffer;
    SymbolTable symbol_table;
    RelocationTable relocation_table;
    CharBuffer string_table;
    StringToIntMap symbol_map;
} LinkerSection;

typedef enum {
    LINE_HAS_SECTION_CONTENT,
    LINE_HAS_DIRECTIVE,
} LineContent;

LinkerSection linker_section_init(void);
void linker_section_free(LinkerSection* section);
LineContent process_line(LinkerSection* section, StringSlice line_slice, StringToIntMap* defines, LineError* err);
