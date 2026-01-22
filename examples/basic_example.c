#define LS_ARGS_IMPLEMENTATION
#include "ls_args.h"

int main(int argc, char** argv) {
    ls_args args;
    int help = 0;
    const char* outfile = "out.txt";

    ls_args_init(&args);
    ls_args_bool(&args, &help, "h", "help", "Prints help", 0);
    ls_args_string(&args, &outfile, "o", "out", "Specify the outfile, default 'out.txt'", 0);
    if (!ls_args_parse(&args, argc, argv)) {
        printf("Error: %s\n%s\n", args.last_error, ls_args_help(&args));
    }
    ls_args_free(&args);
    return 0;
}
