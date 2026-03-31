/* ANSI-C code produced by gperf version 3.3 */
/* Command-line: gperf -tc7 --output-file=src/gperf/perfect_hash.c src/gperf/perfect_hash.gperf  */
/* Computed positions: -k'1-4' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 1 "src/gperf/perfect_hash.gperf"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "perfect_hash.h"
#include "../string_slice.h"
#line 9 "src/gperf/perfect_hash.gperf"
struct MnemonicToken {
    char* name;
    Mnemonic mnemonic;
};

#define TOTAL_KEYWORDS 61
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 7
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 129
/* maximum key range = 129, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register size_t len)
{
  static unsigned char asso_values[] =
    {
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130,  35,   0,   0,
        5,  15,  41,   0,   6,  15, 130,   0,   0,  10,
       50,  20,  25,  45,  50,   5,  30,   2,  61,  45,
       35, 130,  35, 130, 130, 130, 130, 130, 130
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[3]];
#if (defined __cplusplus && (__cplusplus >= 201703L || (__cplusplus >= 201103L && defined __clang__ && __clang_major__ + (__clang_minor__ >= 9) > 3))) || (defined __STDC_VERSION__ && __STDC_VERSION__ >= 202000L && ((defined __GNUC__ && __GNUC__ >= 10) || (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || (defined __clang__ && __clang_major__ >= 10)
      __attribute__ ((__fallthrough__));
#endif
      /*FALLTHROUGH*/
      case 3:
        hval += asso_values[(unsigned char)str[2]];
#if (defined __cplusplus && (__cplusplus >= 201703L || (__cplusplus >= 201103L && defined __clang__ && __clang_major__ + (__clang_minor__ >= 9) > 3))) || (defined __STDC_VERSION__ && __STDC_VERSION__ >= 202000L && ((defined __GNUC__ && __GNUC__ >= 10) || (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || (defined __clang__ && __clang_major__ >= 10)
      __attribute__ ((__fallthrough__));
#endif
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]+1];
#if (defined __cplusplus && (__cplusplus >= 201703L || (__cplusplus >= 201103L && defined __clang__ && __clang_major__ + (__clang_minor__ >= 9) > 3))) || (defined __STDC_VERSION__ && __STDC_VERSION__ >= 202000L && ((defined __GNUC__ && __GNUC__ >= 10) || (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || (defined __clang__ && __clang_major__ >= 10)
      __attribute__ ((__fallthrough__));
#endif
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

