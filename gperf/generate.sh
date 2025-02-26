#!/bin/sh
gperf -tc7 parse_mnemonic.gperf --output-file=parse_mnemonic.c
clang -O3 -g0 -c parse_mnemonic.c
ar -rcs parse_mnemonic.a parse_mnemonic.o
rm parse_mnemonic.o
