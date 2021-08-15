/*
 * YF
 * com-test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "test.h"

#define YF_TEST_DEF(id) \
int yf_test_##id(void); \
static int test_##id(void) { puts("\n["#id"]\n"); return yf_test_##id(); }

#define YF_TEST_ALL "all"

/* Error test. */
#define YF_TEST_ERROR "error"
YF_TEST_DEF(error)

/* Clock test. */
#define YF_TEST_CLOCK "clock"
YF_TEST_DEF(clock)

/* List test. */
#define YF_TEST_LIST "list"
YF_TEST_DEF(list)

/* Dictionary test. */
#define YF_TEST_DICT "dict"
YF_TEST_DEF(dict)

/* Publish-Subscribe test. */
#define YF_TEST_PUBSUB "pubsub"
YF_TEST_DEF(pubsub)

static const char *l_ids[] = {
    YF_TEST_ERROR,
    YF_TEST_CLOCK,
    YF_TEST_LIST,
    YF_TEST_DICT,
    YF_TEST_PUBSUB,
    YF_TEST_ALL
};

/* Test function. */
static int test(int argc, char *argv[])
{
    assert(argc > 0);

    size_t test_n;
    size_t results;

    if (strcmp(argv[0], YF_TEST_ERROR) == 0) {
        test_n = 1;
        results = test_error() == 0;
    } else if (strcmp(argv[0], YF_TEST_CLOCK) == 0) {
        test_n = 1;
        results = test_clock() == 0;
    } else if (strcmp(argv[0], YF_TEST_LIST) == 0) {
        test_n = 1;
        results = test_list() == 0;
    } else if (strcmp(argv[0], YF_TEST_DICT) == 0) {
        test_n = 1;
        results = test_dict() == 0;
    } else if (strcmp(argv[0], YF_TEST_PUBSUB) == 0) {
        test_n = 1;
        results = test_pubsub() == 0;
    } else if (strcmp(argv[0], YF_TEST_ALL) == 0) {
        int (*const tests[])(void) = {
            test_error,
            test_clock,
            test_list,
            test_dict,
            test_pubsub
        };
        test_n = sizeof tests / sizeof tests[0];

        results = 0;
        for (size_t i = 0; i < test_n; i++)
            results += tests[i]() == 0;
    } else {
        printf("! Error: unknown TEST_ID '%s'\n", argv[0]);
        printf("\nPossible values for TEST_ID:\n");
        for (size_t i = 0; i < (sizeof l_ids / sizeof l_ids[0]); i++)
            printf("* %s\n", l_ids[i]);
        printf("\n! No tests executed\n");
        return -1;
    }

    printf("\nDONE!\n\nNumber of tests executed: %zu\n", test_n);
    printf("> #%zu passed\n", results);
    printf("> #%zu failed\n", test_n - results);
    printf("\n(%.0f%% coverage)\n", (double)results / (double)test_n * 100.0);

    return 0;
}

const YF_test yf_g_test = {"com", test, l_ids, sizeof l_ids / sizeof l_ids[0]};
