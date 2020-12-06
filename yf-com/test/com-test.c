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
#include "list.h"

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

  puts("");
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

  puts("");
  return 0;
}

/* List test. */
static int list_cb(void *val, void *arg) {
  printf("%c,%p ", *(int *)val, arg);
  return 0;
}
static int test_list(void) {
  YF_TEST_SUBT;

  int a = 'a';
  int b = 'b';
  int c = 'c';
  int d = 'd';

  YF_list ls = yf_list_init(NULL);

  if (yf_list_getlen(ls) != 0)
    return -1;
  if (yf_list_contains(ls, NULL))
    return -1;

  yf_list_insert(ls, &a);
  if (yf_list_getlen(ls) != 1)
    return -1;
  if (!yf_list_contains(ls, &a))
    return -1;

  yf_list_insert(ls, &b);
  if (yf_list_getlen(ls) != 2)
    return -1;
  if (!yf_list_contains(ls, &b))
    return -1;
  if (!yf_list_contains(ls, &a))
    return -1;

  yf_list_insert(ls, &c);
  if (yf_list_getlen(ls) != 3)
    return -1;
  if (!yf_list_contains(ls, &c))
    return -1;
  if (!yf_list_contains(ls, &b))
    return -1;
  if (!yf_list_contains(ls, &a))
    return -1;
  if (yf_list_contains(ls, &d))
    return -1;

  yf_list_remove(ls, &b);
  if (yf_list_getlen(ls) != 2)
    return -1;
  if (yf_list_contains(ls, &b))
    return -1;
  if (!yf_list_contains(ls, &a))
    return -1;
  if (!yf_list_contains(ls, &c))
    return -1;

  yf_list_remove(ls, &a);
  if (yf_list_getlen(ls) != 1)
    return -1;
  if (yf_list_contains(ls, &a))
    return -1;
  if (yf_list_contains(ls, &b))
    return -1;
  if (!yf_list_contains(ls, &c))
    return -1;

  yf_list_insert(ls, &d);
  if (yf_list_getlen(ls) != 2)
    return -1;
  if (!yf_list_contains(ls, &c))
    return -1;
  if (!yf_list_contains(ls, &d))
    return -1;
  if (yf_list_contains(ls, &a))
    return -1;
  if (yf_list_contains(ls, &b))
    return -1;

  yf_list_insert(ls, &b);
  if (yf_list_getlen(ls) != 3)
    return -1;
  if (!yf_list_contains(ls, &b))
    return -1;
  if (!yf_list_contains(ls, &c))
    return -1;
  if (!yf_list_contains(ls, &d))
    return -1;
  if (yf_list_contains(ls, &a))
    return -1;

  YF_iter it;

  printf("\n(list beg) ");
  it = YF_NILIT;
  for (;;) {
    int *v = yf_list_next(ls, &it);
    if (YF_IT_ISNIL(it))
      break;
    printf("%c ", *v);
  }
  puts("(list end)");

  printf("(list beg) ");
  yf_list_each(ls, list_cb, NULL);
  puts("(list end)");

  yf_list_remove(ls, &c);

  printf("(list beg) ");
  it = YF_NILIT;
  for (;;) {
    int *v = yf_list_next(ls, &it);
    if (YF_IT_ISNIL(it))
      break;
    printf("%c ", *v);
  }
  puts("(list end)");

  printf("(list beg) ");
  yf_list_each(ls, list_cb, NULL);
  puts("(list end)");

  yf_list_clear(ls);

  printf("(list beg) ");
  it = YF_NILIT;
  for (;;) {
    int *v = yf_list_next(ls, &it);
    if (YF_IT_ISNIL(it))
      break;
    printf("%c ", *v);
  }
  puts("(list end)");

  printf("(list beg) ");
  yf_list_each(ls, list_cb, NULL);
  puts("(list end)");

  if (yf_list_getlen(ls) != 0)
    return -1;
  if (yf_list_contains(ls, &a))
    return -1;
  if (yf_list_contains(ls, &b))
    return -1;
  if (yf_list_contains(ls, &c))
    return -1;
  if (yf_list_contains(ls, &d))
    return -1;

  puts("");
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
