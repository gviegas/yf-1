/*
 * YF
 * test-dict.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "yf-dict.h"
#include "yf-error.h"

/* 'dict_each()' callback. */
static int dict_cb(void *key, void *val, void *arg)
{
    printf(" key: %p, val: %s, arg: %zu\n", key, (char *)val, (size_t)arg);
    return (size_t)arg;
}

/* Tests dictionary. */
int yf_test_dict(void)
{
    puts("(init NULL,NULL)\n -> dict\n");
    YF_dict dict = yf_dict_init(NULL, NULL);

    const void *key1 = (const void *)1UL;
    const void *key2 = (const void *)1234UL;
    const void *key3 = (const void *)255UL;
    void *key = NULL;
    const char *val = NULL;
    YF_iter it = YF_NILIT;

    if (yf_dict_getlen(dict) != 0)
        return -1;

    printf("(insert dict,%p,\"a\")\n\n", key1);
    yf_dict_insert(dict, key1, "a");
    if (!yf_dict_contains(dict, key1) || yf_dict_getlen(dict) != 1)
        return -1;

    printf("(insert dict,%p,\"b\")\n\n", key2);
    yf_dict_insert(dict, key2, "b");
    if (!yf_dict_contains(dict, key2) || yf_dict_getlen(dict) != 2)
        return 1;

    yf_seterr(YF_ERR_UNKNOWN, NULL);
    yf_dict_insert(dict, key1, "c");
    if (yf_dict_getlen(dict) != 2 || yf_geterr() != YF_ERR_EXIST)
        return -1;

    printf("(remove dict,%p)\n\n", key1);
    val = yf_dict_remove(dict, key1);
    if (val == NULL || strcmp(val, "a") != 0 ||
        yf_dict_contains(dict, key1) || yf_dict_getlen(dict) != 1)
        return -1;

    printf("(remove dict,%p)\n\n", key1);
    val = yf_dict_remove(dict, key1);
    if (val != NULL || yf_dict_getlen(dict) != 1)
        return -1;

    printf("(search dict,%p)\n\n", key2);
    val = yf_dict_search(dict, key2);
    if (val == NULL || strcmp(val, "b") != 0 || !yf_dict_contains(dict, key2))
        return -1;

    yf_seterr(YF_ERR_UNKNOWN, NULL);
    printf("(search dict,%p)\n\n", key1);
    val = yf_dict_search(dict, key1);
    if (val != NULL || yf_geterr() != YF_ERR_NOTFND)
        return -1;

    printf("(insert dict,%p,NULL)\n\n", key3);
    yf_dict_insert(dict, key3, NULL);
    if (yf_dict_getlen(dict) != 2 || !yf_dict_contains(dict, key3))
        return -1;

    printf("(replace dict,%p,\"xyz\")\n\n", key2);
    val = yf_dict_replace(dict, key2, "xyz");
    if (val == NULL || strcmp(val, "b") != 0 || !yf_dict_contains(dict, key2))
        return -1;

    printf("(search dict,%p)\n\n", key2);
    val = yf_dict_search(dict, key2);
    if (val == NULL || strcmp(val, "xyz") != 0)
        return -1;

    yf_seterr(YF_ERR_UNKNOWN, NULL);
    printf("(replace dict,%p,\"KEY ONE\")\n\n", key1);
    val = yf_dict_replace(dict, key1, "KEY ONE");
    if (val != NULL || yf_geterr() != YF_ERR_NOTFND)
        return -1;

    it = YF_NILIT;
    puts("(next dict,&it,&key)");
    for (;;) {
        val = yf_dict_next(dict, &it, &key);
        if (YF_IT_ISNIL(it))
            break;
        printf(" key: %p, val: %s\n", key, val);
    }
    puts("");

    it = YF_NILIT;
    key = (void *)65535ULL;
    puts("(next dict,&it,NULL)");
    for (;;) {
        val = yf_dict_next(dict, &it, NULL);
        if (YF_IT_ISNIL(it))
            break;
        printf(" key: %p, val: %s\n", key, val);
    }
    puts("");

    for (size_t i = 0; i < 3; i++) {
        puts("(next dict,NULL,&key)");
        val = yf_dict_next(dict, NULL, &key);
        printf(" key: %p, val: %s\n\n", key, val);
    }

    key = (void *)127UL;
    for (size_t i = 0; i < 3; i++) {
        puts("(next dict,NULL,NULL)");
        val = yf_dict_next(dict, NULL, NULL);
        printf(" key: %p, val: %s\n\n", key, val);
    }

    puts("(each dict,dict_cb,NULL)");
    yf_dict_each(dict, dict_cb, NULL);
    puts("");

    puts("(each dict,dict_cb,1)");
    yf_dict_each(dict, dict_cb, (void *)1ULL);
    puts("");

    puts("(clear dict)\n");
    yf_dict_clear(dict);
    if (yf_dict_getlen(dict) != 0 || yf_dict_contains(dict, key3))
        return -1;

    puts("(deinit dict)\n");
    yf_dict_deinit(dict);

    puts("(init NULL,NULL)\n ->dict\n");
    dict = yf_dict_init(NULL, NULL);

    size_t count = 50;

    for (size_t i = 0; i < count; i++) {
        printf("(insert dict,%zu,%zu)\n\n", i, i*i);
        if (yf_dict_insert(dict, (void *)i, (void *)(i*i)) != 0)
            return -1;
    }

    if (yf_dict_getlen(dict) != count ||
        yf_dict_contains(dict, (void *)count) ||
        !yf_dict_contains(dict, (void *)(count-1)) ||
        !yf_dict_contains(dict, 0) ||
        !yf_dict_contains(dict, (void *)(count>>1)))
        return -1;

    yf_seterr(YF_ERR_UNKNOWN, NULL);
    for (size_t i = 0; i < count; i++) {
        printf("(remove dict,%zu)\n\n", i);
        void *val = yf_dict_remove(dict, (void *)i);
        if (yf_geterr() == YF_ERR_NOTFND || (size_t)val != i*i)
            return -1;
    }

    if (yf_dict_getlen(dict) != 0 ||
        yf_dict_contains(dict, (void *)(count-1)) ||
        yf_dict_contains(dict, 0) ||
        yf_dict_contains(dict, (void *)(count>>1)))
        return -1;

    puts("(deinit dict)\n");
    yf_dict_deinit(dict);

    puts("(init hashstr,cmpstr)\n -> dict\n");
    dict = yf_dict_init(yf_hashstr, yf_cmpstr);

    count = 20;
    const size_t len = 32;
    char *strs = malloc(count * len);
    assert(strs != NULL);
    srand(time(NULL));

    for (size_t i = 0; i < count; i++) {
        int n = 4 + rand() % 20;
        for (int j = 0; j < n; j++) {
            strs[i*len+j] = 32 + rand() % 96;
        }
        strs[i*len+n] = '\0';
    }

    for (size_t i = 0; i < count; i++) {
        printf("(insert dict,%p,%zu)\n\n", &strs[i*len], i);
        if (yf_dict_insert(dict, &strs[i*len], (void *)i) != 0)
            return -1;
    }

    for (size_t i = 0; i < count; i++) {
        printf("(contains dict,%p)\n\n", &strs[i*len]);
        if (!yf_dict_contains(dict, &strs[i*len]))
            return -1;
    }

    char str[len];
    strcpy(str, &strs[(count>>1)*len]);
    if (!yf_dict_contains(dict, str))
        return -1;

    yf_seterr(YF_ERR_UNKNOWN, NULL);
    if (yf_dict_insert(dict, str, (void *)0xff) == 0 ||
        yf_geterr() != YF_ERR_EXIST)
        return -1;

    yf_seterr(YF_ERR_UNKNOWN, NULL);
    for (size_t i = 0; i < count; i++) {
        printf("(remove dict,%p)\n\n", &strs[i*len]);
        yf_dict_remove(dict, &strs[i*len]);
        if (yf_geterr() == YF_ERR_NOTFND)
            return -1;
    }

    free(strs);

    puts("(clear dict)\n");
    yf_dict_clear(dict);
    if (yf_dict_getlen(dict) != 0)
        return -1;

    char *str2 = "string";
    printf("(insert dict,%s,1)\n\n", str2);
    if (yf_dict_insert(dict, str2, (void *)1) != 0 ||
        yf_dict_getlen(dict) != 1 || !yf_dict_contains(dict, str2) ||
        !yf_dict_contains(dict, "string") || yf_dict_contains(dict, "STRING"))
        return -1;

    char *str3 = malloc(16);
    assert(str3 != NULL);
    strcpy(str3, str2);
    printf("(insert dict,%s,1)\n\n", str2);
    printf("(insert dict,%s,2)\n\n", str3);
    if (yf_dict_insert(dict, str2, (void *)1) == 0 ||
        yf_dict_insert(dict, str3, (void *)2) == 0 ||
        !yf_dict_contains(dict, str3))
        return -1;

    void *dst = str3;
    printf("(lookup dict,&dst<%s>)\n\n", (char *)dst);
    size_t ival = (size_t)yf_dict_lookup(dict, &dst);
    if (ival != 1 || dst == str3 || dst != str2 || yf_dict_getlen(dict) != 1 ||
        !yf_dict_contains(dict, dst))
        return -1;

    dst = str3;
    printf("(delete dict,&dst<%s>)\n\n", (char *)dst);
    ival = (size_t)yf_dict_delete(dict, &dst);
    if (ival != 1 || dst == str3 || dst != str2 || yf_dict_getlen(dict) != 0 ||
        yf_dict_contains(dict, dst) || yf_dict_contains(dict, str3))
        return -1;

    free(str3);

    puts("(deinit dict)");
    yf_dict_deinit(dict);

    return 0;
}
