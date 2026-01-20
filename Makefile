# CAUTION: This Makefile builds ONLY the tests.
# To use this library, see ls_test.h or README.md.

tests/tests: ls_args.o tests/tests.c tests/ls_test.h
	$(CC) -o $@ ls_args.o tests/tests.c -Itests -I. -ggdb

# Usually you wouldn't do this, but for tests we want this compiled with the
# most pedantic settings.
# Dont use this.
ls_args.o: ls_args.h
	$(CC) -c -x c -o $@ $^ -Wall -Wextra -Wpedantic -Werror -std=c89 -ggdb \
    	-Wno-error=pragma-once-outside-header \
        -I. \
        -DLS_ARGS_IMPLEMENTATION \
    	-Wno-pragma-once-outside-header

.PHONY: clean

clean:
	rm -f tests/tests
	rm -f ls_args.o
