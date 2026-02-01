/* Lion's Standard (LS) ANSI C commandline argument parser with included help
 * renderer.
 *
 * Version: 2.2
 * Website: https://libls.org
 * Repo: https://github.com/libls/args
 * SPDX-License-Identifier: MIT
 *
 * ==== TABLE OF CONTENTS ====
 *
 * 1. DESCRIPTION
 * 2. HOW TO USE
 * 3. LICENSE
 *
 * ==== 1. DESCRIPTION ====
 *
 * A simpe, terse, but complete args parser.
 *
 * Supports the following syntaxes:
 *
 * - Short options: `-h`, `-f filename`, `-abc` (equivalent to `-a -b -c`)
 * - Long options: `--help`, `--file filename`
 * - Stop signals: `--` (everything after this is positional arguments)
 * - Positional arguments: `input.txt output.txt`
 *
 * Includes a help renderer.
 *
 * ==== 2. HOW TO USE ====
 *
 * ls_args, like all LS libraries, is a header-only library in a single file.
 * To use it in your codebase, simply copy and paste it into your source tree,
 * in a place where includes are read from.
 *
 * Then include and use it.
 *
 * Define LS_ARGS_IMPLEMENTATION in exactly one source file before the include.
 *
 * Example:
 *
 * #include <ls_args.h>
 * // ...
 *     ls_args args;
 *     int help = 0;
 *     const char* outfile = "out.txt";
 *     const char* infile;
 *     const char* testfile;
 *
 *     ls_args_init(&args);
 *     args.help_description = "Some description"; // optional
 *     ls_args_bool(&args, &help, "h", "help", "Prints help", 0);
 *     ls_args_string(&args, &outfile, "o", "out",
 *         "Specify the outfile, default 'out.txt'", 0);
 *     ls_args_pos_string(&args, &infile, "input file", LS_ARGS_REQUIRED);
 *     ls_args_pos_string(&args, &testfile, "test file", 0);
 *     if (!ls_args_parse(&args, argc, argv)) {
 *         if (help) {
 *             puts(ls_args_help(&args));
 *         } else {
 *             printf("Error: %s\n", args.last_error);
 *         }
 *         ls_args_free(&args);
 *         return 1;
 *     }
 *
 *     // TODO: Do something here with your arguments!
 *
 *     ls_args_free(&args);
 *
 * ==== 3. LICENSE ====
 *
 * This file is provided under the MIT license. For commercial support and
 * maintenance, feel free to use the e-mail below to contact the author(s).
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2026 Lion Kortlepel <libls@kortlepel.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
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
    LS_ARGS_OPTIONAL = 0,
    LS_ARGS_REQUIRED = 1
} ls_args_mode;

typedef enum ls_args_type {
    LS_ARGS_TYPE_BOOL = 0,
    LS_ARGS_TYPE_STRING = 1
} ls_args_type;

typedef struct ls_args_arg {
    int is_pos;
    union {
        struct _lsa_spec {
            const char* short_opt;
            const char* long_opt;
        } name;
        unsigned pos;
    } match;
    const char* help;
    ls_args_type type;
    void* val_ptr;
    ls_args_mode mode;
    int found;
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

    /* program name -- this is set on ls_args_parse, but you can change it here
     * if you want to, for whatever reason. */
    const char* program_name;

    /* description rendered under the "usage" line in ls_args_help */
    const char* help_description;

    /* some bookkeeping -- these are used to free dynamically allocated memory
     * for help or errors cleanly on `ls_args_free`. */
    void* _allocated_error;
    void* _allocated_help;

    size_t _next_pos;
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
/* A positional argument
 *
 * ./hello -r hello1 -v -x hello2 --other-flag
 *            ^^^^^^       ^^^^^^
 *             n=0          n=1
 *
 * assuming that -r is a boolean flag (takes no arguments).
 *
 * Example 2:
 *
 * ./my-app --flag1 --flag2 --  --help.txt
 *                          ^^  ^^^^^^^^^^
 *                          |    n=0
 *                          |
 *                          "stop" indicator
 * everything after the `--` is a positional argument, for example a filename
 * which starts with a dash (like in this example).
 *
 * The first call to this function declares the argument for n=0, the next for
 * n=1, and so on.
 *
 * If the first positional isn't LS_ARGS_REQUIRED, but the second is,
 * effectively both are required.
 */
