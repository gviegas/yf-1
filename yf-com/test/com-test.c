/*
 * YF
 * com-test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "test.h"
#include "yf-com.h"

#define YF_TEST_ALL "all"
#define YF_TEST_SUBLN "..............................."
#define YF_TEST_SUBT \
    printf("%s\n%.*s\n", __func__, (int)strlen(__func__), YF_TEST_SUBLN)

/* Error test. */
#define YF_TEST_ERROR "error"

int yf_test_error(void);

static int test_error(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_error();
    puts("");
    return r;
}

/* Clock test. */
#define YF_TEST_CLOCK "clock"

int yf_test_clock(void);

static int test_clock(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_clock();
    puts("");
    return r;
}

/* List test. */
#define YF_TEST_LIST "list"

int yf_test_list(void);

static int test_list(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_list();
    puts("");
    return r;
}

/* Dictionary test. */
#define YF_TEST_DICT "dict"

static int dict_cb(void *key, void *val, void *arg)
{
    printf("\n %s:\n  key: %p\n  val: %s\n  arg: %zu", __func__,
           key, (char *)val, (size_t)arg);

    return (size_t)arg;
}

static int test_dict(void)
{
    YF_TEST_SUBT;

    puts("\ninit()");
    YF_dict dict = yf_dict_init(NULL, NULL);

    const void *key1 = (const void *)1ULL;
    const void *key2 = (const void *)1234UL;
    const void *key3 = (const void *)255UL;
    void *key = NULL;
    const char *val = NULL;
    YF_iter it = YF_NILIT;

    puts("\ninsert()");
    if (yf_dict_getlen(dict) != 0)
        return -1;

    yf_dict_insert(dict, key1, "a");
    if (!yf_dict_contains(dict, key1) || yf_dict_getlen(dict) != 1)
        return -1;

    yf_dict_insert(dict, key2, "b");
    if (!yf_dict_contains(dict, key2) || yf_dict_getlen(dict) != 2)
        return 1;

    yf_seterr(YF_ERR_UNKNOWN, NULL);
    yf_dict_insert(dict, key1, "c");
    if (yf_dict_getlen(dict) != 2 || yf_geterr() != YF_ERR_EXIST)
        return -1;

    puts("\nremove()");
    val = yf_dict_remove(dict, key1);
    if (val == NULL || strcmp(val, "a") != 0 ||
        yf_dict_contains(dict, key1) || yf_dict_getlen(dict) != 1)
        return -1;

    val = yf_dict_remove(dict, key1);
    if (val != NULL || yf_dict_getlen(dict) != 1)
        return -1;

    puts("\nsearch()");
    val = yf_dict_search(dict, key2);
    if (val == NULL || strcmp(val, "b") != 0 || !yf_dict_contains(dict, key2))
        return -1;

    yf_seterr(YF_ERR_UNKNOWN, NULL);
    val = yf_dict_search(dict, key1);
    if (val != NULL || yf_geterr() != YF_ERR_NOTFND)
        return -1;

    yf_dict_insert(dict, key3, NULL);
    if (yf_dict_getlen(dict) != 2 || !yf_dict_contains(dict, key3))
        return -1;

    puts("\nreplace()");
    val = yf_dict_replace(dict, key2, "O_o");
    if (val == NULL || strcmp(val, "b") != 0 || !yf_dict_contains(dict, key2))
        return -1;

    val = yf_dict_search(dict, key2);
    if (val == NULL || strcmp(val, "O_o") != 0)
        return -1;

    yf_seterr(YF_ERR_UNKNOWN, NULL);
    val = yf_dict_replace(dict, key1, " >o_; quac!");
    if (val != NULL || yf_geterr() != YF_ERR_NOTFND)
        return -1;

    it = YF_NILIT;
    printf("\nnext() begin:");
    for (;;) {
        val = yf_dict_next(dict, &it, &key);
        if (YF_IT_ISNIL(it))
            break;
        printf("\n key: %p\n val: %s", key, val);
    }
    printf("\nnext() end\n");

    it = YF_NILIT;
    key = (void *)65535ULL;
    printf("\nnext() begin (no key dst):");
    for (;;) {
        val = yf_dict_next(dict, &it, NULL);
        if (YF_IT_ISNIL(it))
            break;
        printf("\n key: %p\n val: %s", key, val);
    }
    printf("\nnext() end\n");

    printf("\nnext() begin (no iter, 3 calls):");
    for (size_t i = 0; i < 3; i++) {
        val = yf_dict_next(dict, NULL, &key);
        printf("\n key: %p\n val: %s", key, val);
    }
    printf("\nnext() end\n");

    key = (void *)127UL;
    printf("\nnext() begin (no iter, no key dst, 3 calls):");
    for (size_t i = 0; i < 3; i++) {
        val = yf_dict_next(dict, NULL, NULL);
        printf("\n key: %p\n val: %s", key, val);
    }
    printf("\nnext() end\n");

    printf("\neach() begin:");
    yf_dict_each(dict, dict_cb, NULL);
    printf("\neach() end\n");

    printf("\neach() begin (early break):");
    yf_dict_each(dict, dict_cb, (void *)1ULL);
    printf("\neach() end\n");

    puts("\nclear()");
    yf_dict_clear(dict);
    if (yf_dict_getlen(dict) != 0 || yf_dict_contains(dict, key3))
        return -1;

    puts("\ndeinit()");
    yf_dict_deinit(dict);

    puts("\ninit()");
    dict = yf_dict_init(NULL, NULL);

    size_t count = 1000;

    printf("\ninsert() #%zu\n", count);
    for (size_t i = 0; i < count; i++) {
        if (yf_dict_insert(dict, (void *)i, (void *)(i*i)) != 0)
            return -1;
    }

    if (yf_dict_getlen(dict) != count ||
        yf_dict_contains(dict, (void *)count) ||
        !yf_dict_contains(dict, (void *)(count-1)) ||
        !yf_dict_contains(dict, 0) ||
        !yf_dict_contains(dict, (void *)(count>>1)))
        return -1;

    printf("\nremove() #%zu\n", count);
    yf_seterr(YF_ERR_UNKNOWN, NULL);
    for (size_t i = 0; i < count; i++) {
        void *val = yf_dict_remove(dict, (void *)i);
        if (yf_geterr() == YF_ERR_NOTFND || (size_t)val != i*i)
            return -1;
    }

    if (yf_dict_getlen(dict) != 0 ||
        yf_dict_contains(dict, (void *)(count-1)) ||
        yf_dict_contains(dict, 0) ||
        yf_dict_contains(dict, (void *)(count>>1)))
        return -1;

    puts("\ndeinit()");
    yf_dict_deinit(dict);

    puts("\ninit(str)");
    dict = yf_dict_init(yf_hashstr, yf_cmpstr);

    count = 50;
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

    printf("\ninsert() #%zu\n", count);
    for (size_t i = 0; i < count; i++) {
        if (yf_dict_insert(dict, &strs[i*len], (void *)i) != 0)
            return -1;
    }

    puts("\ncontains()");
    for (size_t i = 0; i < count; i++) {
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

    printf("\nremove() #%zu\n", count);
    yf_seterr(YF_ERR_UNKNOWN, NULL);
    for (size_t i = 0; i < count; i++) {
        yf_dict_remove(dict, &strs[i*len]);
        if (yf_geterr() == YF_ERR_NOTFND)
            return -1;
    }

    free(strs);

    puts("\nclear()");
    yf_dict_clear(dict);
    if (yf_dict_getlen(dict) != 0)
        return -1;

    char *str2 = "string";
    if (yf_dict_insert(dict, str2, (void *)1) != 0 ||
        yf_dict_getlen(dict) != 1 || !yf_dict_contains(dict, str2) ||
        !yf_dict_contains(dict, "string") || yf_dict_contains(dict, "STRING"))
        return -1;

    char *str3 = malloc(16);
    assert(str3 != NULL);
    strcpy(str3, str2);
    if (yf_dict_insert(dict, str2, (void *)1) == 0 ||
        yf_dict_insert(dict, str3, (void *)2) == 0 ||
        !yf_dict_contains(dict, str3))
        return -1;

    puts("\nlookup()");
    void *dst = str3;
    size_t ival = (size_t)yf_dict_lookup(dict, &dst);
    if (ival != 1 || dst == str3 || dst != str2 || yf_dict_getlen(dict) != 1 ||
        !yf_dict_contains(dict, dst))
        return -1;

    puts("\ndelete()");
    dst = str3;
    ival = (size_t)yf_dict_delete(dict, &dst);
    if (ival != 1 || dst == str3 || dst != str2 || yf_dict_getlen(dict) != 0 ||
        yf_dict_contains(dict, dst) || yf_dict_contains(dict, str3))
        return -1;

    free(str3);

    puts("\ndeinit()");
    yf_dict_deinit(dict);

    puts("");
    return 0;
}

/* Publish-Subscribe test. */
#define YF_TEST_PUBSUB "pubsub"

struct ps1 { int val; };
struct ps2 { double val; };

static void pubsub_callb1(void *pub, int pubsub, void *arg)
{
    printf("\ngot: pub=%p (%d) pubsub=0x%x arg=%zu",
           pub, ((struct ps1 *)pub)->val, pubsub, (size_t)arg);
}

static void pubsub_callb2(void *pub, int pubsub, void *arg)
{
    printf("\ngot: pub=%p (%.4f) pubsub=0x%x arg=%zu",
           pub, ((struct ps2 *)pub)->val, pubsub, (size_t)arg);
}

static int test_pubsub(void)
{
    YF_TEST_SUBT;

    struct ps1 a1 = {43131};
    struct ps1 b1 = {-2219};
    struct ps2 a2 = {-9.991};

    if (yf_setpub(&a1, YF_PUBSUB_DEINIT|YF_PUBSUB_CHANGE) != 0)
        return -1;
    printf("\n(setpub() a1) mask: 0x%x", yf_checkpub(&a1));
    printf("\n(publish() a1)");
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    if (yf_setpub(&a1, YF_PUBSUB_DEINIT) != 0)
        return -1;
    printf("\n(setpub() a1) mask: 0x%x", yf_checkpub(&a1));

    if (yf_setpub(&a1, YF_PUBSUB_NONE) != 0)
        return -1;
    printf("\n(setpub() a1) mask: 0x%x", yf_checkpub(&a1));

    if (yf_setpub(&a1, YF_PUBSUB_DEINIT|YF_PUBSUB_CHANGE) != 0)
        return -1;
    printf("\n(setpub() a1) mask: 0x%x", yf_checkpub(&a1));

    if (yf_subscribe(&a1, &b1, YF_PUBSUB_DEINIT, pubsub_callb1, (void *)5) != 0)
        return -1;
    printf("\n\n(b1 subscribe() to a1 {DEINIT}");

    printf("\n(publish() a1)");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    if (yf_subscribe(&a1, &b1, YF_PUBSUB_CHANGE, pubsub_callb1, (void *)6) != 0)
        return -1;
    printf("\n\n(b1 subscribe() to a1 {CHANGE}");

    printf("\n(publish() a1)");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    if (yf_subscribe(&a1, &b1, YF_PUBSUB_DEINIT|YF_PUBSUB_CHANGE,
                     pubsub_callb1, (void *)7) != 0)
        return -1;
    printf("\n\n(b1 subscribe() to a1 {DEINIT|CHANGE}");

    printf("\n(publish() a1)");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    if (yf_subscribe(&a1, &b1, YF_PUBSUB_NONE, pubsub_callb1, (void *)8) != 0)
        return -1;
    printf("\n\n(b1 subscribe() to a1 {NONE}");

    printf("\n(publish() a1)");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    if (yf_setpub(&a2, YF_PUBSUB_DEINIT) != 0)
        return -1;
    printf("\n\n(setpub() a2) mask: 0x%x", yf_checkpub(&a2));

    if (yf_subscribe(&a1, &b1, YF_PUBSUB_DEINIT|YF_PUBSUB_CHANGE,
                     pubsub_callb1, (void *)9) != 0)
        return -1;
    printf("\n(b1 subscribe() to a1 {DEINIT|CHANGE}");

    if (yf_subscribe(&a2, &b1, YF_PUBSUB_DEINIT, pubsub_callb2, (void *)10)
        != 0)
        return -1;
    printf("\n(b1 subscribe() to a2 {DEINIT}");

    printf("\n(publish() a1)");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    printf("\n(publish() a2)");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    if (yf_subscribe(&a2, &b1, YF_PUBSUB_CHANGE, pubsub_callb2, (void *)11)
        != 0)
        return -1;
    printf("\n\n(b1 subscribe() to a2 {CHANGE}");

    printf("\n(publish() a2)");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    if (yf_subscribe(&a2, &a1, YF_PUBSUB_DEINIT, pubsub_callb2, (void *)12)
        != 0)
        return -1;
    printf("\n\n(a1 subscribe() to a2 {DEINIT}");

    if (yf_subscribe(&a2, &b1, YF_PUBSUB_DEINIT, pubsub_callb2, (void *)13)
        != 0)
        return -1;
    printf("\n(b1 subscribe() to a2 {DEINIT}");

    printf("\n(publish() a1)");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    printf("\n(publish() a2)");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    if (yf_subscribe(&a2, &a1, YF_PUBSUB_NONE, NULL, NULL) != 0)
        return -1;
    printf("\n\n(a1 subscribe() to a2 {NONE}");

    printf("\n(publish() a1)");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    printf("\n(publish() a2)");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    if (yf_subscribe(&a2, &b1, YF_PUBSUB_NONE, NULL, NULL) != 0)
        return -1;
    printf("\n(b1 subscribe() to a2 {NONE}");

    printf("\n(publish() a1)");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    printf("\n(publish() a2)");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    if (yf_subscribe(&a1, &b1, YF_PUBSUB_NONE, NULL, NULL) != 0)
        return -1;
    printf("\n(b1 subscribe() to a1 {NONE}");

    printf("\n(publish() a1)");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    printf("\n(publish() a2)");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    yf_setpub(&a2, YF_PUBSUB_NONE);
    yf_setpub(&a1, YF_PUBSUB_NONE);
    yf_publish(&a2, YF_PUBSUB_DEINIT);
    yf_publish(&a1, YF_PUBSUB_DEINIT);

    if (yf_checkpub(&a2) != YF_PUBSUB_NONE ||
        yf_checkpub(&a1) != YF_PUBSUB_NONE)
        return -1;

    puts("");
    return 0;
}

static const char *l_ids[] = {
    YF_TEST_ERROR,
    YF_TEST_CLOCK,
    YF_TEST_LIST,
    YF_TEST_DICT,
    YF_TEST_PUBSUB,
    YF_TEST_ALL
};

/* Test function. */
static int test(int argc, char *argv[])
{
    assert(argc > 0);

    size_t test_n;
    size_t results;

    if (strcmp(argv[0], YF_TEST_ERROR) == 0) {
        test_n = 1;
        results = test_error() == 0;
    } else if (strcmp(argv[0], YF_TEST_CLOCK) == 0) {
        test_n = 1;
        results = test_clock() == 0;
    } else if (strcmp(argv[0], YF_TEST_LIST) == 0) {
        test_n = 1;
        results = test_list() == 0;
    } else if (strcmp(argv[0], YF_TEST_DICT) == 0) {
        test_n = 1;
        results = test_dict() == 0;
    } else if (strcmp(argv[0], YF_TEST_PUBSUB) == 0) {
        test_n = 1;
        results = test_pubsub() == 0;
    } else if (strcmp(argv[0], YF_TEST_ALL) == 0) {
        int (*const tests[])(void) = {
            test_error,
            test_clock,
            test_list,
            test_dict,
            test_pubsub
        };
        test_n = sizeof tests / sizeof tests[0];

        results = 0;
        for (size_t i = 0; i < test_n; i++)
            results += tests[i]() == 0;
    } else {
        printf("! Error: unknown TEST_ID '%s'\n", argv[0]);
        printf("\nPossible values for TEST_ID:\n");
        for (size_t i = 0; i < (sizeof l_ids / sizeof l_ids[0]); i++)
            printf("* %s\n", l_ids[i]);
        printf("\n! No tests executed\n");
        return -1;
    }

    printf("\nDONE!\n\nNumber of tests executed: %zu\n", test_n);
    printf("> #%zu passed\n", results);
    printf("> #%zu failed\n", test_n - results);
    printf("\n(%.0f%% coverage)\n", (double)results / (double)test_n * 100.0);

    return 0;
}

const YF_test yf_g_test = {"com", test, l_ids, sizeof l_ids / sizeof l_ids[0]};
