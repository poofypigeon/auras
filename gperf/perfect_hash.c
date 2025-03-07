/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -tc7 --output-file=perfect_hash.c perfect_hash.gperf  */
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

#line 1 "perfect_hash.gperf"

#include "stdint.h"
#include "stddef.h"
#include "string.h"

enum mnemonic_t {
    invalid,
    // Data arrays
    word, half, byte, ascii, align,
    // Instructions
    ld,   ldb,  ldh,  ldsb, ldsh,
    st,   stb,  sth,
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

#line 28 "perfect_hash.gperf"
struct mnemonic_token {
    char* name;
    enum mnemonic_t mnemonic;
};

#define TOTAL_KEYWORDS 77
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 5
#define MIN_HASH_VALUE 7
#define MAX_HASH_VALUE 294
/* maximum key range = 288, duplicates = 0 */

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
  static unsigned short asso_values[] =
    {
      295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
      295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
      295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
      295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
      295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
      295,  70, 295, 295, 295, 295, 295, 295, 295, 295,
      295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
      295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
      295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
      295, 295, 295, 295, 295, 295, 295,  65,  15,  30,
       10,  30, 110, 105,  70,  90,  85,  20,   0,  45,
       15,  50, 120,  85,  85,   5,   0,  55,   2, 115,
      100,  45, 295, 295, 295, 295, 295, 295, 295
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
#line 43 "perfect_hash.gperf"
      {"st",    st},
      {""}, {""}, {""}, {""},
#line 38 "perfect_hash.gperf"
      {"ld",    ld},
#line 71 "perfect_hash.gperf"
      {"lsr",   lsr},
      {""}, {""},
#line 77 "perfect_hash.gperf"
      {"b",     b},
#line 92 "perfect_hash.gperf"
      {"bl",    bl},
#line 87 "perfect_hash.gperf"
      {"bls",   bls},
      {""},
#line 84 "perfect_hash.gperf"
      {"bvs",   bvs},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 56 "perfect_hash.gperf"
      {"btc",   btc},
#line 41 "perfect_hash.gperf"
      {"ldsb",  ldsb},
#line 85 "perfect_hash.gperf"
      {"bvc",   bvc},
      {""}, {""},
#line 52 "perfect_hash.gperf"
      {"sbc",   sbc},
#line 95 "perfect_hash.gperf"
      {"blcs",  blcs},
      {""}, {""}, {""},
#line 44 "perfect_hash.gperf"
      {"stb",   stb},
      {""}, {""}, {""}, {""},
#line 39 "perfect_hash.gperf"
      {"ldb",   ldb},
      {""}, {""}, {""}, {""},
#line 80 "perfect_hash.gperf"
      {"bcs",   bcs},
#line 64 "perfect_hash.gperf"
      {"btck",  btck},
      {""}, {""}, {""},
#line 70 "perfect_hash.gperf"
      {"lsl",   lsl},
#line 60 "perfect_hash.gperf"
      {"sbck",  sbck},
      {""}, {""}, {""},
#line 81 "perfect_hash.gperf"
      {"bcc",   bcc},
#line 96 "perfect_hash.gperf"
      {"blcc",  blcc},
      {""}, {""}, {""},
#line 66 "perfect_hash.gperf"
      {"tst",   tst},
#line 104 "perfect_hash.gperf"
      {"bllt",  bllt},
      {""}, {""}, {""},
#line 48 "perfect_hash.gperf"
      {"sst",   sst},
#line 102 "perfect_hash.gperf"
      {"blls",  blls},
      {""}, {""}, {""},
#line 89 "perfect_hash.gperf"
      {"blt",   blt},
#line 73 "perfect_hash.gperf"
      {"lslk",  lslk},
      {""}, {""}, {""},
#line 72 "perfect_hash.gperf"
      {"asr",   asr},
      {""}, {""}, {""}, {""},
#line 47 "perfect_hash.gperf"
      {"scl",   scl},
#line 42 "perfect_hash.gperf"
      {"ldsh",  ldsh},
      {""}, {""}, {""},
#line 50 "perfect_hash.gperf"
      {"adc",   adc},
#line 105 "perfect_hash.gperf"
      {"blgt",  blgt},
      {""}, {""}, {""},
#line 51 "perfect_hash.gperf"
      {"sub",   sub},
#line 106 "perfect_hash.gperf"
      {"blle",  blle},
      {""}, {""}, {""},
#line 45 "perfect_hash.gperf"
      {"sth",   sth},
#line 94 "perfect_hash.gperf"
      {"blne",  blne},
      {""}, {""}, {""},
#line 40 "perfect_hash.gperf"
      {"ldh",   ldh},
#line 98 "perfect_hash.gperf"
      {"blpl",  blpl},
      {""}, {""}, {""},
#line 49 "perfect_hash.gperf"
      {"add",   add},
#line 58 "perfect_hash.gperf"
      {"adck",  adck},
      {""}, {""}, {""},
#line 53 "perfect_hash.gperf"
      {"and",   and},
#line 59 "perfect_hash.gperf"
      {"subk",  subk},
      {""}, {""}, {""},
#line 67 "perfect_hash.gperf"
      {"teq",   teq},
#line 103 "perfect_hash.gperf"
      {"blge",  blge},
      {""}, {""}, {""},
#line 75 "perfect_hash.gperf"
      {"not",   not},
#line 97 "perfect_hash.gperf"
      {"blmi",  blmi},
      {""}, {""}, {""},
#line 91 "perfect_hash.gperf"
      {"ble",   ble},
#line 57 "perfect_hash.gperf"
      {"addk",  addk},
      {""}, {""}, {""},
#line 78 "perfect_hash.gperf"
      {"beq",   beq},
#line 61 "perfect_hash.gperf"
      {"andk",  andk},
#line 107 "perfect_hash.gperf"
      {"mvi",   mvi},
      {""},
#line 54 "perfect_hash.gperf"
      {"or",    or},
#line 62 "perfect_hash.gperf"
      {"ork",   ork},
#line 99 "perfect_hash.gperf"
      {"blvs",  blvs},
      {""}, {""}, {""},
#line 79 "perfect_hash.gperf"
      {"bne",   bne},
#line 76 "perfect_hash.gperf"
      {"notk",  notk},
      {""}, {""}, {""},
#line 82 "perfect_hash.gperf"
      {"bmi",   bmi},
#line 35 "perfect_hash.gperf"
      {"byte",  byte},
      {""}, {""}, {""},
#line 65 "perfect_hash.gperf"
      {"nop",   nop},
      {""}, {""}, {""}, {""},
#line 55 "perfect_hash.gperf"
      {"xor",   xor},
      {""}, {""}, {""}, {""},
#line 68 "perfect_hash.gperf"
      {"cmp",   cmp},
#line 100 "perfect_hash.gperf"
      {"blvc",  blvc},
      {""}, {""}, {""},
#line 46 "perfect_hash.gperf"
      {"smv",   smv},
      {""}, {""}, {""}, {""},
#line 86 "perfect_hash.gperf"
      {"bhi",   bhi},
      {""},
#line 36 "perfect_hash.gperf"
      {"ascii", ascii},
      {""}, {""},
#line 90 "perfect_hash.gperf"
      {"bgt",   bgt},
#line 63 "perfect_hash.gperf"
      {"xork",  xork},
      {""}, {""}, {""},
#line 83 "perfect_hash.gperf"
      {"bpl",   bpl},
#line 33 "perfect_hash.gperf"
      {"word",  word},
      {""}, {""}, {""},
#line 109 "perfect_hash.gperf"
      {"m32",   m32},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 101 "perfect_hash.gperf"
      {"blhi",  blhi},
      {""}, {""}, {""},
#line 69 "perfect_hash.gperf"
      {"cpn",   cpn},
      {""}, {""}, {""}, {""},
#line 108 "perfect_hash.gperf"
      {"swi",   swi},
      {""}, {""}, {""}, {""},
#line 74 "perfect_hash.gperf"
      {"mov",   mov},
#line 93 "perfect_hash.gperf"
      {"bleq",  bleq},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 88 "perfect_hash.gperf"
      {"bge",   bge},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 37 "perfect_hash.gperf"
      {"align", align},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 34 "perfect_hash.gperf"
      {"half",  half}
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
#line 110 "perfect_hash.gperf"

int32_t parse_mnemonic(register const char* str, register size_t len) {
    struct mnemonic_token * res = in_word_set(str, len);
    return (res) ? (int32_t)res->mnemonic : (int32_t)invalid;
}
