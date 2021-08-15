/*
 * YF
 * com-test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "test.h"
#include "yf-com.h"

#define YF_TEST_ALL "all"
#define YF_TEST_SUBLN "..............................."
#define YF_TEST_SUBT \
    printf("%s\n%.*s\n", __func__, (int)strlen(__func__), YF_TEST_SUBLN)

/* Error test. */
#define YF_TEST_ERROR "error"

int yf_test_error(void);

static int test_error(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_error();
    puts("");
    return r;
}

/* Clock test. */
#define YF_TEST_CLOCK "clock"

int yf_test_clock(void);

static int test_clock(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_clock();
    puts("");
    return r;
}

/* List test. */
#define YF_TEST_LIST "list"

int yf_test_list(void);

static int test_list(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_list();
    puts("");
    return r;
}

/* Dictionary test. */
#define YF_TEST_DICT "dict"

int yf_test_dict(void);

static int test_dict(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_dict();
    puts("");
    return r;
}

/* Publish-Subscribe test. */
#define YF_TEST_PUBSUB "pubsub"

int yf_test_pubsub(void);

static int test_pubsub(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_pubsub();
    puts("");
    return r;
}

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
