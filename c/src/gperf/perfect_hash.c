/* ANSI-C code produced by gperf version 3.3 */
/* Command-line: gperf -tc7 --output-file=src/gperf/perfect_hash.c src/gperf/perfect_hash.gperf  */
/* Computed positions: -k'1-2' */

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

#define TOTAL_KEYWORDS 15
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 5
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 18
/* maximum key range = 17, duplicates = 0 */

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
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19,  0, 10, 19,
       5, 19, 19, 19,  0, 19, 19, 19,  5,  0,
      19,  5, 19, 19, 19,  0, 19, 19,  0,  4,
      19,  0, 19, 19, 19, 19, 19, 19
    };
  return len + asso_values[(unsigned char)str[1]] + asso_values[(unsigned char)str[0]];
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
      {""}, {""},
#line 27 "src/gperf/perfect_hash.gperf"
      {"sh", MN_SH},
#line 28 "src/gperf/perfect_hash.gperf"
      {"mvi", MN_MVI},
#line 16 "src/gperf/perfect_hash.gperf"
      {"half", MN_HALF},
#line 18 "src/gperf/perfect_hash.gperf"
      {"ascii", MN_ASCII},
#line 25 "src/gperf/perfect_hash.gperf"
      {"sw", MN_SW},
#line 22 "src/gperf/perfect_hash.gperf"
      {"lh", MN_LH},
#line 24 "src/gperf/perfect_hash.gperf"
      {"lhu", MN_LHU},
#line 14 "src/gperf/perfect_hash.gperf"
      {"addr", MN_ADDR},
#line 19 "src/gperf/perfect_hash.gperf"
      {"align", MN_ALIGN},
#line 20 "src/gperf/perfect_hash.gperf"
      {"lw", MN_LW},
#line 26 "src/gperf/perfect_hash.gperf"
      {"sb", MN_SB},
#line 15 "src/gperf/perfect_hash.gperf"
      {"word", MN_WORD},
#line 17 "src/gperf/perfect_hash.gperf"
      {"byte", MN_BYTE},
      {""}, {""},
#line 21 "src/gperf/perfect_hash.gperf"
      {"lb", MN_LB},
#line 23 "src/gperf/perfect_hash.gperf"
      {"lbu", MN_LBU}
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
#line 29 "src/gperf/perfect_hash.gperf"

Mnemonic parse_mnemonic(StringSlice token) {
    struct MnemonicToken* res = in_word_set(token.bytes, token.length);
    return (res) ? res->mnemonic : MN_INVALID;
}
