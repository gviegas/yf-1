/*
 * YF
 * test.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_TEST_H
#define YF_TEST_H

typedef struct {
  char name[64];
  int (*fn)(int argc, char *argv[]);
} YF_test;

extern const YF_test yf_g_test;

#endif /* YF_TEST_H */
