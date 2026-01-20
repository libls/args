#define LS_TEST_IMPLEMENTATION
#include "ls_test.h"

#include "ls_args.h"

TEST_CASE(basic_args) {
    int help = 0;
    int test = 0;
    int no = 0;
    ls_args args;
    char* argv[] = { "./hello", "-h", "--test", NULL };
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    ls_args_init(&args);
    ls_arg_bool(&args, &help, "h", "help", "Provides help", 0);
    ls_arg_bool(&args, &test, "t", "test", "A test argument", 0);
    ls_arg_bool(&args, &no, "n", "nope", "An argument that isn't present", 0);
    ASSERT(ls_args_parse(&args, argc, argv));
    ASSERT_EQ(help, 1, "%d");
    ASSERT_EQ(test, 1, "%d");
    ASSERT_EQ(no, 0, "%d");
    ls_args_free(&args);
    return 0;
}


TEST_MAIN
