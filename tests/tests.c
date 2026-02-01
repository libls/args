#include <limits.h>
#include <stdint.h>
#define LS_TEST_IMPLEMENTATION
#include "ls_test.h"

int fail_alloc_once = 0;
int alloc_limit = -1;

void* test_realloc(void* p, size_t size) {
    if (fail_alloc_once) {
        fail_alloc_once = 0;
        return NULL;
    }
    if (alloc_limit != -1 && (int)size > alloc_limit) {
        return NULL;
    }
    return realloc(p, size);
}

/* this is also set in the Makefile because of the way this is built for tests.
 * It requires a bit of magic because of the way we do things.
 */
#define LS_REALLOC test_realloc

#include "ls_args.h"

TEST_CASE(help_value_vs_bracket_value) {
    ls_args args;
    int help = 0;
    const char* outfile = "out.txt";
    const char* infile;
    const char* testfile = NULL;

    ls_args_init(&args);
    args.help_description = "An example program to show how arguments work. "
                            "Provide an input file and optionally an output "
                            "file and test file and see what happens!";
    ls_args_bool(&args, &help, "h", "help", "Prints help", 0);
    ls_args_string(&args, &outfile, "o", "out",
        "Specify the outfile, default 'out.txt'", 0);
    ls_args_pos_string(&args, &infile, "input file", LS_ARGS_REQUIRED);
    ls_args_pos_string(&args, &testfile, "test file", 0);

    const char* help_text = ls_args_help(&args);

    ASSERT(strstr(help_text, "<input file>") != NULL);

    ASSERT(strstr(help_text, "[test file]") != NULL);

    ASSERT(strstr(help_text, "-o") != NULL);
    ASSERT(strstr(help_text, "--out") != NULL);
    ASSERT(strstr(help_text, "[VALUE]") != NULL);

    ASSERT(strstr(help_text, "<VALUE>") == NULL);

    ls_args_free(&args);
    return 0;
}

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

