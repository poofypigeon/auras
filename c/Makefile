SHELL = /usr/bin/env bash

CFLAGS := -std=gnu23
WARN_FLAGS := -Wall -Werror -Wpedantic
TEST_FLAGS := -g

ALL_CHECK_SUITES := parsing string_to_int_map encode_instruction

ifdef SUITE
	SUITES_TO_RUN := $(SUITE)
else
	SUITES_TO_RUN := $(ALL_CHECK_SUITES)
endif

BINARIES_TO_RUN := $(foreach s, $(SUITES_TO_RUN), bin/check_$(s))

# make test [SUITE=<parsing|identifier_table>]
.PHONY: test
test: CFLAGS += $(TEST_FLAGS)
test: $(BINARIES_TO_RUN)
	@for binary in $(BINARIES_TO_RUN); do $$binary; done

.PHONY: errors
errors: CFLAGS += $(TEST_FLAGS)
errors: bin/error_messages
	bin/error_messages 2>&1 | less -R

STRING_TO_INT_MAP_DEPENDENCIES := test/check_string_to_int_map.c \
	build/string_slice.o \
	build/string_to_int_map.o \

PARSING_DEPENDENCIES := test/check_parsing.c \
	build/string_slice.o \
	build/parsing.o \
	build/string_to_int_map.o \

ENCODE_INSTRUCTION_DEPENDENCIES := test/check_encode_instruction.c \
	build/string_slice.o \
	build/string_to_int_map.o \
	build/parsing.o \
	build/perfect_hash.o \
	build/encode_instruction.o \

bin/check_string_to_int_map: $(STRING_TO_INT_MAP_DEPENDENCIES)
	$(CC) $(CFLAGS) $(shell pkg-config --cflags --libs check) $^ -o $@

bin/check_parsing: $(PARSING_DEPENDENCIES)
	$(CC) $(CFLAGS) $(shell pkg-config --cflags --libs check) $^ -o $@

bin/check_encode_instruction: $(ENCODE_INSTRUCTION_DEPENDENCIES)
	$(CC) $(CFLAGS) $(shell pkg-config --cflags --libs check) $^ -o $@

ERROR_MESSAGES_DEPENDENCIES := test/error_messages.c \
	build/string_slice.o \
	build/line_error.o \
	build/string_to_int_map.o \
	build/parsing.o \
	build/perfect_hash.o \
	build/encode_instruction.o \

bin/error_messages: $(ERROR_MESSAGES_DEPENDENCIES)
	$(CC) $(CFLAGS) $(shell pkg-config --cflags --libs check) $^ -o $@

build/perfect_hash.o: src/gperf/perfect_hash.c
	$(CC) -c $(CFLAGS) $(WARN_FLAGS) $< -o $@

src/gperf/perfect_hash.c: src/gperf/perfect_hash.gperf
	gperf -tc7 src/gperf/perfect_hash.gperf --output-file=src/gperf/perfect_hash.c
	
build/%.o: src/%.c
	$(CC) -c $(CFLAGS) $(WARN_FLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f build/*.o
	rm -rf bin/*
