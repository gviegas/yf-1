/*
 * YF
 * core-test.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "test.h"

#define YF_TEST_ALL "all"
#define YF_TEST_SUBL "................................"
#define YF_TEST_SUBT \
  printf("%s\n%.*s\n", __func__, (int)strlen(__func__), YF_TEST_SUBL)

/* Info test. */
#define YF_TEST_CAPAB "capab"
int test_capab0(void);
static int test_capab(void) {
  YF_TEST_SUBT;
  puts("");
  int r = test_capab0();
  puts("");
  return r;
}

/* Test function. */
static int test(int argc, char *argv[]) {
  assert(argc > 0);
  size_t test_n;
  size_t results;

  if (strcmp(argv[0], YF_TEST_CAPAB) == 0) {
    test_n = 1;
    results = test_capab() == 0;
  } else {
    fprintf(stderr, "! No test named '%s'. Try:\n%s\n", argv[0], YF_TEST_CAPAB);
    return -1;
  }

  printf("\nDONE!\n\nNumber of tests executed: %lu\n", test_n);
  printf("> #%lu passed\n", results);
  printf("> #%lu failed\n", test_n - results);
  printf("\n(%.0f%% coverage)\n",(double)results / (double)test_n * 100.0);

  return 0;
}

const YF_test yf_g_test = {"core", test};