struct MnemonicToken *
in_word_set (register const char *str, register size_t len)
{
#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 6) > 4) || (defined __clang__ && __clang_major__ >= 3)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
  static struct MnemonicToken wordlist[] =
    {
      {""},
#line 59 "src/gperf/perfect_hash.gperf"
      {"b", MN_B},
#line 21 "src/gperf/perfect_hash.gperf"
      {"lb", MN_LB},
      {""}, {""},
#line 23 "src/gperf/perfect_hash.gperf"
      {"lbu", MN_LBU},
      {""},
#line 26 "src/gperf/perfect_hash.gperf"
      {"sb", MN_SB},
#line 38 "src/gperf/perfect_hash.gperf"
      {"sbc", MN_SBC},
#line 45 "src/gperf/perfect_hash.gperf"
      {"sbck", MN_SBCK},
      {""}, {""},
#line 67 "src/gperf/perfect_hash.gperf"
      {"bl", MN_BL},
#line 54 "src/gperf/perfect_hash.gperf"
      {"srl", MN_SRL},
#line 57 "src/gperf/perfect_hash.gperf"
      {"srlk", MN_SRLK},
      {""}, {""},
#line 22 "src/gperf/perfect_hash.gperf"
      {"lh", MN_LH},
#line 53 "src/gperf/perfect_hash.gperf"
      {"sll", MN_SLL},
#line 56 "src/gperf/perfect_hash.gperf"
      {"sllk", MN_SLLK},
#line 24 "src/gperf/perfect_hash.gperf"
      {"lhu", MN_LHU},
      {""},
#line 27 "src/gperf/perfect_hash.gperf"
      {"sh", MN_SH},
#line 65 "src/gperf/perfect_hash.gperf"
      {"bhs", MN_BHS},
#line 63 "src/gperf/perfect_hash.gperf"
      {"bge", MN_BGE},
#line 73 "src/gperf/perfect_hash.gperf"
      {"blhs", MN_BLHS},
      {""},
#line 40 "src/gperf/perfect_hash.gperf"
      {"or", MN_OR},
#line 47 "src/gperf/perfect_hash.gperf"
      {"ork", MN_ORK},
#line 71 "src/gperf/perfect_hash.gperf"
      {"blge", MN_BLGE},
      {""}, {""}, {""},
#line 64 "src/gperf/perfect_hash.gperf"
      {"blo", MN_BLO},
#line 72 "src/gperf/perfect_hash.gperf"
      {"bllo", MN_BLLO},
      {""}, {""},
#line 20 "src/gperf/perfect_hash.gperf"
      {"lw", MN_LW},
#line 61 "src/gperf/perfect_hash.gperf"
      {"bne", MN_BNE},
#line 74 "src/gperf/perfect_hash.gperf"
      {"blmi", MN_BLMI},
      {""}, {""},
#line 25 "src/gperf/perfect_hash.gperf"
      {"sw", MN_SW},
#line 62 "src/gperf/perfect_hash.gperf"
      {"blt", MN_BLT},
#line 70 "src/gperf/perfect_hash.gperf"
      {"bllt", MN_BLLT},
      {""}, {""}, {""},
#line 55 "src/gperf/perfect_hash.gperf"
      {"sra", MN_SRA},
#line 58 "src/gperf/perfect_hash.gperf"
      {"srak", MN_SRAK},
      {""},
#line 16 "src/gperf/perfect_hash.gperf"
      {"half", MN_HALF},
#line 31 "src/gperf/perfect_hash.gperf"
      {"syscall", MN_SYSCALL},
#line 36 "src/gperf/perfect_hash.gperf"
      {"adc", MN_ADC},
#line 43 "src/gperf/perfect_hash.gperf"
      {"adck", MN_ADCK},
      {""}, {""}, {""},
#line 35 "src/gperf/perfect_hash.gperf"
      {"add", MN_ADD},
#line 42 "src/gperf/perfect_hash.gperf"
      {"addk", MN_ADDK},
      {""}, {""}, {""},
#line 39 "src/gperf/perfect_hash.gperf"
      {"and", MN_AND},
#line 46 "src/gperf/perfect_hash.gperf"
      {"andk", MN_ANDK},
#line 19 "src/gperf/perfect_hash.gperf"
      {"align", MN_ALIGN},
      {""}, {""},
#line 66 "src/gperf/perfect_hash.gperf"
      {"bmi", MN_BMI},
#line 37 "src/gperf/perfect_hash.gperf"
      {"sub", MN_SUB},
#line 44 "src/gperf/perfect_hash.gperf"
      {"subk", MN_SUBK},
      {""}, {""},
#line 28 "src/gperf/perfect_hash.gperf"
      {"mvi", MN_MVI},
#line 68 "src/gperf/perfect_hash.gperf"
      {"bleq", MN_BLEQ},
      {""}, {""}, {""},
#line 51 "src/gperf/perfect_hash.gperf"
      {"cmp", MN_CMP},
#line 69 "src/gperf/perfect_hash.gperf"
      {"blne", MN_BLNE},
      {""}, {""}, {""},
#line 29 "src/gperf/perfect_hash.gperf"
      {"lsr", MN_LSR},
#line 17 "src/gperf/perfect_hash.gperf"
      {"byte", MN_BYTE},
#line 18 "src/gperf/perfect_hash.gperf"
      {"ascii", MN_ASCII},
      {""}, {""},
#line 30 "src/gperf/perfect_hash.gperf"
      {"ssr", MN_SSR},
#line 60 "src/gperf/perfect_hash.gperf"
      {"beq", MN_BEQ},
      {""}, {""}, {""},
#line 49 "src/gperf/perfect_hash.gperf"
      {"tst", MN_TST},
      {""}, {""}, {""}, {""},
#line 52 "src/gperf/perfect_hash.gperf"
      {"cpn", MN_CPN},
#line 33 "src/gperf/perfect_hash.gperf"
      {"mov", MN_MOV},
      {""}, {""}, {""},
#line 32 "src/gperf/perfect_hash.gperf"
      {"nop", MN_NOP},
      {""}, {""}, {""}, {""},
#line 34 "src/gperf/perfect_hash.gperf"
      {"not", MN_NOT},
#line 14 "src/gperf/perfect_hash.gperf"
      {"addr", MN_ADDR},
      {""}, {""}, {""},
#line 41 "src/gperf/perfect_hash.gperf"
      {"xor", MN_XOR},
#line 48 "src/gperf/perfect_hash.gperf"
      {"xork", MN_XORK},
      {""}, {""}, {""}, {""},
#line 50 "src/gperf/perfect_hash.gperf"
      {"teq", MN_TEQ},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 15 "src/gperf/perfect_hash.gperf"
      {"word", MN_WORD}
    };
#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 6) > 4) || (defined __clang__ && __clang_major__ >= 3)
#pragma GCC diagnostic pop
#endif

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
            return &wordlist[key];
        }
    }
  return (struct MnemonicToken *) 0;
}
#line 75 "src/gperf/perfect_hash.gperf"

Mnemonic parse_mnemonic(StringSlice token) {
    struct MnemonicToken* res = in_word_set(token.bytes, token.length);
    return (res) ? res->mnemonic : MN_INVALID;
}
