/*
 * YF
 * wsys-test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-clock.h"

#include "test.h"
#include "yf-wsys.h"

#define YF_TEST_ALL "all"
#define YF_TEST_SUBL "................................"
#define YF_TEST_SUBT \
    printf("%s\n%.*s\n", __func__, (int)strlen(__func__), YF_TEST_SUBL)

/* Window test. */
#define YF_TEST_WINDOW "window"

int yf_test_window(void);

static int test_window(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_window();
    puts("");
    return r;
}

/* Event test. */
#define YF_TEST_EVENT "event"

int yf_test_event(void);

static int test_event(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_event();
    puts("");
    return r;
}

static const char *l_ids[] = {YF_TEST_WINDOW, YF_TEST_EVENT, YF_TEST_ALL};

/* Test function. */
static int test(int argc, char *argv[])
{
    assert(argc > 0);

    size_t test_n;
    size_t results;

    if (strcmp(argv[0], YF_TEST_WINDOW) == 0) {
        test_n = 1;
        results = test_window() == 0;
    } else if (strcmp(argv[0], YF_TEST_EVENT) == 0) {
        test_n = 1;
        results = test_event() == 0;
    } else if (strcmp(argv[0], YF_TEST_ALL) == 0) {
        int (*const tests[])(void) = {test_window, test_event};
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
    printf("\n(%.0f%% coverage)\n",(double)results / (double)test_n * 100.0);

    return 0;
}

const YF_test yf_g_test = {"wsys", test, l_ids, sizeof l_ids / sizeof l_ids[0]};
