/*
 * YF
 * test-list.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "test.h"
#include "yf-list.h"

/* 'list_each()' callback. */
static int list_cb(void *val, void *arg)
{
    printf(" %c,%p", *(int *)val, arg);
    return 0;
}

/* Tests list. */
int yf_test_list(void)
{
    const int a = 'a';
    const int b = 'b';
    const int c = 'c';
    const int d = 'd';

    YF_TEST_PRINT("init", "NULL", "ls");
    yf_list_t *ls = yf_list_init(NULL);
    if (yf_list_getlen(ls) != 0)
        return -1;
    if (yf_list_contains(ls, NULL))
        return -1;

    YF_TEST_PRINT("insert", "ls, &a", "");
    yf_list_insert(ls, &a);
    if (yf_list_getlen(ls) != 1)
        return -1;
    if (!yf_list_contains(ls, &a))
        return -1;

    YF_TEST_PRINT("insert", "ls, &b", "");
    yf_list_insert(ls, &b);
    if (yf_list_getlen(ls) != 2)
        return -1;
    if (!yf_list_contains(ls, &b))
        return -1;
    if (!yf_list_contains(ls, &a))
        return -1;

    YF_TEST_PRINT("insert", "ls, &c", "");
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

    YF_TEST_PRINT("remove", "ls, &b", "");
    yf_list_remove(ls, &b);
    if (yf_list_getlen(ls) != 2)
        return -1;
    if (yf_list_contains(ls, &b))
        return -1;
    if (!yf_list_contains(ls, &a))
        return -1;
    if (!yf_list_contains(ls, &c))
        return -1;

    YF_TEST_PRINT("remove", "ls, &a", "");
    yf_list_remove(ls, &a);
    if (yf_list_getlen(ls) != 1)
        return -1;
    if (yf_list_contains(ls, &a))
        return -1;
    if (yf_list_contains(ls, &b))
        return -1;
    if (!yf_list_contains(ls, &c))
        return -1;

    YF_TEST_PRINT("insert", "ls, &d", "");
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

    YF_TEST_PRINT("insert", "ls, &b", "");
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

    yf_iter_t it;

    YF_TEST_PRINT("next", "ls, &it", "");
    it = YF_NILIT;
    for (;;) {
        int *v = yf_list_next(ls, &it);
        if (YF_IT_ISNIL(it))
            break;
        printf(" %c", *v);
    }
    puts("");

    YF_TEST_PRINT("each", "ls, list_cb, 0x01f", "");
    yf_list_each(ls, list_cb, (void*)0x1f);
    puts("");

    YF_TEST_PRINT("remove", "ls, &c", "");
    yf_list_remove(ls, &c);

    YF_TEST_PRINT("next", "ls, &it", "");
    it = YF_NILIT;
    for (;;) {
        int *v = yf_list_next(ls, &it);
        if (YF_IT_ISNIL(it))
            break;
        printf(" %c", *v);
    }
    puts("");

    YF_TEST_PRINT("each", "ls, list_cb, NULL", "");
    yf_list_each(ls, list_cb, NULL);
    puts("");

    YF_TEST_PRINT("clear", "ls", "");
    yf_list_clear(ls);

    YF_TEST_PRINT("next", "ls, &it", "");
    it = YF_NILIT;
    for (;;) {
        int *v = yf_list_next(ls, &it);
        if (YF_IT_ISNIL(it))
            break;
        printf(" %c", *v);
    }
    puts("");

    YF_TEST_PRINT("each", "ls, list_cb, NULL", "");
    yf_list_each(ls, list_cb, NULL);
    puts("");

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

    YF_TEST_PRINT("insertat", "ls, NULL, &a", "");
    yf_list_insertat(ls, NULL, &a);
    if (yf_list_getlen(ls) != 1)
        return -1;
    YF_TEST_PRINT("insertat", "ls, NULL, &b", "");
    yf_list_insertat(ls, NULL, &b);
    YF_TEST_PRINT("insertat", "ls, NULL, &c", "");
    yf_list_insertat(ls, NULL, &c);
    YF_TEST_PRINT("insertat", "ls, NULL, &d", "");
    yf_list_insertat(ls, NULL, &d);
    if (yf_list_getlen(ls) != 4)
        return -1;

    YF_TEST_PRINT("each", "ls, list_cb, NULL", "");
    yf_list_each(ls, list_cb, NULL);
    puts("");

    YF_TEST_PRINT("clear", "ls", "");
    yf_list_clear(ls);

    it = YF_NILIT;
    YF_TEST_PRINT("insertat", "ls, &it, &a", "");
    yf_list_insertat(ls, &it, &a);
    if (yf_list_getlen(ls) != 1)
        return -1;
    YF_TEST_PRINT("insertat", "ls, &it, &b", "");
    yf_list_insertat(ls, &it, &b);
    YF_TEST_PRINT("insertat", "ls, &it, &c", "");
    yf_list_insertat(ls, &it, &c);
    YF_TEST_PRINT("insertat", "ls, &it, &d", "");
    yf_list_insertat(ls, &it, &d);
    if (yf_list_getlen(ls) != 4)
        return -1;

    YF_TEST_PRINT("each", "ls, list_cb, NULL", "");
    yf_list_each(ls, list_cb, NULL);
    puts("");

    if (YF_IT_ISNIL(it))
        return -1;
    if (yf_list_next(ls, &it) != NULL)
        return -1;
    if (!YF_IT_ISNIL(it))
        return -1;

    int *v;
    while ((v = yf_list_next(ls, NULL)) != NULL) {
        YF_TEST_PRINT("removeat", "ls, NULL", "");
        if (*v != *(int *)yf_list_removeat(ls, NULL))
            return -1;
        YF_TEST_PRINT("each", "ls, list_cb, NULL", "");
        yf_list_each(ls, list_cb, NULL);
        puts("");
    }

    YF_TEST_PRINT("insert", "ls, &a", "");
    yf_list_insert(ls, &a);
    YF_TEST_PRINT("insert", "ls, &b", "");
    yf_list_insert(ls, &b);
    YF_TEST_PRINT("insert", "ls, &c", "");
    yf_list_insert(ls, &c);
    YF_TEST_PRINT("insert", "ls, &d", "");
    yf_list_insert(ls, &d);

    it = YF_NILIT;
    while ((v = yf_list_next(ls, NULL)) != NULL) {
        YF_TEST_PRINT("removeat", "ls, &it", "");
        if (*v != *(int *)yf_list_removeat(ls, &it))
            return -1;
        YF_TEST_PRINT("each", "ls, list_cb, NULL", "");
        yf_list_each(ls, list_cb, NULL);
        puts("");
    }

    it = YF_NILIT;
    YF_TEST_PRINT("insertat", "ls, &it, &a", "");
    yf_list_insertat(ls, &it, &a);
    YF_TEST_PRINT("insertat", "ls, &it, &b", "");
    yf_list_insertat(ls, &it, &b);
    YF_TEST_PRINT("insertat", "ls, &it, &c", "");
    yf_list_insertat(ls, &it, &c);

    YF_TEST_PRINT("each", "ls, list_cb, NULL", "");
    yf_list_each(ls, list_cb, NULL);
    puts("");

    YF_TEST_PRINT("next", "ls, &it", "");
    it = YF_NILIT;
    for (;;) {
        v = yf_list_next(ls, &it);
        printf(" %c", *v);
        if (*v == b) {
            printf(" <early break>");
            break;
        }
    }
    if (!yf_list_contains(ls, &b))
        return -1;
    puts("");

    YF_TEST_PRINT("removeat", "ls, &it", "");
    yf_list_removeat(ls, &it);
    if (yf_list_contains(ls, &b))
        return -1;
    if (yf_list_getlen(ls) != 2)
        return -1;

    YF_TEST_PRINT("each", "ls, list_cb, NULL", "");
    yf_list_each(ls, list_cb, NULL);
    puts("");

    YF_TEST_PRINT("deinit", "ls", "");
    yf_list_deinit(ls);

    return 0;
}