int ls_args_pos_string(
    ls_args*, const char** val, const char* name, ls_args_mode mode);

/* Does all the heavy lifting. Assumes that `argv` has `argc` elements. NULL
 * termination of the `argv` array doesn't matter, but null-termination of each
 * individual string is required of course.
 *
 * Returns 1 on success, 0 on failure (boolean behavior).
 * On failure, the `args.last_error` is set to a human-readable string. */
int ls_args_parse(ls_args* args, int argc, char** argv);

/* Constructs a help message from the arguments registered on the args struct
 * via `ls_args_{bool, string, ...} functions.
 * The string is dynamically allocated using LS_REALLOC and is freed
 * automatically once ls_args_free() is called. The string may be
 * replaced/changed by the next invocation to this function, as the buffer is
 * reused. */
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
    arg->match.name.short_opt = short_opt;
    arg->match.name.long_opt = long_opt;
    arg->is_pos = 0;
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

int ls_args_pos_string(
    ls_args* a, const char** val, const char* name, ls_args_mode mode) {
    /* TODO: The semantics are unclear when the first arg is not required but
     * the second is. Effectively, the first becomes required, too, because the
     * second cannot be the second without the first. */
    ls_args_arg* arg;
    int ret;
    assert(a != NULL);
    assert(val != NULL);
    ret = _lsa_add(a, &arg);
    if (ret == 0) {
        a->last_error = "Allocation failure";
        return 0;
    }
    arg->type = LS_ARGS_TYPE_STRING;
    arg->match.pos = a->_next_pos++;
    arg->help = name;
    arg->mode = mode;
    arg->val_ptr = val;
    arg->is_pos = 1;
    return 1;
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
    if (s_len == 0 || (s_len == 1 && s[0] == '-')) {
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

static void _lsa_apply(ls_args_arg* arg, ls_args_arg** prev_arg) {
    arg->found = 1;
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
        if (a->args[k].is_pos) {
            continue;
        }
        if (a->args[k].match.name.long_opt != NULL
            && strcmp(parsed->as.long_arg, a->args[k].match.name.long_opt)
                == 0) {
            _lsa_apply(&a->args[k], prev_arg);
            found = 1;
            break;
        }
    }
    if (!found) {
        const size_t len = 32 + strlen(parsed->as.erroneous);
        a->_allocated_error = LS_REALLOC(a->_allocated_error, len);
        memset(a->_allocated_error, 0, len);
        sprintf(a->_allocated_error, "Invalid argument '--%s'",
            parsed->as.erroneous);
        a->last_error = a->_allocated_error;
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
            struct _lsa_spec named = (*prev_arg)->match.name;
            const size_t len = 128 + strlen(named.short_opt);
            a->_allocated_error = LS_REALLOC(a->_allocated_error, len);
            memset(a->_allocated_error, 0, len);
            sprintf(a->_allocated_error,
                "Expected argument following '-%s', instead got another "
                "argument '-%c'",
                named.short_opt, arg);
            a->last_error = a->_allocated_error;
            return 0;
        }
        for (k = 0; k < a->args_len; ++k) {
            const char* opt;
            if (a->args[k].is_pos) {
                continue;
            }
            opt = a->args[k].match.name.short_opt;
            if (opt != NULL && opt[0] == arg) {
                _lsa_apply(&a->args[k], prev_arg);
                found = 1;
                break;
            }
        }
        if (!found) {
            const size_t len = 32;
            a->_allocated_error = LS_REALLOC(a->_allocated_error, len);
            memset(a->_allocated_error, 0, len);
            sprintf(a->_allocated_error, "Invalid argument '-%c'", arg);
            a->last_error = a->_allocated_error;
            return 0;
        }
    }
    return 1;
}

