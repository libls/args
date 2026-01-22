#pragma once

#include <stddef.h>

#ifndef LS_REALLOC
#include <stdlib.h>
#define LS_REALLOC realloc
#endif
#ifndef LS_FREE
#include <stdlib.h>
#define LS_FREE free
#endif

typedef enum ls_args_mode {
    LS_ARG_OPTIONAL = 0,
    LS_ARG_REQUIRED = 1
} ls_args_mode;

typedef enum ls_args_type {
    LS_ARGS_TYPE_BOOL = 0,
    LS_ARGS_TYPE_STRING = 1
} ls_args_type;

typedef struct ls_args_arg {
    const char* short_opt;
    const char* long_opt;
    const char* help;
    ls_args_type type;
    void* val_ptr;
    ls_args_mode mode;
} ls_args_arg;

typedef struct ls_args {
    /* The last error, if any. Might be dynamically allocated; if so, it's
     * free'd with `ls_args_free` automatically. Always a valid, printable
     * string. */
    char* last_error;

    /* don't use the following fields outside the library */
    ls_args_arg* args;
    size_t args_len;
    size_t args_cap;

    /* some bookkeeping -- these are used to free dynamically allocated memory
     * for help or errors cleanly on `ls_args_free`. */
    void* allocated_error;
    void* allocated_help;
} ls_args;

/* Zero-initializes the arguments, does not allocate */
void ls_args_init(ls_args*);

/* The following functions register arguments. Upon a call to `ls_args_parse`,
 * the given `val` parameter is filled. The `val` pointer must never be NULL.
 *
 * ONE of `short_opt` and `long_opt` may be NULL, if only a short- or long
 * version should exist. The `help` string may be null, but is used to construct
 * help with `ls_args_help`.
 *
 * In your short and long opts, you *can* have `-` or `--`, but you don't need
 * them and, really, you should not use them. For example "h" or "help" are
 * valid short- and long-opts respectively.
 *
 * You can call ls_args_* functions with the same `val` pointer, if multiple
 * arguments should affect the same memory, however the order of evaluation (and
 * thus the order of which they may overwrite each other) is the same order as
 * the registration.
 *
 * BE AWARE that, if an argument is not present, the corresponding `val` is NOT
 * touched. This means that, if you initialize a bool with `true` and then parse
 * the args, and the corresponding flag is not present, the flag will not be set
 * to `false` (it will instead stay untouched). This allows you to set defaults
 * for the case in which the argument isn't present.
 */

/* A "flag", aka a boolean argument. If the argument is present, `*val` is set
 * to 1, otherwise it's left untouched.
 * Can fail if the allocator fails. `args.last_error` is set on failure. */
int ls_args_bool(ls_args*, int* val, const char* short_opt,
    const char* long_opt, const char* help, ls_args_mode mode);
/* An argument which requires a string parameter, for example `--file
 * hello.txt`. Can fail if the allocator fails. `args.last_error` is set on
 * failure. */
int ls_args_string(ls_args*, const char** val, const char* short_opt,
    const char* long_opt, const char* help, ls_args_mode mode);

/* Does all the heavy lifting. Assumes that `argv` has `argc` elements. NULL
 * termination of the `argv` array doesn't matter, but null-termination of each
 * individual string is required of course.
 *
 * Returns 1 on success, 0 on failure (boolean behavior).
 * On failure, the `args.last_error` is set to a human-readable string. */
int ls_args_parse(ls_args* args, int argc, char** argv);

/* Same as args.last_error. */
char* ls_args_get_error(ls_args*);
/* Constructs a help message from the arguments registered on the args struct
 * via `ls_args_{bool, string, ...} functions.
 * The string is statically allocated and only valid until `ls_args_help` is
 * called again. */
char* ls_args_help(ls_args*);

/* Frees all memory allocated in the args. */
void ls_args_free(ls_args*);

/* Define this in exactly ONE source file, or in an object file compiled
 * separately with -DLS_ARGS_IMPLEMENTATION. */
#ifdef LS_ARGS_IMPLEMENTATION

#include <assert.h>
#include <stdint.h>
#include <stdio.h> /* for sprintf */
#include <string.h>

/* 0 on failure, 1 on success */
static int _lsa_add(ls_args* a, ls_args_arg** arg) {
    /* a is already checked when this is called */
    assert(arg != NULL);
    if (a->args_len + 1 > a->args_cap) {
        ls_args_arg* new_args;
        size_t new_cap = a->args_cap + a->args_cap / 2 + 8;

        size_t max_items = SIZE_MAX / sizeof(*a->args);
        if (new_cap > max_items) {
            /* would overflow size_t */
            return 0;
        }
        new_args = LS_REALLOC(a->args, new_cap * sizeof(*new_args));
        if (new_args == NULL) {
            /* allocation failure */
            return 0;
        }
        a->args_cap = new_cap;
        a->args = new_args;
    }
    *arg = &a->args[a->args_len++];
    return 1;
}

