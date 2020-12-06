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
#include "hashset.h"

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
  yf_list_each(ls, list_cb, (void*)0x1f);
  puts("(list end)");

  yf_list_remove(ls, &c);

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

  yf_list_clear(ls);

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

  yf_list_deinit(ls);

  puts("");
  return 0;
}

/* Hashset test. */
static size_t hashset_hash(const void *x) {
  if (x == NULL)
    return 1<<24;
  const char *s = x;
  return ((size_t)s[0] << 16) | ((size_t)s[1] << 8) | (size_t)s[2];
}
static int hashset_cmp(const void *a, const void *b) {
  return strcmp(a, b);
}
static int hashset_cb(void *val, void *arg) {
  printf("%s,%p ", (const char *)val, arg);
  if (strcmp(val, "_B_") == 0) {
    printf("{early break} ");
    return 1;
  }
  return 0;
}
static int test_hashset(void) {
  YF_TEST_SUBT;

  const char a[] = "_A_";
  const char b[] = "_B_";
  const char c[] = "_C_";
  const char d[] = "_D_";
  const char d1[] = "_D_";

  YF_hashset hs = yf_hashset_init(hashset_hash, hashset_cmp);

  if (yf_hashset_getlen(hs) != 0)
    return -1;
  if (yf_hashset_contains(hs, NULL))
    return -1;

  yf_hashset_insert(hs, d);
  if (yf_hashset_getlen(hs) != 1)
    return -1;
  if (!yf_hashset_contains(hs, d))
    return -1;
  if (!yf_hashset_contains(hs, d1))
    return -1;
  if (yf_hashset_insert(hs, d) == 0)
    return -1;
  if (yf_hashset_insert(hs, d1) == 0)
    return -1;
  if (yf_hashset_search(hs, d1) != d)
    return -1;

  yf_hashset_insert(hs, c);
  if (yf_hashset_getlen(hs) != 2)
    return -1;
  if (!yf_hashset_contains(hs, c))
    return -1;
  if (!yf_hashset_contains(hs, d))
    return -1;
  if (yf_hashset_insert(hs, c) == 0)
    return -1;
  if (yf_hashset_insert(hs, d) == 0)
    return -1;
  if (yf_hashset_search(hs, c) != c)
    return -1;

  yf_hashset_insert(hs, b);
  if (yf_hashset_getlen(hs) != 3)
    return -1;
  if (!yf_hashset_contains(hs, b))
    return -1;
  if (!yf_hashset_contains(hs, d1))
    return -1;
  if (!yf_hashset_contains(hs, c))
    return -1;
  if (yf_hashset_contains(hs, a))
    return -1;
  if (yf_hashset_insert(hs, b) == 0)
    return -1;
  if (yf_hashset_search(hs, "_B_") != b)
    return -1;

  YF_iter it;

  printf("\n(set beg) ");
  it = YF_NILIT;
  for (;;) {
    const char *s = yf_hashset_next(hs, &it);
    if (YF_IT_ISNIL(it))
      break;
    printf("%s ", s);
  }
  puts("(set end)");

  printf("(set beg) ");
  yf_hashset_each(hs, hashset_cb, (void *)0xff);
  puts("(set end)");

  yf_hashset_remove(hs, d1);
  if (yf_hashset_getlen(hs) != 2)
    return -1;
  if (yf_hashset_contains(hs, d))
    return -1;
  if (yf_hashset_contains(hs, d1))
    return -1;
  if (!yf_hashset_contains(hs, b))
    return -1;
  if (yf_hashset_search(hs, d) != NULL)
    return -1;

  printf("\n(set beg) ");
  it = YF_NILIT;
  for (;;) {
    const char *s = yf_hashset_next(hs, &it);
    if (YF_IT_ISNIL(it))
      break;
    printf("%s ", s);
  }
  puts("(set end)");

  printf("(set beg) ");
  yf_hashset_each(hs, hashset_cb, NULL);
  puts("(set end)");

  yf_hashset_remove(hs, c);
  if (yf_hashset_getlen(hs) != 1)
    return -1;
  if (yf_hashset_contains(hs, d))
    return -1;
  if (yf_hashset_contains(hs, d1))
    return -1;
  if (!yf_hashset_contains(hs, b))
    return -1;
  if (yf_hashset_remove(hs, c) == 0)
    return -1;
  if (yf_hashset_insert(hs, "_B_") == 0)
    return -1;

  printf("\n(set beg) ");
  it = YF_NILIT;
  for (;;) {
    const char *s = yf_hashset_next(hs, &it);
    if (YF_IT_ISNIL(it))
      break;
    printf("%s ", s);
  }
  puts("(set end)");

  printf("(set beg) ");
  yf_hashset_each(hs, hashset_cb, NULL);
  puts("(set end)");

  yf_hashset_insert(hs, a);
  if (yf_hashset_getlen(hs) != 2)
    return -1;
  if (!yf_hashset_contains(hs, a))
    return -1;
  if (!yf_hashset_contains(hs, b))
    return -1;
  if (yf_hashset_contains(hs, c))
    return -1;
  if (yf_hashset_contains(hs, "_C_"))
    return -1;
  if (yf_hashset_contains(hs, d))
    return -1;

  printf("\n(set beg) ");
  it = YF_NILIT;
  for (;;) {
    const char *s = yf_hashset_next(hs, &it);
    if (YF_IT_ISNIL(it))
      break;
    printf("%s ", s);
  }
  puts("(set end)");

  printf("(set beg) ");
  yf_hashset_each(hs, hashset_cb, NULL);
  puts("(set end)");

  it = YF_NILIT;
  void *val = yf_hashset_extract(hs, &it);
  if (yf_hashset_getlen(hs) != 1)
    return -1;
  if (yf_hashset_contains(hs, val))
    return -1;

  printf("\n(set beg) ");
  it = YF_NILIT;
  for (;;) {
    const char *s = yf_hashset_next(hs, &it);
    if (YF_IT_ISNIL(it))
      break;
    printf("%s ", s);
  }
  puts("(set end)");

  printf("(set beg) ");
  yf_hashset_each(hs, hashset_cb, NULL);
  puts("(set end)");

  yf_hashset_clear(hs);
  if (yf_hashset_getlen(hs) != 0)
    return -1;
  if (yf_hashset_contains(hs, a))
    return -1;
  if (yf_hashset_contains(hs, b))
    return -1;
  if (yf_hashset_contains(hs, c))
    return -1;
  if (yf_hashset_contains(hs, d))
    return -1;
  if (yf_hashset_contains(hs, d1))
    return -1;
  if (yf_hashset_contains(hs, "_D_"))
    return -1;

  printf("\n(set beg) ");
  it = YF_NILIT;
  for (;;) {
    const char *s = yf_hashset_next(hs, &it);
    if (YF_IT_ISNIL(it))
      break;
    printf("%s ", s);
  }
  puts("(set end)");

  printf("(set beg) ");
  yf_hashset_each(hs, hashset_cb, NULL);
  puts("(set end)");

  it = YF_NILIT;
  if (yf_hashset_extract(hs, &it) != NULL)
    return -1;
  if (yf_hashset_search(hs, d) != NULL)
    return -1;
  if (yf_hashset_getlen(hs) != 0)
    return -1;

  yf_hashset_deinit(hs);

  puts("");
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
