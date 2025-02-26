/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -tc7 --output-file=parse_mnemonic.c parse_mnemonic.gperf  */
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

#line 1 "parse_mnemonic.gperf"

#include "stdint.h"
#include "stddef.h"
#include "string.h"

enum mnemonic_t {
    invalid,
    ld,   ldb,  ldh,  ldsb, ldsh,
    st,   stb,  sth,  stsb, stsh,
    smv,  scl,  sst,  
    add,  adc,  sub,  sbc,  and,  or,  xor,  btc, 
    addk, adck, subk, sbck, andk, ork, xork, btck,
    nop, 
    tst,  teq,  cmp,  cpn, 
    lsl,  lsr,  asr,  lslk,
    mov,  not,  notk, 
    b,    beq,  bne,  bcs,  bcc,  bmi,  bpl,  bvs,  bvc,  bhi,  bls,  bge,  blt,  bgt,  ble, 
    bl,   bleq, blne, blcs, blcc, blmi, blpl, blvs, blvc, blhi, blls, blge, bllt, blgt, blle,
    mvi,
    swi, 
    m32, 
};

#line 25 "parse_mnemonic.gperf"
struct mnemonic_token {
    char* name;
    enum mnemonic_t mnemonic;
};

#define TOTAL_KEYWORDS 74
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 4
#define MIN_HASH_VALUE 7
#define MAX_HASH_VALUE 209
/* maximum key range = 203, duplicates = 0 */

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
      210, 210, 210, 210, 210, 210, 210, 210, 210, 210,
      210, 210, 210, 210, 210, 210, 210, 210, 210, 210,
      210, 210, 210, 210, 210, 210, 210, 210, 210, 210,
      210, 210, 210, 210, 210, 210, 210, 210, 210, 210,
      210, 210, 210, 210, 210, 210, 210, 210, 210, 210,
      210,  65, 210, 210, 210, 210, 210, 210, 210, 210,
      210, 210, 210, 210, 210, 210, 210, 210, 210, 210,
      210, 210, 210, 210, 210, 210, 210, 210, 210, 210,
      210, 210, 210, 210, 210, 210, 210, 210, 210, 210,
      210, 210, 210, 210, 210, 210, 210,  65,  15,  30,
       10,  20,   7, 100,   0,  95,   2,  20,   0,  45,
       25,  40,  95,  75,  95,   5,   0,  55,  12,  75,
      100, 210, 210, 210, 210, 210, 210, 210, 210
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
        hval += asso_values[(unsigned char)str[2]+1];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

