/*
 * YF
 * test.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_TEST_H
#define YF_TEST_H

/* Calls the test function identified by the given number. */
#define YF_TESTFN_CALL(num, r) do { \
  switch (num) { \
    case 1: r = yf_test_1(); break; \
    case 2: r = yf_test_2(); break; \
    case 3: r = yf_test_3(); break; \
    case 4: r = yf_test_4(); break; \
    case 5: r = yf_test_5(); break; \
    case 6: r = yf_test_6(); break; \
    case 7: r = yf_test_7(); break; \
    case 8: r = yf_test_8(); break; \
    default: r = 1 << 31; \
  } } while (0)

/* Test functions. */
int yf_test_1(void);
int yf_test_2(void);
int yf_test_3(void);
int yf_test_4(void);
int yf_test_5(void);
int yf_test_6(void);
int yf_test_7(void);
int yf_test_8(void);

#endif /* YF_TEST_H */
