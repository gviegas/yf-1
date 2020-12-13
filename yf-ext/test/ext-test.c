/*
 * YF
 * ext-test.c
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

/* Node test. */
#define YF_TEST_NODE "node"
int yf_test_node(void);
static int test_node(void) {
  YF_TEST_SUBT;
  puts("");
  int r = yf_test_node();
  puts("");
  return r;
}

/* Vector/Matrix test. */
#define YF_TEST_VECMAT "vecmat"
int yf_test_vecmat(void);
static int test_vecmat(void) {
  YF_TEST_SUBT;
  puts("");
  int r = yf_test_vecmat();
  puts("");
  return r;
}

/* Model test. */
#define YF_TEST_MODEL "model"
int yf_test_model(void);
static int test_model(void) {
  YF_TEST_SUBT;
  puts("");
  int r = yf_test_model();
  puts("");
  return r;
}

/* Test function. */
static int test(int argc, char *argv[]) {
  assert(argc > 0);
  size_t test_n;
  size_t results;

  if (strcmp(argv[0], YF_TEST_NODE) == 0) {
    test_n = 1;
    results = test_node() == 0;
  } else if (strcmp(argv[0], YF_TEST_VECMAT) == 0) {
    test_n = 1;
    results = test_vecmat() == 0;
  } else if(strcmp(argv[0], YF_TEST_MODEL) == 0) {
    test_n = 1;
    results = test_model() == 0;
  } else if (strcmp(argv[0], YF_TEST_ALL) == 0) {
    int (*const tests[])(void) = {test_node, test_vecmat, test_model};
    test_n = sizeof tests / sizeof tests[0];
    results = 0;
    for (size_t i = 0; i < test_n; ++i)
      results += tests[i]();
  } else {
    fprintf(stderr, "! No test named '%s'. Try:\n%s\n%s\n%s\n%s\n", argv[0],
        YF_TEST_NODE, YF_TEST_VECMAT, YF_TEST_MODEL, YF_TEST_ALL);
    return -1;
  }

  printf("\nDONE!\n\nNumber of tests executed: %lu\n", test_n);
  printf("> #%lu passed\n", results);
  printf("> #%lu failed\n", test_n - results);
  printf("\n(%.0f%% coverage)\n",(double)results / (double)test_n * 100.0);

  return 0;
}

const YF_test yf_g_test = {"ext", test};
