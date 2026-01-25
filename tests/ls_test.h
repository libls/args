/* Lion's Standard (LS) test harness.
 *
 * Version: 1.3
 * Website: https://libls.org
 * Repo: https://github.com/libls/test
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
 * This is a super simple, minimal unit-test harness. It has auto-registering
 * tests and some macros for easy usage.
 *
 * Compiles under ANSI C, the only special part is the extension __typeof__ if
 * you use asserts other than `ASSERT` (e.g. ASSERT_EQ), and the constructor
 * attribute __attribute__((destructor)) for automatic test registration.
 *
 * Supports string comparisons using ASSERT_STR_EQ and ASSERT_STR_NEQ.
 *
 * ==== 2. HOW TO USE ====
 *
 * 1. Copy this file into your project and include it:
 *
 *      #include "ls_test.h"
 *
 * 2. In ONE C file, define LS_TEST_IMPLEMENTATION before including:
 *
 *      #define LS_TEST_IMPLEMENTATION
 *      #include "ls_test.h"
 *
 * 3. Write tests as functions with no arguments/returns:
 *
 *      TEST_CASE(test_add) {
 *          ASSERT_EQ(add(1, 2), 3, "%d");
 *          return 0; // needed for --failfast
 *      }
 *
 *     Use unique names, avoid starting with ls_ or lst_.
 *
 * 4. Add a main:
 *
 *      TEST_MAIN
 *
 * 5. Compile and run. Use --help for options.
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
 * of this software and associated documentation files (the “Software”), to deal
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

#include <stdio.h>

#define TEST_MAIN                                                              \
    int main(int argc, char** argv) { return ls_test_main(argc, argv); }

#define TEST_CASE(name)                                                        \
    static int LS_CAT(lst_t_, name)(void);                                     \
    static void LS_CAT(lst_init_, name)(void) LS_CONSTRUCTOR;                  \
    static void LS_CAT(lst_init_, name)(void) {                                \
        lst_reg(LS_CAT(lst_t_, name));                                         \
    }                                                                          \
    static int LS_CAT(lst_t_, name)(void)

#define ASSERT(cond)                                                           \
    do {                                                                       \
        if (!(cond)) {                                                         \
            const char* _func = __func__;                                      \
            if (strncmp(_func, "lst_t_", 6) == 0)                              \
                _func += 6;                                                    \
            fprintf(stderr, "%s: FAILED: %s (%s:%d)\n", _func, #cond,          \
                __FILE__, __LINE__);                                           \
            ++lst_fail;                                                        \
            return 1;                                                          \
        }                                                                      \
    } while (0)

/* the following macros require __typeof__ */

