/*
 * YF
 * test-list.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

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

    puts("(init NULL)\n -> ls\n");
    YF_list ls = yf_list_init(NULL);
    if (yf_list_getlen(ls) != 0)
        return -1;
    if (yf_list_contains(ls, NULL))
        return -1;

    puts("(insert ls,&a)\n");
    yf_list_insert(ls, &a);
    if (yf_list_getlen(ls) != 1)
        return -1;
    if (!yf_list_contains(ls, &a))
        return -1;

    puts("(insert ls,&b)\n");
    yf_list_insert(ls, &b);
    if (yf_list_getlen(ls) != 2)
        return -1;
    if (!yf_list_contains(ls, &b))
        return -1;
    if (!yf_list_contains(ls, &a))
        return -1;

    puts("(insert ls,&c)\n");
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

    puts("(remove ls,&b)\n");
    yf_list_remove(ls, &b);
    if (yf_list_getlen(ls) != 2)
        return -1;
    if (yf_list_contains(ls, &b))
        return -1;
    if (!yf_list_contains(ls, &a))
        return -1;
    if (!yf_list_contains(ls, &c))
        return -1;

    puts("(remove ls,&a)\n");
    yf_list_remove(ls, &a);
    if (yf_list_getlen(ls) != 1)
        return -1;
    if (yf_list_contains(ls, &a))
        return -1;
    if (yf_list_contains(ls, &b))
        return -1;
    if (!yf_list_contains(ls, &c))
        return -1;

    puts("(insert ls,&d)\n");
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

    puts("(insert ls,&b)\n");
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

    puts("(next ls,&it)");
    it = YF_NILIT;
    for (;;) {
        int *v = yf_list_next(ls, &it);
        if (YF_IT_ISNIL(it))
            break;
        printf(" %c", *v);
    }
    puts("\n");

    puts("(each ls,list_cb,0x1f)");
    yf_list_each(ls, list_cb, (void*)0x1f);
    puts("\n");

    puts("(remove ls,&c)\n");
    yf_list_remove(ls, &c);

    puts("(next ls,&it)");
    it = YF_NILIT;
    for (;;) {
        int *v = yf_list_next(ls, &it);
        if (YF_IT_ISNIL(it))
            break;
        printf(" %c", *v);
    }
    puts("\n");

    puts("(each ls,list_cb,NULL)");
    yf_list_each(ls, list_cb, NULL);
    puts("\n");

    puts("(clear ls)\n");
    yf_list_clear(ls);

    puts("(next ls,&it)");
    it = YF_NILIT;
    for (;;) {
        int *v = yf_list_next(ls, &it);
        if (YF_IT_ISNIL(it))
            break;
        printf(" %c", *v);
    }
    puts("\n");

    puts("(each ls,list_cb,NULL)");
    yf_list_each(ls, list_cb, NULL);
    puts("\n");

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

    puts("(insertat ls,NULL,&a)\n");
    yf_list_insertat(ls, NULL, &a);
    if (yf_list_getlen(ls) != 1)
        return -1;
    puts("(insertat ls,NULL,&b)\n");
    yf_list_insertat(ls, NULL, &b);
    puts("(insertat ls,NULL,&c)\n");
    yf_list_insertat(ls, NULL, &c);
    puts("(insertat ls,NULL,&d)\n");
    yf_list_insertat(ls, NULL, &d);
    if (yf_list_getlen(ls) != 4)
        return -1;

    puts("(each ls,list_cb,NULL)");
    yf_list_each(ls, list_cb, NULL);
    puts("\n");

    puts("(clear ls)\n");
    yf_list_clear(ls);

    it = YF_NILIT;
    puts("(insertat ls,&it,&a)\n");
    yf_list_insertat(ls, &it, &a);
    if (yf_list_getlen(ls) != 1)
        return -1;
    puts("(insertat ls,&it,&b)\n");
    yf_list_insertat(ls, &it, &b);
    puts("(insertat ls,&it,&c)\n");
    yf_list_insertat(ls, &it, &c);
    puts("(insertat ls,&it,&d)\n");
    yf_list_insertat(ls, &it, &d);
    if (yf_list_getlen(ls) != 4)
        return -1;

    puts("(each ls,list_cb,NULL)");
    yf_list_each(ls, list_cb, NULL);
    puts("\n");

    if (YF_IT_ISNIL(it))
        return -1;
    if (yf_list_next(ls, &it) != NULL)
        return -1;
    if (!YF_IT_ISNIL(it))
        return -1;

    int *v;
    while ((v = yf_list_next(ls, NULL)) != NULL) {
        puts("(removeat ls,NULL)\n");
        if (*v != *(int *)yf_list_removeat(ls, NULL))
            return -1;
        puts("(each ls,list_cb,NULL)");
        yf_list_each(ls, list_cb, NULL);
        puts("\n");
    }

    puts("(insert ls,&a)\n");
    yf_list_insert(ls, &a);
    puts("(insert ls,&b)\n");
    yf_list_insert(ls, &b);
    puts("(insert ls,&c)\n");
    yf_list_insert(ls, &c);
    puts("(insert ls,&d)\n");
    yf_list_insert(ls, &d);

    it = YF_NILIT;
    while ((v = yf_list_next(ls, NULL)) != NULL) {
        puts("(removeat ls,&it)\n");
        if (*v != *(int *)yf_list_removeat(ls, &it))
            return -1;
        puts("(each ls,list_cb,NULL)");
        yf_list_each(ls, list_cb, NULL);
        puts("\n");
    }

    it = YF_NILIT;
    puts("(insertat ls,&it,&a)\n");
    yf_list_insertat(ls, &it, &a);
    puts("(insertat ls,&it,&b)\n");
    yf_list_insertat(ls, &it, &b);
    puts("(insertat ls,&it,&c)\n");
    yf_list_insertat(ls, &it, &c);

    puts("(each ls,list_cb,NULL)");
    yf_list_each(ls, list_cb, NULL);
    puts("\n");

    puts("(next ls,&it)");
    it = YF_NILIT;
    for (;;) {
        v = yf_list_next(ls, &it);
        printf(" %c", *v);
        if (*v == b)
            break;
    }
    if (!yf_list_contains(ls, &b))
        return -1;
    puts("\n");

    puts("(removeat ls,&it)\n");
    yf_list_removeat(ls, &it);
    if (yf_list_contains(ls, &b))
        return -1;
    if (yf_list_getlen(ls) != 2)
        return -1;

    puts("(each ls,list_cb,NULL)");
    yf_list_each(ls, list_cb, NULL);
    puts("\n");

    puts("(deinit ls)");
    yf_list_deinit(ls);

    return 0;
}
