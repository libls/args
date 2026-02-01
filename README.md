# LS Args

Minimal, single-header command-line argument parser for C.

- ANSI C / C89
- Header-only
- No macros or code generation
- Extensively unit-tested (90%+ line- and branch coverage)
- Supports short/long options, booleans, strings, and positional arguments
- Supports short options as `-abc` equivalent to `-a -b -c`
- Optional/required argument modes
- Auto-generated help text
- Supports `--` to indicate that all following arguments should be treated as positional, even if they start with `-`

## Quick Start
1. Copy `ls_args.h` to your project.
2. Define and initialize an argument parser:
    ```c
    ls_args args;
    ls_args_init(&args);
    args.help_description = "My program description.";
    ```
3. Register arguments:
    ```c
    int verbose = 0;
    ls_args_bool(&args, &verbose, "v", "verbose", "Enable verbose output", 0);

    const char* output = NULL;
    ls_args_string(&args, &output, "o", "output", "Output file", 0);

    const char* input;
    ls_args_pos_string(&args, &input, "Input file", LS_ARGS_REQUIRED);
    ```
4. Parse arguments:
    ```c
    if (!ls_args_parse(&args, argc, argv)) {
        fprintf(stderr, "%s\n", args.last_error);
        exit(1);
    }
    ```
5. Use parsed values. Free resources when done:
    ```c
    ls_args_free(&args);
    ```
6. Use `ls_args_help()` to generate a help string from provided arguments:
    ```c
    puts(ls_args_help(&args));
    ```
    Example output:
    ```
    Usage: ./my_example [OPTION] <Input file>
    
    Options:
      -v 	--verbose   Enable verbose output
      -o 	--output    Output file
    ```

See [`ls_args.h`](ls_args.h) for detailed documentation and usage patterns.

## License

MIT.
