#include <stdint.h>
#include <limits.h>
#define LS_TEST_IMPLEMENTATION
#include "ls_test.h"

int fail_alloc = 0;

void* test_realloc(void* p, size_t size) {
    if (fail_alloc) {
        return NULL;
    }
    return realloc(p, size);
}

/* this is also set in the Makefile because of the way this is built for tests.
 * It requires a bit of magic because of the way we do things.
 */
#define LS_REALLOC test_realloc

#include "ls_args.h"

TEST_CASE(basic_args) {
    int help = 0;
    int test = 0;
    int no = 0;
    ls_args args;
    char* argv[] = { "./hello", "-h", "--test", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ls_args_bool(&args, &test, "t", "test", "A test argument", 0);
    ls_args_bool(&args, &no, "n", "nope", "An argument that isn't present", 0);
    if (!ls_args_parse(&args, argc, argv)) {
        printf("Error: %s\n", args.last_error);
        ASSERT(!"ls_args_parse failed");
    }
    ASSERT_EQ(help, 1, "%d");
    ASSERT_EQ(test, 1, "%d");
    ASSERT_EQ(no, 0, "%d");
    ls_args_free(&args);
    return 0;
}

TEST_CASE(basic_args_only_short) {
    int help = 0;
    int test = 0;
    int no = 0;
    ls_args args;
    char* argv[] = { "./hello", "-h", "-t", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", NULL, "Provides help", 0);
    ls_args_bool(&args, &test, "t", NULL, "A test argument", 0);
    ls_args_bool(&args, &no, "n", NULL, "An argument that isn't present", 0);
    if (!ls_args_parse(&args, argc, argv)) {
        printf("Error: %s\n", args.last_error);
        ASSERT(!"ls_args_parse failed");
    }
    ASSERT_EQ(help, 1, "%d");
    ASSERT_EQ(test, 1, "%d");
    ASSERT_EQ(no, 0, "%d");
    ls_args_free(&args);
    return 0;
}

TEST_CASE(basic_args_only_long) {
    int help = 0;
    int test = 0;
    int no = 0;
    ls_args args;
    char* argv[] = { "./hello", "--help", "--test", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_bool(&args, &help, NULL, "help", "Provides help", 0);
    ls_args_bool(&args, &test, NULL, "test", "A test argument", 0);
    ls_args_bool(&args, &no, NULL, "nope", "An argument that isn't present", 0);
    if (!ls_args_parse(&args, argc, argv)) {
        printf("Error: %s\n", args.last_error);
        ASSERT(!"ls_args_parse failed");
    }
    ASSERT_EQ(help, 1, "%d");
    ASSERT_EQ(test, 1, "%d");
    ASSERT_EQ(no, 0, "%d");
    ls_args_free(&args);
    return 0;
}

TEST_CASE(basic_args_short_combined) {
    int help = 0;
    int test = 0;
    int no = 0;
    ls_args args;
    char* argv[] = { "./hello", "-ht", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ls_args_bool(&args, &test, "t", "test", "A test argument", 0);
    ls_args_bool(&args, &no, "n", "nope", "An argument that isn't present", 0);
    if (!ls_args_parse(&args, argc, argv)) {
        printf("Error: %s\n", args.last_error);
        ASSERT(!"ls_args_parse failed");
    }
    ASSERT_EQ(help, 1, "%d");
    ASSERT_EQ(test, 1, "%d");
    ASSERT_EQ(no, 0, "%d");
    ls_args_free(&args);
    return 0;
}

TEST_CASE(error_invalid_argument) {
    int help = 0;
    ls_args args;
    char* argv[] = { "./hello", "-h", "--test", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ASSERT(!ls_args_parse(&args, argc, argv));
    ASSERT(strcmp(args.last_error, "Invalid argument '--test'") == 0);
    ls_args_free(&args);
    return 0;
}

TEST_CASE(error_expected_argument) {
    const char* file = 0;
    ls_args args;
    int help;
    char* argv[] = { "./hello", "--file", "--help", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ls_args_string(&args, &file, "f", "file", "File to work on", 0);
    ASSERT(!ls_args_parse(&args, argc, argv));
    ASSERT(strcmp(args.last_error, "Expected argument following '--file'") == 0);
    ls_args_free(&args);
    return 0;
}

TEST_CASE(error_expected_argument_last_arg) {
    const char* file = 0;
    ls_args args;
    char* argv[] = { "./hello", "--file", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_string(&args, &file, "f", "file", "File to work on", 0);
    ASSERT(!ls_args_parse(&args, argc, argv));
    ASSERT(strcmp(args.last_error, "Expected argument following '--file'") == 0);
    ls_args_free(&args);
    return 0;
}

TEST_CASE(error_expected_argument_short_combined) {
    const char* file = 0;
    ls_args args;
    int help;
    char* argv[] = { "./hello", "-fh", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ls_args_string(&args, &file, "f", "file", "File to work on", 0);
    ASSERT(!ls_args_parse(&args, argc, argv));
    ASSERT(strcmp(args.last_error, "Expected argument following '-f', instead got another argument '-h'") == 0);
    ls_args_free(&args);
    return 0;
}

TEST_CASE(error_parse_fail) {
    int help = 0;
    ls_args args;
    char* argv[] = { "./hello", "-", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ASSERT(!ls_args_parse(&args, argc, argv));
    ASSERT(strcmp(args.last_error, "Invalid argument '-'") == 0);
    ls_args_free(&args);
    return 0;
}

TEST_CASE(overflow_args_cap) {
    int help = 0;
    int dummy = 0;
    ls_args args;
    int ret;

    ls_args_init(&args);
    /* First allocate one arg so the structure is properly initialized */
    ret = ls_args_bool(&args, &dummy, "d", "dummy", "Dummy argument", 0);
    ASSERT(ret);
    /* can't have more elements, not even one, at this count */
    args.args_len = SIZE_MAX / sizeof(ls_args_arg);
    args.args_cap = SIZE_MAX / sizeof(ls_args_arg);
    /* Try to add an argument, which should fail due to overflow */
    ret = ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ASSERT(!ret);
    ls_args_free(&args);
    return 0;
}

TEST_CASE(strip_dashes) {
    int help = 0;
    int test = 0;
    int no = 0;
    ls_args args;
    char* argv[] = { "./hello", "-h", "--test", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    /* the dashes are optional */
    ls_args_bool(&args, &help, "-h", "--help", "Provides help", 0);
    /* you can mix them */
    ls_args_bool(&args, &test, "t", "--test", "A test argument", 0);
    /* have as many as you want */
    ls_args_bool(&args, &no, "-n", "----nope", "An argument that isn't present", 0);
    if (!ls_args_parse(&args, argc, argv)) {
        printf("Error: %s\n", args.last_error);
        ASSERT(!"ls_args_parse failed");
    }
    ASSERT_EQ(help, 1, "%d");
    ASSERT_EQ(test, 1, "%d");
    ASSERT_EQ(no, 0, "%d");
    ls_args_free(&args);
    return 0;
}

TEST_CASE(alloc_fail) {
    int help = 0;
    ls_args args;
    char* argv[] = { "./hello", "-", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;
    int ret;

    ls_args_init(&args);
    ASSERT_EQ(args.args_len, 0, "%uz");
    /* deliberately fail the allocation here */
    fail_alloc = 1;
    /* if the allocation fails, this fails */
    ret = ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    fail_alloc = 0;
    ASSERT(!ret);
    ASSERT(strcmp(args.last_error, "Allocation failure") == 0);
    /* there is no documented error state for this; we simply fail to add the
     * argument? */
    ASSERT_EQ(args.args_len, 0, "%uz");
    ls_args_free(&args);
    return 0;
}

TEST_CASE(error_parse_fail_empty) {
    int help = 0;
    ls_args args;
    char* argv[] = { "./hello", "", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ASSERT(!ls_args_parse(&args, argc, argv));
    ASSERT(strcmp(args.last_error, "Invalid argument ''") == 0);
    ls_args_free(&args);
    return 0;
}

TEST_CASE(error_parse_ignore_double_dash) {
    int help = 0;
    ls_args args;
    char* argv[] = { "./hello", "--", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ASSERT(ls_args_parse(&args, argc, argv));
    ls_args_free(&args);
    return 0;
}

TEST_CASE(error_invalid_argument_short) {
    int help = 0;
    ls_args args;
    char* argv[] = { "./hello", "-h", "-t", "-h", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ASSERT(!ls_args_parse(&args, argc, argv));
    ASSERT(strcmp(args.last_error, "Invalid argument '-t'") == 0);
    ls_args_free(&args);
    return 0;
}

TEST_CASE(string_args) {
    const char* input = NULL;
    const char* output = NULL;
    int verbose = 0;
    ls_args args;
    char* argv[] = { "./program", "--input", "file.txt", "-o", "output.txt",
        "-v", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_string(&args, &input, "i", "input", "Input file path", 0);
    ls_args_string(&args, &output, "o", "output", "Output file path", 0);
    ls_args_bool(&args, &verbose, "v", "verbose", "Verbose output", 0);
    ASSERT(ls_args_parse(&args, argc, argv));
    ASSERT(strcmp(input, "file.txt") == 0);
    ASSERT(strcmp(output, "output.txt") == 0);
    ASSERT_EQ(verbose, 1, "%d");
    ls_args_free(&args);
    return 0;
}

TEST_MAIN