#define ASSERT_EQ(a, b, fmt)                                                   \
    do {                                                                       \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        if (_a != _b) {                                                        \
            const char* _func = __func__;                                      \
            if (strncmp(_func, "lst_t_", 6) == 0)                              \
                _func += 6;                                                    \
            fprintf(stderr,                                                    \
                "%s: FAILED: %s == %s (actual: " fmt " != " fmt ") (%s:%d)\n", \
                _func, #a, #b, _a, _b, __FILE__, __LINE__);                    \
            ++lst_fail;                                                        \
            return 1;                                                          \
        }                                                                      \
    } while (0)

#define ASSERT_STR_EQ(a, b)                                                    \
    do {                                                                       \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        if (strcmp(_a, _b) != 0) {                                             \
            const char* _func = __func__;                                      \
            if (strncmp(_func, "lst_t_", 6) == 0)                              \
                _func += 6;                                                    \
            fprintf(stderr,                                                    \
                "%s: FAILED: %s == %s (actual: \"%s\" != \"%s\") (%s:%d)\n",   \
                _func, #a, #b, _a, _b, __FILE__, __LINE__);                    \
            ++lst_fail;                                                        \
            return 1;                                                          \
        }                                                                      \
    } while (0)

#define ASSERT_STR_NEQ(a, b)                                                    \
    do {                                                                       \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        if (strcmp(_a, _b) == 0) {                                             \
            const char* _func = __func__;                                      \
            if (strncmp(_func, "lst_t_", 6) == 0)                              \
                _func += 6;                                                    \
            fprintf(stderr,                                                    \
                "%s: FAILED: %s != %s (actual: \"%s\" == \"%s\") (%s:%d)\n",   \
                _func, #a, #b, _a, _b, __FILE__, __LINE__);                    \
            ++lst_fail;                                                        \
            return 1;                                                          \
        }                                                                      \
    } while (0)

#define ASSERT_NEQ(a, b, fmt)                                                  \
    do {                                                                       \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        if (_a == _b) {                                                        \
            const char* _func = __func__;                                      \
            if (strncmp(_func, "lst_t_", 6) == 0)                              \
                _func += 6;                                                    \
            fprintf(stderr,                                                    \
                "%s: FAILED: %s != %s (actual: " fmt " == " fmt ") (%s:%d)\n", \
                _func, #a, #b, _a, _b, __FILE__, __LINE__);                    \
            ++lst_fail;                                                        \
            return 1;                                                          \
        }                                                                      \
    } while (0)

#define ASSERT_LT(a, b, fmt)                                                   \
    do {                                                                       \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        if (!(_a < _b)) {                                                      \
            const char* _func = __func__;                                      \
            if (strncmp(_func, "lst_t_", 6) == 0)                              \
                _func += 6;                                                    \
            fprintf(stderr,                                                    \
                "%s: FAILED: %s < %s (actual: " fmt " >= " fmt ") (%s:%d)\n",  \
                _func, #a, #b, _a, _b, __FILE__, __LINE__);                    \
            ++lst_fail;                                                        \
            return 1;                                                          \
        }                                                                      \
    } while (0)

#define ASSERT_LE(a, b, fmt)                                                   \
    do {                                                                       \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        if (!(_a <= _b)) {                                                     \
            const char* _func = __func__;                                      \
            if (strncmp(_func, "lst_t_", 6) == 0)                              \
                _func += 6;                                                    \
            fprintf(stderr,                                                    \
                "%s: FAILED: %s <= %s (actual: " fmt " > " fmt ") (%s:%d)\n",  \
                _func, #a, #b, _a, _b, __FILE__, __LINE__);                    \
            ++lst_fail;                                                        \
            return 1;                                                          \
        }                                                                      \
    } while (0)

#define ASSERT_GT(a, b, fmt)                                                   \
    do {                                                                       \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        if (!(_a > _b)) {                                                      \
            const char* _func = __func__;                                      \
            if (strncmp(_func, "lst_t_", 6) == 0)                              \
                _func += 6;                                                    \
            fprintf(stderr,                                                    \
                "%s: FAILED: %s > %s (actual: " fmt " <= " fmt ") (%s:%d)\n",  \
                _func, #a, #b, _a, _b, __FILE__, __LINE__);                    \
            ++lst_fail;                                                        \
            return 1;                                                          \
        }                                                                      \
    } while (0)

#define ASSERT_GE(a, b, fmt)                                                   \
    do {                                                                       \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        if (!(_a >= _b)) {                                                     \
            const char* _func = __func__;                                      \
            if (strncmp(_func, "lst_t_", 6) == 0)                              \
                _func += 6;                                                    \
            fprintf(stderr,                                                    \
                "%s: FAILED: %s >= %s (actual: " fmt " < " fmt ") (%s:%d)\n",  \
                _func, #a, #b, _a, _b, __FILE__, __LINE__);                    \
            ++lst_fail;                                                        \
            return 1;                                                          \
        }                                                                      \
    } while (0)
#define LS_CAT2(a, b) a##b
#define LS_CAT(a, b) LS_CAT2(a, b)

#if defined(__GNUC__) || defined(__clang__)
#define LS_CONSTRUCTOR __attribute__((constructor))
#else
#error "Requires __attribute__((constructor)) support"
#endif

typedef int (*lst_func)(void);

extern lst_func* lst_funcs;
extern int lst_n;
extern int lst_cap;
extern int lst_fail;

void lst_reg(lst_func f);

#ifdef LS_TEST_IMPLEMENTATION
#include <stdlib.h>
#include <string.h>

lst_func* lst_funcs;
int lst_n;
int lst_cap;
int lst_fail = 0;

void lst_reg(lst_func f) {
    if (lst_n == lst_cap) {
        if (lst_cap == 0) {
            lst_cap = 8;
        } else {
            lst_cap *= 2;
        }
        lst_funcs = (lst_func*)realloc(lst_funcs, lst_cap * sizeof(*lst_funcs));
    }
    lst_funcs[lst_n++] = f;
}

#define HELP_STR                                                               \
    "Usage: %s [options]\n"                                                    \
    "Options:\n"                                                               \
    "  --failfast   Stop after the first failed test\n"                        \
    "  --help       Show this help message\n"

static int ls_test_main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    int failfast = 0;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--failfast") == 0) {
            failfast = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            fprintf(stderr, HELP_STR, argv[0]);
            return 0;
        } else {
            fprintf(
                stderr, "unknown argument: %s\n\n" HELP_STR, argv[i], argv[0]);
            return 1;
        }
    }

    for (i = 0; i < lst_n; ++i) {
        if (lst_funcs[i]() != 0 && failfast) {
            goto end;
        }
    }

end:
    fprintf(stderr, "%d succeeded, %d failed, %d total\n", lst_n - lst_fail,
        lst_fail, lst_n);
    free(lst_funcs);

    if (lst_fail > 0) {
        return 1;
    }

    return 0;
}
#endif