static int _lsa_parse_positional(
    ls_args* a, _lsa_parsed* parsed, unsigned pos) {
    const size_t len = 32;
    size_t i;
    for (i = 0; i < a->args_len; ++i) {
        ls_args_arg* arg = &a->args[i];
        if (arg->is_pos && arg->match.pos == pos) {
            *(const char**)arg->val_ptr = parsed->as.positional;
            arg->found = 1;
            return 1;
        }
    }
    a->_allocated_error = LS_REALLOC(a->_allocated_error, len);
    memset(a->_allocated_error, 0, len);
    sprintf(
        a->_allocated_error, "Unexpected argument '%s'", parsed->as.positional);
    a->last_error = a->_allocated_error;
    return 0;
}

int ls_args_parse(ls_args* a, int argc, char** argv) {
    int i;
    unsigned pos_i = 0;
    ls_args_arg* prev_arg = NULL;
    assert(a != NULL);
    assert(argv != NULL);
    a->last_error = "Success";
    a->program_name = argv[0];
    /* set all args to not found in case this is called multiple times */
    for (i = 0; i < (int)a->args_len; ++i) {
        a->args[i].found = 0;
    }
    for (i = 1; i < argc; ++i) {
        _lsa_parsed parsed = _lsa_parse(argv[i]);
        if (prev_arg) {
            if (parsed.type != LS_ARGS_PARSED_POSITIONAL) {
                /* argument for the previous param expected, but none given */
                const size_t len = 64 + strlen(prev_arg->match.name.long_opt);
                a->_allocated_error = LS_REALLOC(a->_allocated_error, len);
                memset(a->_allocated_error, 0, len);
                sprintf(a->_allocated_error,
                    "Expected argument following '--%s'",
                    prev_arg->match.name.long_opt);
                a->last_error = a->_allocated_error;
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
            a->_allocated_error = LS_REALLOC(a->_allocated_error, len);
            memset(a->_allocated_error, 0, len);
            sprintf(a->_allocated_error, "Invalid argument '%s'",
                parsed.as.erroneous);
            a->last_error = a->_allocated_error;
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
        case LS_ARGS_PARSED_STOP: {
            i += 1;
            for (; i < argc; ++i) {
                _lsa_parsed parsed;
                parsed.type = LS_ARGS_PARSED_POSITIONAL;
                parsed.as.positional = argv[i];
                if (!_lsa_parse_positional(a, &parsed, pos_i)) {
                    return 0;
                }
                pos_i += 1;
            }
            /* that's all, no more parsing allowed */
            i = argc;
            break;
        }
        case LS_ARGS_PARSED_POSITIONAL:
            if (!_lsa_parse_positional(a, &parsed, pos_i)) {
                return 0;
            }
            ++pos_i;
            break;
        }
    }
    if (prev_arg) {
        size_t len;
        /* argument for the previous param expected, but none given */
        /* this can not be a positional argument, because in order to become a
         * prev_arg, it must have expected a value earlier. this is only the
         * case with -/--... arguments */
        assert(!prev_arg->is_pos);
        len = 64 + strlen(prev_arg->match.name.long_opt);
        a->_allocated_error = LS_REALLOC(a->_allocated_error, len);
        memset(a->_allocated_error, 0, len);
        sprintf(a->_allocated_error, "Expected argument following '--%s'",
            prev_arg->match.name.long_opt);
        a->last_error = a->_allocated_error;
        return 0;
    }

    for (i = 0; i < (int)a->args_len; ++i) {
        if (a->args[i].mode == LS_ARGS_REQUIRED && !a->args[i].found) {
            size_t len;
            if (a->args[i].is_pos) {
                len = 64;
                a->_allocated_error = LS_REALLOC(a->_allocated_error, len);
                memset(a->_allocated_error, 0, len);
                sprintf(a->_allocated_error,
                    "Required argument '%s' not provided", a->args[i].help);
            } else {
                len = 64 + strlen(a->args[i].match.name.long_opt);
                a->_allocated_error = LS_REALLOC(a->_allocated_error, len);
                memset(a->_allocated_error, 0, len);
                sprintf(a->_allocated_error,
                    "Required argument '--%s' not found",
                    a->args[i].match.name.long_opt);
            }

            a->last_error = a->_allocated_error;
            return 0;
        }
    }

    return 1;
}

typedef struct _lsa_buffer {
    char* data;
    size_t length;
    size_t capacity;
} _lsa_buffer;

static int _lsa_buffer_reserve(_lsa_buffer* buffer, size_t required_capacity) {
    char* new_data;

    if (required_capacity <= buffer->capacity)
        return 1;

    new_data = (char*)LS_REALLOC(buffer->data, required_capacity);
    if (!new_data)
        return 0;

    buffer->data = new_data;
    buffer->capacity = required_capacity;
    return 1;
}

static int _lsa_buffer_append_bytes(
    _lsa_buffer* buffer, const void* source, size_t byte_count) {
    if (!_lsa_buffer_reserve(buffer, buffer->length + byte_count + 1)) {
        return 0;
    }

    memcpy(buffer->data + buffer->length, source, byte_count);

    buffer->length += byte_count;
    buffer->data[buffer->length] = '\0';
    return 1;
}

static int _lsa_buffer_append_cstr(_lsa_buffer* buffer, const char* string) {
    return _lsa_buffer_append_bytes(buffer, string, strlen(string));
}

char* ls_args_help(ls_args* a) {
    _lsa_buffer help;
    if (a->_allocated_help != NULL) {
        LS_FREE(a->_allocated_help);
        a->_allocated_help = NULL;
    }
    help.data = NULL;
    help.length = 0;
    help.capacity = 0;

    if (!_lsa_buffer_append_cstr(&help, "Usage: ")) {
        goto alloc_fail;
    }
    if (a->program_name == NULL) {
        a->program_name = "<program>";
    }
    if (!_lsa_buffer_append_cstr(&help, a->program_name)) {
        goto alloc_fail;
    }
    if (a->args_len > 0) {
        size_t i;
        for (i = 0; i < a->args_len; ++i) {
            if (!a->args[i].is_pos) {
                if (!_lsa_buffer_append_cstr(&help, " [OPTION]")) {
                    goto alloc_fail;
                }
                break;
            }
        }
        for (i = 0; i < a->args_len; ++i) {
            if (a->args[i].is_pos) {
                const char* open
                    = a->args[i].mode == LS_ARGS_REQUIRED ? " <" : " [";
                const char* close
                    = a->args[i].mode == LS_ARGS_REQUIRED ? ">" : "]";
                if (!_lsa_buffer_append_cstr(&help, open))
                    goto alloc_fail;
                if (!_lsa_buffer_append_cstr(&help, a->args[i].help))
                    goto alloc_fail;
                if (!_lsa_buffer_append_cstr(&help, close))
                    goto alloc_fail;
            }
        }
        if (a->help_description) {
            if (!_lsa_buffer_append_cstr(&help, "\n\n"))
                goto alloc_fail;
            if (!_lsa_buffer_append_cstr(&help, a->help_description))
                goto alloc_fail;
        }
        if (!_lsa_buffer_append_cstr(&help, "\n\nOptions:"))
            goto alloc_fail;
        for (i = 0; i < a->args_len; ++i) {
            if (!a->args[i].is_pos) {
                if (!_lsa_buffer_append_cstr(&help, "\n  -"))
                    goto alloc_fail;
                if (!_lsa_buffer_append_cstr(
                        &help, a->args[i].match.name.short_opt))
                    goto alloc_fail;
                if (!_lsa_buffer_append_cstr(&help, " \t--"))
                    goto alloc_fail;
                if (!_lsa_buffer_append_cstr(
                        &help, a->args[i].match.name.long_opt))
                    goto alloc_fail;
                if (!_lsa_buffer_append_cstr(&help, " \t\t"))
                    goto alloc_fail;
                if (!_lsa_buffer_append_cstr(&help, a->args[i].help))
                    goto alloc_fail;
            }
        }
    }

    a->_allocated_help = help.data;
    a->last_error = "Success";
    return a->_allocated_help;
alloc_fail:
    a->_allocated_help = help.data;
    a->last_error = "Allocation failure";
    return "Not enough memory available to generate help text.";
}

void ls_args_free(ls_args* a) {
    if (a) {
        LS_FREE(a->args);
        a->args = NULL;
        a->args_cap = 0;
        a->args_len = 0;

        LS_FREE(a->_allocated_error);
        a->_allocated_error = NULL;
        a->last_error = "";
        LS_FREE(a->_allocated_help);
        a->_allocated_help = NULL;
    }
}
#endif
