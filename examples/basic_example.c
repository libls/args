#define LS_ARGS_IMPLEMENTATION
#include "ls_args.h"

int main(int argc, char** argv) {
    ls_args args;
    int help = 0;
    const char* outfile = "out.txt";
    const char* infile;
    const char* testfile;

    ls_args_init(&args);
    args.help_description = "An example program to show how arguments work. "
                            "Provide an input file and optionally an output "
                            "file and test file and see what happens!";
    ls_args_bool(&args, &help, "h", "help", "Prints help", 0);
    ls_args_string(&args, &outfile, "o", "out",
        "Specify the outfile, default 'out.txt'", 0);
    ls_args_pos_string(&args, &infile, "input file", LS_ARGS_REQUIRED);
    ls_args_pos_string(&args, &testfile, "test file", 0);
    if (!ls_args_parse(&args, argc, argv)) {
        if (help) {
            puts(ls_args_help(&args));
        } else {
            printf("Error: %s\n", args.last_error);
        }
        ls_args_free(&args);
        return 1;
    }

    printf("Got input file: %s\n", infile);
    printf("Got output file: %s\n", outfile);
    printf("Got test file: %s\n", testfile);

    ls_args_free(&args);
    return 0;
}