void ls_args_init(ls_args* a) {
    memset(a, 0, sizeof(*a));
    a->last_error = "Success";
}

int _lsa_register(ls_args* a, void* val, ls_args_type type,
    const char* short_opt, const char* long_opt, const char* help,
    ls_args_mode mode) {
    ls_args_arg* arg;
    int ret;
    assert(a != NULL);
    assert(val != NULL);
    /* only one can be NULL, not both, but neither have to be NULL */
    assert(short_opt != NULL || long_opt != NULL);
    /* remove preceding dashes for later matching */
    if (long_opt)
        while (*long_opt == '-')
            long_opt++;
    /* remove preceding dashes for later matching */
    if (short_opt)
        while (*short_opt == '-')
            short_opt++;
    /* if short_opt isn't null, it must be 1 char */
    assert(short_opt == NULL || strlen(short_opt) == 1);
    ret = _lsa_add(a, &arg);
    if (ret == 0) {
        a->last_error = "Allocation failure";
        return 0;
    }
    /* TODO: sanity check that there are no dashes in there, because that would
     * be a misuse of the API. */
    /* the rest may be NULL */
    arg->type = type;
    arg->short_opt = short_opt;
    arg->long_opt = long_opt;
    arg->help = help;
    arg->mode = mode;
    arg->val_ptr = val;
    return 1;
}

int ls_args_bool(ls_args* a, int* val, const char* short_opt,
    const char* long_opt, const char* help, ls_args_mode mode) {
    return _lsa_register(
        a, val, LS_ARGS_TYPE_BOOL, short_opt, long_opt, help, mode);
}

int ls_args_string(ls_args* a, const char** val, const char* short_opt,
    const char* long_opt, const char* help, ls_args_mode mode) {
    return _lsa_register(
        a, val, LS_ARGS_TYPE_STRING, short_opt, long_opt, help, mode);
}

typedef enum _lsa_parsed_type {
    LS_ARGS_PARSED_ERROR = 0,
    LS_ARGS_PARSED_LONG = 1,
    LS_ARGS_PARSED_SHORT = 2,
    LS_ARGS_PARSED_STOP = 3,
    LS_ARGS_PARSED_POSITIONAL = 4
} _lsa_parsed_type;

typedef struct _lsa_parsed {
    _lsa_parsed_type type;
    union {
        /* the full argument that caused the error */
        const char* erroneous;
        /* the long arg without the `--` */
        const char* long_arg;
        /* might be multiple, like for -abc it would be `abc` */
        const char* short_args;
        /* an argument provided without `--`, in full */
        const char* positional;
    } as;
} _lsa_parsed;

static _lsa_parsed _lsa_parse(const char* s) {
    size_t s_len = strlen(s);
    _lsa_parsed res;
    assert(s != NULL);
    /* empty string or `-` */
    if (s_len < 2) {
        res.type = LS_ARGS_PARSED_ERROR;
        res.as.erroneous = s;
        goto end;
    }
    if (s[0] == '-') {
        if (s[1] == '-') {
            /* long opt */
            size_t remaining = s_len - 2;
            if (remaining == 0) {
                /* special case where `--` is provided on its own to signal
                 * "everything after this is positional" */
                res.type = LS_ARGS_PARSED_STOP;
                goto end;
            }
            res.type = LS_ARGS_PARSED_LONG;
            res.as.long_arg = &s[2];
        } else {
            /* short opt */
            /* guaranteed to be the right size due to earlier checks */
            res.type = LS_ARGS_PARSED_SHORT;
            res.as.short_args = &s[1];
        }
    } else {
        res.type = LS_ARGS_PARSED_POSITIONAL;
        res.as.positional = s;
    }

end:
    return res;
}

void _lsa_apply(ls_args_arg* arg, ls_args_arg** prev_arg) {
    switch (arg->type) {
    case LS_ARGS_TYPE_BOOL:
        *(int*)arg->val_ptr = 1;
        *prev_arg = NULL;
        break;
    case LS_ARGS_TYPE_STRING:
        *prev_arg = arg;
        break;
    }
}

