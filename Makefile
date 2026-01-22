# CAUTION: This Makefile builds ONLY the tests.
# To use this library, see ls_test.h or README.md.

CFLAGS ?= -fsanitize=address,undefined
CFLAGS += -I.

all: tests/tests examples/basic_example

tests/tests: ls_args.o tests/tests.c tests/ls_test.h
	$(CC) -o $@ ls_args.o tests/tests.c -Itests -ggdb $(CFLAGS)

# Usually you wouldn't do this, but for tests we want this compiled with the
# most pedantic settings.
# Dont use this.
ls_args.o: ls_args.h
	echo -e "#include <stddef.h>\nvoid* test_realloc(void*, size_t);" >.test.h
	cat .test.h ls_args.h >.ls_args_test.c
	rm .test.h
	$(CC) -c -x c -o $@ .ls_args_test.c -Wall -Wextra -Wpedantic -Werror -std=c89 -ggdb \
    	-Wno-error=pragma-once-outside-header \
        -DLS_ARGS_IMPLEMENTATION \
        -DLS_REALLOC=test_realloc \
    	-Wno-pragma-once-outside-header \
        $(CFLAGS)
	rm .ls_args_test.c

.PHONY: clean

clean:
	rm -f tests/tests
	rm -f ls_args.o
