/*
 * YF
 * com-test.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"
#include "error.h"

#undef YF_TESTLN
#define YF_TESTLN "..............................."

/* Error test. */
static int test_error(void) {
  printf("%s\n%.*s\n", __func__, (int)strlen(__func__), YF_TESTLN);

  int err;

  yf_seterr(YF_ERR_NOTFND, "test");
  err = yf_geterr();
  if (err != YF_ERR_NOTFND)
    return -1;

  err = yf_geterr();
  if (err != YF_ERR_NOTFND)
    return -1;

  yf_seterr(YF_ERR_LIMIT, NULL);
  if (err == yf_geterr())
    return -1;

  if (yf_geterr() != YF_ERR_LIMIT)
    return -1;

  yf_seterr(YF_ERR_OTHER, "TEST");
  err = yf_geterr();
  if (err != YF_ERR_OTHER)
    return -1;

  return 0;
}

/* Clock test. */
static int test_clock(void) {
  /* TODO */
  return 0;
}

/* List test. */
static int test_list(void) {
  /* TODO */
  return 0;
}

/* Hashset test. */
static int test_hashset(void) {
  /* TODO */
  return 0;
}

/* Test function. */
static int test(int argc, char *argv[]) {
  int (* const tests[])(void) = {
    test_error,
    test_clock,
    test_list,
    test_hashset
  };
  const size_t test_n = sizeof tests / sizeof tests[0];

  size_t results = 0;
  for (size_t i = 0; i < test_n; ++i)
    results += tests[i]() == 0;

  printf("DONE!\n\nNumber of tests executed: %lu\n", test_n);
  printf("> #%lu passed\n", results);
  printf("> #%lu failed\n", test_n - results);
  printf("\n(%.0f%% coverage)\n", (double)results / (double)test_n * 100.0);

  return 0;
}

const YF_test yf_g_test = {"com", test};