struct mnemonic_token *
in_word_set (register const char *str, register size_t len)
{
  static struct mnemonic_token wordlist[] =
    {
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 35 "parse_mnemonic.gperf"
      {"st",   st},
      {""},
#line 39 "parse_mnemonic.gperf"
      {"stsh", stsh},
      {""}, {""},
#line 30 "parse_mnemonic.gperf"
      {"ld",   ld},
#line 65 "parse_mnemonic.gperf"
      {"lsr",  lsr},
#line 34 "parse_mnemonic.gperf"
      {"ldsh", ldsh},
      {""},
#line 71 "parse_mnemonic.gperf"
      {"b",    b},
#line 86 "parse_mnemonic.gperf"
      {"bl",   bl},
#line 81 "parse_mnemonic.gperf"
      {"bls",  bls},
#line 99 "parse_mnemonic.gperf"
      {"blgt", blgt},
#line 80 "parse_mnemonic.gperf"
      {"bhi",  bhi},
      {""}, {""}, {""},
#line 38 "parse_mnemonic.gperf"
      {"stsb", stsb},
#line 85 "parse_mnemonic.gperf"
      {"ble",  ble},
      {""}, {""},
#line 50 "parse_mnemonic.gperf"
      {"btc",  btc},
#line 33 "parse_mnemonic.gperf"
      {"ldsb", ldsb},
#line 78 "parse_mnemonic.gperf"
      {"bvs",  bvs},
      {""}, {""},
#line 46 "parse_mnemonic.gperf"
      {"sbc",  sbc},
#line 89 "parse_mnemonic.gperf"
      {"blcs", blcs},
      {""}, {""}, {""},
#line 36 "parse_mnemonic.gperf"
      {"stb",  stb},
#line 97 "parse_mnemonic.gperf"
      {"blge", blge},
#line 79 "parse_mnemonic.gperf"
      {"bvc",  bvc},
      {""}, {""},
#line 31 "parse_mnemonic.gperf"
      {"ldb",  ldb},
      {""}, {""}, {""}, {""},
#line 74 "parse_mnemonic.gperf"
      {"bcs",  bcs},
#line 58 "parse_mnemonic.gperf"
      {"btck", btck},
#line 73 "parse_mnemonic.gperf"
      {"bne",  bne},
      {""}, {""},
#line 64 "parse_mnemonic.gperf"
      {"lsl",  lsl},
#line 54 "parse_mnemonic.gperf"
      {"sbck", sbck},
      {""}, {""}, {""},
#line 75 "parse_mnemonic.gperf"
      {"bcc",  bcc},
#line 90 "parse_mnemonic.gperf"
      {"blcc", blcc},
      {""}, {""},
#line 101 "parse_mnemonic.gperf"
      {"mvi",  mvi},
#line 60 "parse_mnemonic.gperf"
      {"tst",  tst},
#line 98 "parse_mnemonic.gperf"
      {"bllt", bllt},
#line 76 "parse_mnemonic.gperf"
      {"bmi",  bmi},
      {""}, {""},
#line 42 "parse_mnemonic.gperf"
      {"sst",  sst},
#line 96 "parse_mnemonic.gperf"
      {"blls", blls},
      {""}, {""}, {""},
#line 83 "parse_mnemonic.gperf"
      {"blt",  blt},
#line 67 "parse_mnemonic.gperf"
      {"lslk", lslk},
      {""}, {""}, {""},
#line 66 "parse_mnemonic.gperf"
      {"asr",  asr},
#line 88 "parse_mnemonic.gperf"
      {"blne", blne},
      {""}, {""}, {""},
#line 41 "parse_mnemonic.gperf"
      {"scl",  scl},
#line 100 "parse_mnemonic.gperf"
      {"blle", blle},
#line 102 "parse_mnemonic.gperf"
      {"swi",  swi},
      {""}, {""},
#line 44 "parse_mnemonic.gperf"
      {"adc",  adc},
      {""}, {""}, {""}, {""},
#line 45 "parse_mnemonic.gperf"
      {"sub",  sub},
#line 92 "parse_mnemonic.gperf"
      {"blpl", blpl},
      {""}, {""}, {""},
#line 43 "parse_mnemonic.gperf"
      {"add",  add},
#line 93 "parse_mnemonic.gperf"
      {"blvs", blvs},
      {""},
#line 87 "parse_mnemonic.gperf"
      {"bleq", bleq},
      {""},
#line 37 "parse_mnemonic.gperf"
      {"sth",  sth},
      {""}, {""}, {""}, {""},
#line 32 "parse_mnemonic.gperf"
      {"ldh",  ldh},
#line 52 "parse_mnemonic.gperf"
      {"adck", adck},
      {""}, {""}, {""},
#line 47 "parse_mnemonic.gperf"
      {"and",  and},
#line 53 "parse_mnemonic.gperf"
      {"subk", subk},
      {""}, {""}, {""},
#line 61 "parse_mnemonic.gperf"
      {"teq",  teq},
#line 51 "parse_mnemonic.gperf"
      {"addk", addk},
      {""}, {""}, {""},
#line 69 "parse_mnemonic.gperf"
      {"not",  not},
#line 94 "parse_mnemonic.gperf"
      {"blvc", blvc},
#line 82 "parse_mnemonic.gperf"
      {"bge",  bge},
      {""}, {""},
#line 40 "parse_mnemonic.gperf"
      {"smv",  smv},
      {""}, {""}, {""}, {""},
#line 72 "parse_mnemonic.gperf"
      {"beq",  beq},
#line 55 "parse_mnemonic.gperf"
      {"andk", andk},
      {""}, {""},
#line 48 "parse_mnemonic.gperf"
      {"or",   or},
#line 56 "parse_mnemonic.gperf"
      {"ork",  ork},
#line 91 "parse_mnemonic.gperf"
      {"blmi", blmi},
      {""}, {""}, {""},
#line 59 "parse_mnemonic.gperf"
      {"nop",  nop},
#line 70 "parse_mnemonic.gperf"
      {"notk", notk},
      {""}, {""}, {""},
#line 49 "parse_mnemonic.gperf"
      {"xor",  xor},
      {""}, {""}, {""}, {""},
#line 62 "parse_mnemonic.gperf"
      {"cmp",  cmp},
      {""}, {""}, {""}, {""},
#line 77 "parse_mnemonic.gperf"
      {"bpl",  bpl},
      {""}, {""}, {""}, {""},
#line 68 "parse_mnemonic.gperf"
      {"mov",  mov},
      {""}, {""}, {""}, {""},
#line 63 "parse_mnemonic.gperf"
      {"cpn",  cpn},
#line 57 "parse_mnemonic.gperf"
      {"xork", xork},
      {""}, {""}, {""},
#line 84 "parse_mnemonic.gperf"
      {"bgt",  bgt},
      {""}, {""}, {""}, {""},
#line 103 "parse_mnemonic.gperf"
      {"m32",  m32},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""},
#line 95 "parse_mnemonic.gperf"
      {"blhi", blhi}
    };

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
  return 0;
}
#line 104 "parse_mnemonic.gperf"

int32_t parse_mnemonic(register const char* str, register size_t len) {
    struct mnemonic_token * res = in_word_set(str, len);
    return (res) ? (int32_t)res->mnemonic : (int32_t)invalid;
}
