#!/bin/sh
gperf -tc7 perfect_hash.gperf --output-file=perfect_hash.c
clang -O3 -g0 -c perfect_hash.c
ar -rcs perfect_hash.a perfect_hash.o
rm perfect_hash.o