static int _lsa_parse_long(
    ls_args* a, _lsa_parsed* parsed, ls_args_arg** prev_arg) {
    int found = 0;
    size_t k;
    for (k = 0; k < a->args_len; ++k) {
        if (a->args[k].long_opt != NULL
            && strcmp(parsed->as.long_arg, a->args[k].long_opt) == 0) {
            _lsa_apply(&a->args[k], prev_arg);
            found = 1;
            break;
        }
    }
    if (!found) {
        const size_t len = 32 + strlen(parsed->as.erroneous);
        a->allocated_error = LS_REALLOC(a->allocated_error, len);
        memset(a->allocated_error, 0, len);
        sprintf(a->allocated_error, "Invalid argument '--%s'",
            parsed->as.erroneous);
        a->last_error = a->allocated_error;
        return 0;
    }
    return 1;
}

static int _lsa_parse_short(
    ls_args* a, _lsa_parsed* parsed, ls_args_arg** prev_arg) {
    const char* args = parsed->as.short_args;
    while (*args) {
        char arg = *args++;
        int found = 0;
        size_t k;
        if (*prev_arg) {
            const size_t len = 128 + strlen((*prev_arg)->short_opt);
            a->allocated_error = LS_REALLOC(a->allocated_error, len);
            memset(a->allocated_error, 0, len);
            sprintf(a->allocated_error,
                "Expected argument following '-%s', instead got another "
                "argument '-%c'",
                (*prev_arg)->short_opt, arg);
            a->last_error = a->allocated_error;
            return 0;
        }
        for (k = 0; k < a->args_len; ++k) {
            const char* opt = a->args[k].short_opt;
            if (opt != NULL && opt[0] == arg) {
                _lsa_apply(&a->args[k], prev_arg);
                found = 1;
                break;
            }
        }
        if (!found) {
            const size_t len = 32;
            a->allocated_error = LS_REALLOC(a->allocated_error, len);
            memset(a->allocated_error, 0, len);
            sprintf(a->allocated_error, "Invalid argument '-%c'", arg);
            a->last_error = a->allocated_error;
            return 0;
        }
    }
    return 1;
}

int ls_args_parse(ls_args* a, int argc, char** argv) {
    int i;
    ls_args_arg* prev_arg = NULL;
    assert(a != NULL);
    assert(argv != NULL);
    for (i = 1; i < argc; ++i) {
        _lsa_parsed parsed = _lsa_parse(argv[i]);
        if (prev_arg) {
            if (parsed.type != LS_ARGS_PARSED_POSITIONAL) {
                /* argument for the previous param expected, but none given */
                const size_t len = 64 + strlen(prev_arg->long_opt);
                a->allocated_error = LS_REALLOC(a->allocated_error, len);
                memset(a->allocated_error, 0, len);
                sprintf(a->allocated_error,
                    "Expected argument following '--%s'", prev_arg->long_opt);
                a->last_error = a->allocated_error;
                return 0;
            }
            if (prev_arg->type == LS_ARGS_TYPE_STRING) {
                *(const char**)prev_arg->val_ptr = parsed.as.positional;
            }
            prev_arg = NULL;
            continue;
        }
        switch (parsed.type) {
        case LS_ARGS_PARSED_ERROR: {
            const size_t len = 32 + strlen(parsed.as.erroneous);
            a->allocated_error = LS_REALLOC(a->allocated_error, len);
            memset(a->allocated_error, 0, len);
            sprintf(a->allocated_error, "Invalid argument '%s'",
                parsed.as.erroneous);
            a->last_error = a->allocated_error;
            return 0;
        }
        case LS_ARGS_PARSED_LONG: {
            if (!_lsa_parse_long(a, &parsed, &prev_arg)) {
                return 0;
            }
            break;
        }
        case LS_ARGS_PARSED_SHORT: {
            if (!_lsa_parse_short(a, &parsed, &prev_arg)) {
                return 0;
            }
            break;
        }
        case LS_ARGS_PARSED_STOP:
            /* TODO */
            break;
        case LS_ARGS_PARSED_POSITIONAL:
            assert(!"UNREACHABLE");
            break;
        }
    }
    if (prev_arg) {
        /* argument for the previous param expected, but none given */
        const size_t len = 64 + strlen(prev_arg->long_opt);
        a->allocated_error = LS_REALLOC(a->allocated_error, len);
        memset(a->allocated_error, 0, len);
        sprintf(a->allocated_error, "Expected argument following '--%s'",
            prev_arg->long_opt);
        a->last_error = a->allocated_error;
        return 0;
    }
    return 1;
}

char* ls_args_help(ls_args* a) {
    (void)a;
    return "help!";
}

char* ls_args_get_error(ls_args* a) { return a->last_error; }

void ls_args_free(ls_args* a) {
    if (a) {
        LS_FREE(a->args);
        a->args = NULL;
        a->args_cap = 0;
        a->args_len = 0;

        LS_FREE(a->allocated_error);
        a->allocated_error = NULL;
        a->last_error = "";
        LS_FREE(a->allocated_help);
        a->allocated_help = NULL;
    }
}
#endif
