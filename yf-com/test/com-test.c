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
#include "clock.h"

#undef YF_TEST_LN
#define YF_TEST_LN "..............................."

#undef YF_TEST_SUBT
#define YF_TEST_SUBT \
  printf("%s\n%.*s\n", __func__, (int)strlen(__func__), YF_TEST_LN)

/* Error test. */
static int test_error(void) {
  YF_TEST_SUBT;

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
  YF_TEST_SUBT;

  double t1, t2;

  t1 = yf_gettime();
  printf("\ntime (1) is %f\n", t1);
  t2 = yf_gettime();
  printf("time (2) is %f\n", t2);
  printf("(%fs elapsed)\n", t2 - t1);

  double ts[] = {1.0, 1.5, 0.1, 0.01, 3.125};

  for (size_t i = 0; i < sizeof ts / sizeof ts[0]; ++i) {
    printf("\nsleeping for %fs...\n", ts[i]);
    t1 = yf_gettime();
    yf_sleep(ts[i]);
    printf("awake! (%fs elapsed)\n", yf_gettime()-t1);
  }

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

  printf("\nDONE!\n\nNumber of tests executed: %lu\n", test_n);
  printf("> #%lu passed\n", results);
  printf("> #%lu failed\n", test_n - results);
  printf("\n(%.0f%% coverage)\n", (double)results / (double)test_n * 100.0);

  return 0;
}

const YF_test yf_g_test = {"com", test};
