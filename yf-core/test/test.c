/*
 * YF
 * test.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "test.h"

int main(int argc, char *argv[]) {
  int r;
  long num;
  if (argc > 1) {
    char *end;
    errno = 0;
    num = strtol(argv[1], &end, 0);
    if (errno != 0 || end == argv[1]) {
      fprintf(stderr, "[YF-Debug] Usage: %s [TEST_FUNC_NUMBER]\n", argv[0]);
      return -1;
    }
  } else {
    num = 1;
  }
  puts("[test]");
  for (int i = 0; i < argc; ++i)
    printf("%s ", argv[i]);
  puts("\n");
  YF_TESTFN_CALL(num, r);
  printf("\n*** test #%ld completed (%d)\n", num, r);
  puts("[end]");
  return r;
}
