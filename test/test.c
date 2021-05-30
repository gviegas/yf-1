/*
 * YF
 * test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

#undef YF_LINESZ
#define YF_LINESZ 80

#undef YF_LINECHR
#define YF_LINECHR '%'

int main(int argc, char *argv[])
{
    char line[YF_LINESZ+1];
    memset(line, YF_LINECHR, YF_LINESZ);
    line[YF_LINESZ] = '\0';

    printf("%s\n[YF][%s] - Test\n%s\n\n", line, yf_g_test.name, line);

    if (argc == 1) {
        printf("! Error: missing TEST_ID\n");
        printf("! Usage: %s TEST_ID [parameters]\n", argv[0]);
        printf("\nPossible values for TEST_ID:\n");
        for (size_t i = 0; i < yf_g_test.id_n; i++)
            printf("* %s\n", yf_g_test.ids[i]);
        printf("\n! No tests executed\n");
    } else {
        yf_g_test.fn(argc-1, argv+1);
    }

    printf("\n%s\nEnd of test\n%s\n", line, line);
}
