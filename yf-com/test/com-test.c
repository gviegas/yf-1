/*
 * YF
 * com-test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "test.h"
#include "yf-com.h"

#define YF_TEST_ALL "all"
#define YF_TEST_SUBLN "..............................."
#define YF_TEST_SUBT \
    printf("%s\n%.*s\n", __func__, (int)strlen(__func__), YF_TEST_SUBLN)

/* Error test. */
#define YF_TEST_ERROR "error"

static int test_error(void)
{
    YF_TEST_SUBT;

    int err;
    const size_t n = 72;
    char info[n];

    if (yf_geterrinfo(info, n) != info)
        return -1;

    printf("\nerr code/info: %d/%s\n", yf_geterr(), info);

    yf_seterr(YF_ERR_NOTFND, "test");
    err = yf_geterr();
    if (err != YF_ERR_NOTFND)
        return -1;

    if (yf_geterrinfo(info, n) != info)
        return -1;

    printf("err code/info: %d/%s\n", yf_geterr(), info);

    yf_seterr(YF_ERR_LIMIT, NULL);
    if (err == yf_geterr())
        return -1;
    if (yf_geterr() != YF_ERR_LIMIT)
        return -1;

    if (yf_geterrinfo(info, n) != info)
        return -1;

    printf("err code/info: %d/%s\n", yf_geterr(), info);

    yf_seterr(YF_ERR_OTHER, "TEST");
    err = yf_geterr();
    if (err != YF_ERR_OTHER)
        return -1;

    if (yf_geterrinfo(info, n) != info)
        return -1;

    printf("err code/info: %d/%s\n", yf_geterr(), info);

    puts("");
    return 0;
}

/* Clock test. */
#define YF_TEST_CLOCK "clock"

static int test_clock(void)
{
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
#define YF_TEST_LIST "list"

static int list_cb(void *val, void *arg)
{
    printf("%c,%p ", *(int *)val, arg);
    return 0;
}

static int test_list(void)
{
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

    yf_list_insertat(ls, NULL, &a);
    if (yf_list_getlen(ls) != 1)
        return -1;
    yf_list_insertat(ls, NULL, &b);
    yf_list_insertat(ls, NULL, &c);
    yf_list_insertat(ls, NULL, &d);
    if (yf_list_getlen(ls) != 4)
        return -1;

    printf("\n(list beg) ");
    yf_list_each(ls, list_cb, NULL);
    puts("(list end)");

    yf_list_clear(ls);

    it = YF_NILIT;
    yf_list_insertat(ls, &it, &a);
    if (yf_list_getlen(ls) != 1)
        return -1;
    yf_list_insertat(ls, &it, &b);
    yf_list_insertat(ls, &it, &c);
    yf_list_insertat(ls, &it, &d);
    if (yf_list_getlen(ls) != 4)
        return -1;

    printf("(list beg) ");
    yf_list_each(ls, list_cb, NULL);
    puts("(list end)");

    if (YF_IT_ISNIL(it))
        return -1;
    if (yf_list_next(ls, &it) != NULL)
        return -1;
    if (!YF_IT_ISNIL(it))
        return -1;

    int *v;
    puts("");
    while ((v = yf_list_next(ls, NULL)) != NULL) {
        if (*v != *(int *)yf_list_removeat(ls, NULL))
            return -1;
        printf("(list beg) ");
        yf_list_each(ls, list_cb, NULL);
        puts("(list end)");
    }

    yf_list_insert(ls, &a);
    yf_list_insert(ls, &b);
    yf_list_insert(ls, &c);
    yf_list_insert(ls, &d);

    it = YF_NILIT;
    puts("");
    while ((v = yf_list_next(ls, NULL)) != NULL) {
        if (*v != *(int *)yf_list_removeat(ls, &it))
            return -1;
        printf("(list beg) ");
        yf_list_each(ls, list_cb, NULL);
        puts("(list end)");
    }

    it = YF_NILIT;
    yf_list_insertat(ls, &it, &a);
    yf_list_insertat(ls, &it, &b);
    yf_list_insertat(ls, &it, &c);

    printf("\n(list beg) ");
    yf_list_each(ls, list_cb, NULL);
    puts("(list end)");

    it = YF_NILIT;
    for (;;) {
        v = yf_list_next(ls, &it);
        if (*v == b)
            break;
    }
    if (!yf_list_contains(ls, &b))
        return -1;
    yf_list_removeat(ls, &it);
    if (yf_list_contains(ls, &b))
        return -1;
    if (yf_list_getlen(ls) != 2)
        return -1;

    printf("(list beg) ");
    yf_list_each(ls, list_cb, NULL);
    puts("(list end)");

    yf_list_deinit(ls);

    puts("");
    return 0;
}

/* Hashset test. */
#define YF_TEST_HASHSET "hashset"

static size_t hashset_hash(const void *x)
{
    if (x == NULL)
        return 1<<24;
    const char *s = x;
    return ((size_t)s[0] << 16) | ((size_t)s[1] << 8) | (size_t)s[2];
}

static int hashset_cmp(const void *a, const void *b)
{
    return strcmp(a, b);
}

static int hashset_cb(void *val, void *arg)
{
    printf("%s,%p ", (const char *)val, arg);
    if (strcmp(val, "_B_") == 0) {
        printf("{early break} ");
        return 1;
    }
    return 0;
}

static int test_hashset(void)
{
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

/* Publish-Subscribe test. */
#define YF_TEST_PUBSUB "pubsub"

struct ps1 { int val; };
struct ps2 { double val; };

static void pubsub_callb1(void *pub, int pubsub, void *arg)
{
    printf("\ngot: pub=%p (%d) pubsub=0x%x arg=%lu",
           pub, ((struct ps1 *)pub)->val, pubsub, (size_t)arg);
}

static void pubsub_callb2(void *pub, int pubsub, void *arg)
{
    printf("\ngot: pub=%p (%.4f) pubsub=0x%x arg=%lu",
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
    YF_TEST_HASHSET,
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
    } else if (strcmp(argv[0], YF_TEST_HASHSET) == 0) {
        test_n = 1;
        results = test_hashset() == 0;
    } else if (strcmp(argv[0], YF_TEST_PUBSUB) == 0) {
        test_n = 1;
        results = test_pubsub() == 0;
    } else if (strcmp(argv[0], YF_TEST_ALL) == 0) {
        int (*const tests[])(void) = {
            test_error,
            test_clock,
            test_list,
            test_hashset,
            test_pubsub
        };
        test_n = sizeof tests / sizeof tests[0];

        results = 0;
        for (size_t i = 0; i < test_n; ++i)
            results += tests[i]() == 0;
    } else {
        printf("! Error: no test named '%s'\n", argv[0]);
        printf("\nTry one of the following:\n");
        for (size_t i = 0; i < (sizeof l_ids / sizeof l_ids[0]); ++i)
            printf("* %s\n", l_ids[i]);
        printf("\n! No tests executed\n");
        return -1;
    }

    printf("\nDONE!\n\nNumber of tests executed: %lu\n", test_n);
    printf("> #%lu passed\n", results);
    printf("> #%lu failed\n", test_n - results);
    printf("\n(%.0f%% coverage)\n", (double)results / (double)test_n * 100.0);

    return 0;
}

const YF_test yf_g_test = {"com", test, l_ids, sizeof l_ids / sizeof l_ids[0]};
