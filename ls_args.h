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

/* Yes the naming is a little inconsistent, but "arg optional" reads better than
 * "args mode optional" */
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
    ls_args_arg* args;
    int args_len;
    int args_cap;
} ls_args;

void ls_args_init(ls_args*);

void ls_arg_bool(ls_args*, int* val, const char* short_opt,
    const char* long_opt, const char* help, ls_args_mode mode);
void ls_arg_string(ls_args*, const char** val, const char* short_opt,
    const char* long_opt, const char* help, ls_args_mode mode);

int ls_args_parse(ls_args*, int argc, char** argv);

char* ls_args_help(ls_args*);

void ls_args_free(ls_args*);

#ifdef LS_ARGS_IMPLEMENTATION

#include <assert.h>
#include <limits.h>
#include <string.h>

/* 0 on failure, 1 on success */
static int _lsa_add(ls_args* a, ls_args_arg** arg) {
    /* a is already checked when this is called */
    assert(arg != NULL);
    if (a->args_len + 1 > a->args_cap) {
        ls_args_arg* new_args;
        int new_cap = a->args_cap + a->args_cap / 2 + 8;
        if (new_cap > INT_MAX / (int)sizeof(*a->args)) {
            /* int overflow */
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

void ls_args_init(ls_args* a) { memset(a, 0, sizeof(*a)); }

void _lsa_register(ls_args* a, void* val, ls_args_type type,
    const char* short_opt, const char* long_opt, const char* help,
    ls_args_mode mode) {
    ls_args_arg* arg;
    assert(a != NULL);
    assert(val != NULL);
    /* only one can be NULL, not both, but neither have to be NULL */
    assert(short_opt != NULL || long_opt != NULL);
    /* if short_opt isn't null, it must be 1 char */
    assert(short_opt == NULL || strlen(short_opt) == 1);
    assert(_lsa_add(a, &arg));
    /* TODO: sanity check that there are no dashes in there, because that would
     * be a misuse of the API. */
    /* remove preceding dashes for later matching */
    while (*long_opt == '-')
        long_opt++;
    /* remove preceding dashes for later matching */
    while (*short_opt == '-')
        short_opt++;
    /* the rest may be NULL */
    arg->type = type;
    arg->short_opt = short_opt;
    arg->long_opt = long_opt;
    arg->help = help;
    arg->mode = mode;
    arg->val_ptr = val;
}

void ls_arg_bool(ls_args* a, int* val, const char* short_opt,
    const char* long_opt, const char* help, ls_args_mode mode) {
    _lsa_register(a, val, LS_ARGS_TYPE_BOOL, short_opt, long_opt, help, mode);
}

void ls_arg_string(ls_args* a, const char** val, const char* short_opt,
    const char* long_opt, const char* help, ls_args_mode mode) {
    _lsa_register(a, val, LS_ARGS_TYPE_STRING, short_opt, long_opt, help, mode);
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

_lsa_parsed _lsa_parse(const char* s) {
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
            size_t remaining = s_len - 1;
            if (remaining == 0) {
                /* shouldn't be possible due to earlier checks */
                res.type = LS_ARGS_PARSED_ERROR;
                res.as.erroneous = s;
                goto end;
            }
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

#include <stdio.h>

int ls_args_parse(ls_args* a, int argc, char** argv) {
    int i;
    int k;
    ls_args_arg* prev_arg = NULL;
    assert(a != NULL);
    assert(argv != NULL);
    for (i = 1; i < argc; ++i) {
        _lsa_parsed parsed = _lsa_parse(argv[i]);
        if (prev_arg) {
            if (parsed.type != LS_ARGS_PARSED_POSITIONAL) {
                /* argument for the previous param expected, but none given */
                /* TODO: Error properly */
                fprintf(stderr, "Expected argument for '--%s'\n",
                    prev_arg->long_opt);
                return 0;
            }
            switch (prev_arg->type) {
            case LS_ARGS_TYPE_BOOL:
                assert(!"UNREACHABLE");
                abort();
                break;
            case LS_ARGS_TYPE_STRING:
                *(const char**)prev_arg->val_ptr = parsed.as.positional;
                break;
            }
            prev_arg = NULL;
            continue;
        }
        switch (parsed.type) {
        case LS_ARGS_PARSED_ERROR:
            /* TODO: Return/print/save error */
            /* TODO: Error properly */
            fprintf(stderr, "Failed to parse '%s'\n", parsed.as.erroneous);
            return 0;
        case LS_ARGS_PARSED_LONG:
            for (k = 0; k < a->args_len; ++k) {
                if (a->args[k].long_opt != NULL
                    && strcmp(parsed.as.long_arg, a->args[k].long_opt) == 0) {
                    _lsa_apply(&a->args[k], &prev_arg);
                    break;
                }
            }
            break;
        case LS_ARGS_PARSED_SHORT: {
            const char* args = parsed.as.short_args;
            while (*args) {
                char arg = *args++;
                if (prev_arg) {
                    /* TODO: Error properly */
                    fprintf(stderr, "Expected argument for '-%s'\n",
                        prev_arg->short_opt);
                    return 0;
                }
                for (k = 0; k < a->args_len; ++k) {
                    const char* opt = a->args[k].short_opt;
                    if (opt != NULL && opt[0] == arg) {
                        _lsa_apply(&a->args[k], &prev_arg);
                        break;
                    }
                }
            }
            break;
        }
        case LS_ARGS_PARSED_STOP:
        case LS_ARGS_PARSED_POSITIONAL:
            assert(!"UNREACHABLE");
            break;
        }
    }
    return 1;
}

char* ls_args_help(ls_args* a) {
    (void)a;
    return "help!";
}

void ls_args_free(ls_args* a) {
    if (a) {
        LS_FREE(a->args);
        a->args = NULL;
        a->args_cap = 0;
        a->args_len = 0;
    }
}
#endif