TEST_CASE(basic_args_with_unused_positionals) {
    int help = 0;
    int test = 0;
    int no = 0;
    const char* unused = NULL;
    ls_args args;
    char* argv[] = { "./hello", "-h", "--test", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_pos_string(&args, &unused, "Not used", 0);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ls_args_pos_string(&args, &unused, "Not used", 0);
    ls_args_bool(&args, &test, "t", "test", "A test argument", 0);
    ls_args_pos_string(&args, &unused, "Not used", 0);
    ls_args_bool(&args, &no, "n", "nope", "An argument that isn't present", 0);
    ls_args_pos_string(&args, &unused, "Not used", 0);
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

TEST_CASE(help_output_no_options) {
    ls_args args;
    const char* infile = NULL;
    char* help_str;

    ls_args_init(&args);
    ls_args_pos_string(&args, &infile, "Input file", 0);

    help_str = ls_args_help(&args);
    ASSERT(help_str != NULL);
    ASSERT_STR_EQ(args.last_error, "Success");

    /* [OPTION] should NOT be present */
    ASSERT(!strstr(help_str, "[OPTION]"));

    /* "Options:" should NOT be present */
    ASSERT(!strstr(help_str, "Options:"));

    /* The positional argument should be present */
    ASSERT(
        strstr(help_str, "[Input file]") || strstr(help_str, "<Input file>"));

    /* Check that [VALUE] and <VALUE> are not present, since there are no value
     * options */
    ASSERT(strstr(help_str, "[VALUE]") == NULL);
    ASSERT(strstr(help_str, "<VALUE>") == NULL);

    ls_args_free(&args);
    return 0;
}

TEST_CASE(huge_description) {
    int help = 0;
    ls_args args;
    int i;
    char* help_str;
    /* Create a very large description string */
    enum { DESC_SIZE = 8192 };
    char* huge_desc = (char*)LS_REALLOC(NULL, DESC_SIZE + 1);
    ASSERT(huge_desc != NULL);
    for (i = 0; i < DESC_SIZE; ++i) {
        huge_desc[i] = 'A' + (i % 26);
    }
    huge_desc[DESC_SIZE] = '\0';

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", huge_desc, 0);

    help_str = ls_args_help(&args);
    ASSERT(help_str != NULL);
    ASSERT_STR_EQ(args.last_error, "Success");
    /* The huge description should appear in the help output */
    ASSERT(strstr(help_str, huge_desc));

    /* Check that [VALUE] and <VALUE> are not present, since there are no value
     * options */
    ASSERT(strstr(help_str, "[VALUE]") == NULL);
    ASSERT(strstr(help_str, "<VALUE>") == NULL);

    LS_FREE(huge_desc);
    ls_args_free(&args);
    return 0;
}

TEST_CASE(help_output_basic) {
    int help = 0;
    const char* infile = NULL;
    const char* outfile = "out.txt";
    ls_args args;
    char* help_str;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ls_args_string(&args, &outfile, "o", "out",
        "Specify the outfile, default 'out.txt'", 0);
    ls_args_pos_string(&args, &infile, "Input file", 0);

    help_str = ls_args_help(&args);
    ASSERT(help_str != NULL);
    ASSERT_STR_EQ(args.last_error, "Success");

    ASSERT(strstr(help_str, "-h"));
    ASSERT(strstr(help_str, "--help"));
    ASSERT(strstr(help_str, "-o"));
    ASSERT(strstr(help_str, "--out"));

    ASSERT(strstr(help_str, "Provides help"));
    ASSERT(strstr(help_str, "Specify the outfile"));

    ASSERT(strstr(help_str, "[OPTION]"));
    ASSERT(strstr(help_str, "[Input file]"));

    ASSERT(strstr(help_str, "default 'out.txt'"));

    ASSERT(strstr(help_str, "Input file"));

    ASSERT(strstr(help_str, "-h"));
    ASSERT(strstr(help_str, "--help"));
    ASSERT(strstr(help_str, "-o"));
    ASSERT(strstr(help_str, "--out"));

    /* Check that [VALUE] is present for value options, and <VALUE> is not */
    ASSERT(strstr(help_str, "[VALUE]") != NULL);
    ASSERT(strstr(help_str, "<VALUE>") == NULL);

    ls_args_free(&args);
    return 0;
}

TEST_CASE(help_output_basic_required_pos) {
    int help = 0;
    const char* infile = NULL;
    const char* outfile = "out.txt";
    ls_args args;
    char* help_str;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ls_args_string(&args, &outfile, "o", "out",
        "Specify the outfile, default 'out.txt'", 0);
    ls_args_pos_string(&args, &infile, "Input file", LS_ARGS_REQUIRED);

    help_str = ls_args_help(&args);
    ASSERT(help_str != NULL);
    ASSERT_STR_EQ(args.last_error, "Success");

    ASSERT(strstr(help_str, "-h"));
    ASSERT(strstr(help_str, "--help"));
    ASSERT(strstr(help_str, "-o"));
    ASSERT(strstr(help_str, "--out"));

    ASSERT(strstr(help_str, "Provides help"));
    ASSERT(strstr(help_str, "Specify the outfile"));

    ASSERT(strstr(help_str, "[OPTION]"));
    ASSERT(strstr(help_str, "<Input file>"));

    ASSERT(strstr(help_str, "default 'out.txt'"));

    ASSERT(strstr(help_str, "Input file"));

    ASSERT(strstr(help_str, "-h"));
    ASSERT(strstr(help_str, "--help"));
    ASSERT(strstr(help_str, "-o"));
    ASSERT(strstr(help_str, "--out"));

    /* Check that [VALUE] is present for value options, and <VALUE> is not */
    ASSERT(strstr(help_str, "[VALUE]") != NULL);
    ASSERT(strstr(help_str, "<VALUE>") == NULL);

    ls_args_free(&args);
    return 0;
}

TEST_CASE(help_alloc_limit_sweep) {
    int help = 0;
    int limit;
    const char* infile = NULL;
    const char* outfile = "out.txt";
    const char* reqfile = NULL;
    const char* reqopt = NULL;
    ls_args args;
    char* help_str = NULL;
    int succeeded = 0;

    ls_args_init(&args);
    args.help_description = "My description!";
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ls_args_string(&args, &outfile, "o", "out",
        "Specify the outfile, default 'out.txt'", 0);
    /* Add a required non-positional argument */
    ls_args_string(
        &args, &reqopt, "r", "reqopt", "A required option", LS_ARGS_REQUIRED);
    ls_args_pos_string(&args, &infile, "Input file", 0);
    /* Add a required positional argument */
    ls_args_pos_string(&args, &reqfile, "Required file", LS_ARGS_REQUIRED);

    /* Sweep alloc_limit from very small sizes upward to ensure all
     * allocation attempts inside ls_args_help are exercised.
     * For each limit, call ls_args_help multiple times to verify repeated
     * failures. */
    for (limit = 0; limit <= 8192 && !succeeded; ++limit) {

        /* First attempt */
        alloc_limit = limit;
        help_str = ls_args_help(&args);
        alloc_limit = -1;
        if (help_str == NULL || strcmp(args.last_error, "Success") != 0) {
            /* Expect allocation-related failure while we are below the needed
             * size */
            ASSERT_STR_EQ(args.last_error, "Allocation failure");
            ASSERT(help_str != NULL);
            ASSERT_STR_EQ(
                help_str, "Not enough memory available to generate help text.");

            /* Second attempt (repeated failure path) */
            alloc_limit = limit;
            help_str = ls_args_help(&args);
            alloc_limit = -1;
            ASSERT_STR_EQ(args.last_error, "Allocation failure");
            ASSERT(help_str != NULL);
            ASSERT_STR_EQ(
                help_str, "Not enough memory available to generate help text.");
        } else {
            /* Success achieved at this alloc_limit; verify content */
            succeeded = 1;
            ASSERT(help_str != NULL);
            ASSERT_STR_EQ(args.last_error, "Success");
            ASSERT(strstr(help_str, "Provides help"));
            ASSERT(strstr(help_str, "Specify the outfile"));
            ASSERT(strstr(help_str, "A required option"));
            ASSERT(strstr(help_str, "Input file"));
            ASSERT(strstr(help_str, "Required file"));
            ASSERT(strstr(help_str, "[VALUE]") != NULL);
            ASSERT(strstr(help_str, "<VALUE>") != NULL);
            /* Check that required positional is shown with <> */
            ASSERT(strstr(help_str, "<Required file>") != NULL);
            /* Check that required non-positional is shown with [VALUE] and
             * marked as required */
            ASSERT(strstr(help_str, "--reqopt"));
            ASSERT(strstr(help_str, "[VALUE]"));
        }
    }

    /* Ensure we eventually succeeded in generating help text */
    ASSERT(succeeded);

    ls_args_free(&args);
    return 0;
}

TEST_CASE(help_output_empty_description) {
    int help = 0;
    ls_args args;
    char* help_str;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "", 0);

    help_str = ls_args_help(&args);
    ASSERT(help_str != NULL);
    ASSERT_STR_EQ(args.last_error, "Success");

    /* Should show the option, but not crash or print garbage for description */
    ASSERT(strstr(help_str, "-h"));
    ASSERT(strstr(help_str, "--help"));
    /* Should not print any description after the option */
    /* Accept either a blank line or just the option itself */
    /* There should not be any non-space character after the option on its line
     */
    {
        const char* opt_line = strstr(help_str, "-h \t--help");
        ASSERT(opt_line != NULL);
        /* Find the end of the line */
        const char* end = strchr(opt_line, '\n');
        if (end) {
            /* Check that between the end of the option and the newline, only
             * spaces/tabs appear */
            const char* after = opt_line + strlen("-h \t--help");
            while (after < end && (*after == ' ' || *after == '\t'))
                ++after;
            ASSERT(after == end);
        }
    }

    /* Check that [VALUE] and <VALUE> are not present, since there are no value
     * options */
    ASSERT(strstr(help_str, "[VALUE]") == NULL);
    ASSERT(strstr(help_str, "<VALUE>") == NULL);

    ls_args_free(&args);
    return 0;
}

TEST_CASE(free_null) {
    /* don't crash */
    ls_args_free(NULL);
    return 0;
}

TEST_CASE(help_alloc_fail) {
    int help = 0;
    ls_args args;
    char* help_str;
    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    fail_alloc_once = 1;
    help_str = ls_args_help(&args);
    ASSERT_STR_EQ(args.last_error, "Allocation failure");
    ASSERT_STR_EQ(
        help_str, "Not enough memory available to generate help text.");
    ls_args_free(&args);
    return 0;
}

TEST_CASE(basic_args_required) {
    int help = 0;
    int test = 0;
    ls_args args;
    char* argv[] = { "./hello", "-h", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ls_args_bool(
        &args, &test, "t", "test", "A test argument", LS_ARGS_REQUIRED);
    ASSERT(!ls_args_parse(&args, argc, argv));
    ASSERT_STR_EQ(args.last_error, "Required argument '--test' not found");

    char* argv2[] = { "./hello", "-h", "-t", NULL };
    int argc2 = sizeof(argv2) / sizeof(*argv2) - 1;
    int ret = ls_args_parse(&args, argc2, argv2);
    ASSERT_STR_EQ(args.last_error, "Success");

    ls_args_free(&args);
    return 0;
}

TEST_CASE(basic_args_positional) {
    int help = 0;
    int test = 0;
    int no = 0;
    const char* input;
    const char* output;
    ls_args args;
    char* argv[] = { "./hello", "-h", "hi", "--test", "world", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ls_args_pos_string(&args, &input, "Input file", 0);
    ls_args_bool(&args, &test, "t", "test", "A test argument", 0);
    ls_args_bool(&args, &no, "n", "nope", "An argument that isn't present", 0);

    ls_args_pos_string(&args, &output, "Output file", 0);

    if (!ls_args_parse(&args, argc, argv)) {
        printf("Error: %s\n", args.last_error);
        ASSERT(!"ls_args_parse failed");
    }
    ASSERT_EQ(help, 1, "%d");
    ASSERT_EQ(test, 1, "%d");
    ASSERT_EQ(no, 0, "%d");
    ASSERT_STR_EQ(input, "hi");
    ASSERT_STR_EQ(output, "world");
    ls_args_free(&args);
    return 0;
}

TEST_CASE(too_many_positional_after_double_dash) {
    const char* first = NULL;
    const char* second = NULL;
    ls_args args;
    char* argv[] = { "./hello", "--", "one", "two", "three", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_pos_string(&args, &first, "First positional argument", 0);
    ls_args_pos_string(&args, &second, "Second positional argument", 0);

    ASSERT(!ls_args_parse(&args, argc, argv));
    ASSERT_STR_EQ(args.last_error, "Unexpected argument 'three'");
    ls_args_free(&args);
    return 0;
}

TEST_CASE(basic_args_positional_required_error) {
    const char* first;
    ls_args args;
    char* argv[] = { "./hello", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_pos_string(&args, &first, "first file", LS_ARGS_REQUIRED);

    ASSERT(!ls_args_parse(&args, argc, argv));

    ASSERT_STR_EQ(
        args.last_error, "Required argument 'first file' not provided");
    ls_args_free(&args);
    return 0;
}

TEST_CASE(positional_value_same_as_flag) {
    int help = 0;
    const char* outfile = "out.txt";
    const char* infile = NULL;
    const char* testfile = NULL;
    ls_args args;
    char* argv[] = { "./basic_example", "hello.txt", "-o", "bruh", "h", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Prints help", 0);
    ls_args_string(&args, &outfile, "o", "out",
        "Specify the outfile, default 'out.txt'", 0);
    ls_args_pos_string(&args, &infile, "Input file", LS_ARGS_REQUIRED);
    ls_args_pos_string(&args, &testfile, "Test file", 0);

    ASSERT(ls_args_parse(&args, argc, argv));
    ASSERT_STR_EQ(infile, "hello.txt");
    ASSERT_STR_EQ(outfile, "bruh");
    ASSERT_STR_EQ(testfile, "h");

    ls_args_free(&args);
    return 0;
}

TEST_CASE(two_positional_second_required) {
    const char* first = NULL;
    const char* second = NULL;
    ls_args args;
    char* argv[] = { "./hello", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_pos_string(&args, &first, "first", 0);
    ls_args_pos_string(&args, &second, "second", LS_ARGS_REQUIRED);

    ASSERT(!ls_args_parse(&args, argc, argv));
    ASSERT_STR_EQ(args.last_error, "Required argument 'second' not provided");

    ls_args_free(&args);
    return 0;
}

TEST_CASE(basic_args_positional_required) {
    const char* first;
    ls_args args;
    char* argv[] = { "./hello", "world", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_pos_string(&args, &first, "first file", LS_ARGS_REQUIRED);

    ASSERT(ls_args_parse(&args, argc, argv));

    ASSERT_STR_EQ(first, "world");
    ls_args_free(&args);
    return 0;
}

TEST_CASE(basic_args_positional_only_error) {
    int help = 0;
    ls_args args;
    char* argv[] = { "./hello", "world", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);

    ASSERT(!ls_args_parse(&args, argc, argv));

    ASSERT_STR_EQ(args.last_error, "Unexpected argument 'world'");
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
    ASSERT_STR_EQ(args.last_error, "Invalid argument '--test'");
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
    ASSERT_STR_EQ(args.last_error, "Expected argument following '--file'");
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
    ASSERT_STR_EQ(args.last_error, "Expected argument following '--file'");
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
    ASSERT_STR_EQ(args.last_error,
        "Expected argument following '-f', instead got another argument '-h'");
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
    ASSERT_STR_EQ(args.last_error, "Invalid argument '-'");
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
    ls_args_bool(
        &args, &no, "-n", "----nope", "An argument that isn't present", 0);
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
    fail_alloc_once = 1;
    /* if the allocation fails, this fails */
    ret = ls_args_bool(&args, &help, "h", "help", "Provides help", 0);

    ASSERT(!ret);
    ASSERT_STR_EQ(args.last_error, "Allocation failure");
    /* there is no documented error state for this; we simply fail to add the
     * argument? */
    ASSERT_EQ(args.args_len, 0, "%uz");
    ls_args_free(&args);
    return 0;
}

TEST_CASE(alloc_fail_pos_string) {
    const char* input = NULL;
    ls_args args;
    char* argv[] = { "./hello", "file.txt", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;
    int ret;

    ls_args_init(&args);
    ASSERT_EQ(args.args_len, 0, "%uz");
    /* deliberately fail the allocation here */
    fail_alloc_once = 1;
    /* if the allocation fails, this fails */
    ret = ls_args_pos_string(&args, &input, "Input file", 0);

    ASSERT(!ret);
    ASSERT_STR_EQ(args.last_error, "Allocation failure");
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
    ASSERT_STR_EQ(args.last_error, "Invalid argument ''");
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

TEST_CASE(parse_stop) {
    const char *first, *second;
    ls_args args;
    int help = 0;
    char* argv[] = { "./hello", "--help", "--", "-h", "--test", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_args_pos_string(&args, &first, "First positional argument", 0);
    ls_args_pos_string(&args, &second, "First positional argument", 0);
    ls_args_bool(&args, &help, "h", "help", "Provides help", 0);
    ASSERT(ls_args_parse(&args, argc, argv));
    ASSERT_STR_EQ(first, "-h");
    ASSERT_STR_EQ(second, "--test");
    ASSERT_EQ(help, 1, "%d");
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
    ASSERT_STR_EQ(args.last_error, "Invalid argument '-t'");
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
    ASSERT_STR_EQ(input, "file.txt");
    ASSERT_STR_EQ(output, "output.txt");
    ASSERT_EQ(verbose, 1, "%d");
    ls_args_free(&args);
    return 0;
}

TEST_MAIN
