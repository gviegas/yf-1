/*
 * YF
 * test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

#undef YF_SUBTITLE
#define YF_SUBTITLE(id) do { \
    const size_t sz = strlen(id)+2+1; \
    char ln[sz]; \
    memset(ln, ':', sz); \
    ln[sz-1] = '\0'; \
    printf("\n%s\n[%s]\n%s\n\n", ln, id, ln); } while (0)

int main(int argc, char *argv[])
{
    char line[80+1];
    memset(line, '#', sizeof line - 1);
    line[sizeof line - 1] = '\0';

    printf("%s\n[YF][%s] - Test\n%s\n", line, yf_g_test.name, line);

    size_t test_n = 0;
    size_t results = 0;

    if (argc == 1) {
        printf("! Error: missing TEST_ID\n"
               "! Usage: %s TEST_ID\n"
               "\nPossible values for TEST_ID:\n", argv[0]);

        for (size_t i = 0; i < yf_g_test.n; i++)
            printf("* %s\n", yf_g_test.ids[i]);

    } else {
        if (strcmp(argv[1], YF_TEST_ALL) == 0) {
            test_n = yf_g_test.n;
            for (size_t i = 0; i < yf_g_test.n; i++) {
                YF_SUBTITLE(yf_g_test.ids[i]);
                results += yf_g_test.fns[i]() == 0;
            }

        } else {
            for (size_t i = 0; i < yf_g_test.n; i++) {
                if (strcmp(argv[1], yf_g_test.ids[i]) == 0) {
                    test_n = 1;
                    YF_SUBTITLE(yf_g_test.ids[i]);
                    results = yf_g_test.fns[i]() == 0;
                    break;
                }
            }
            if (test_n == 0) {
                printf("! Error: unknown TEST_ID '%s'\n"
                       "\nPossible values for TEST_ID:\n", argv[1]);

                for (size_t i = 0; i < yf_g_test.n; i++)
                    printf("* %s\n", yf_g_test.ids[i]);
            }
        }
    }

    if (test_n == 0)
        puts("\n! No tests executed");
    else
        printf("\nDONE!\n"
               "\nNumber of tests executed: %zu\n"
               "> #%zu passed\n"
               "> #%zu failed\n"
               "\nCoverage: %.0f%%\n",
               test_n, results, test_n - results,
               (double)results / (double)test_n * 100.0);

    printf("\n%s\nEnd of test\n%s\n", line, line);
}
