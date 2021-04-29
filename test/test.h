/*
 * YF
 * test.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_TEST_H
#define YF_TEST_H

/* Type defining a single test to execute. */
typedef struct {
  char name[64];
  int (*fn)(int argc, char *argv[]);
  const char *const *ids;
  size_t id_n;
} YF_test;

extern const YF_test yf_g_test;

#endif /* YF_TEST_H */
