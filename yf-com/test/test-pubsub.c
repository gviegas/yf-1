/*
 * YF
 * test-pubsub.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "test.h"
#include "yf-pubsub.h"

/* Variables & functions used by 'test_pubsub'. */
struct ps1 { const char *id; int val; };
struct ps2 { const char *id; double val; };

static void pubsub_cb1(void *pub, int pubsub, void *arg)
{
    struct ps1 *ps = pub;
    printf("\n- got: pub=%p <%s, %d> pubsub=0x%x arg=%zu -\n\n",
           pub, ps->id, ps->val, pubsub, (size_t)arg);
}

static void pubsub_cb2(void *pub, int pubsub, void *arg)
{
    struct ps2 *ps = pub;
    printf("\n- got: pub=%p <%s, %.4f> pubsub=0x%x arg=%zu -\n\n",
           pub, ps->id, ps->val, pubsub, (size_t)arg);
}

/* Tests publish-subscribe. */
int yf_test_pubsub(void)
{
    const struct ps1 a1 = {"a1", 43131};
    const struct ps1 b1 = {"b1", -2219};
    const struct ps2 a2 = {"a2", -9.991};

    char s[16] = {0};

    YF_TEST_PRINT("setpub", "&a1, PUBSUB_DEINIT|PUBSUB_CHANGE", "");
    if (yf_setpub(&a1, YF_PUBSUB_DEINIT|YF_PUBSUB_CHANGE) != 0)
        return -1;
    snprintf(s, sizeof s, "0x%x", yf_checkpub(&a1));
    YF_TEST_PRINT("checkpub", "&a1", s);

    YF_TEST_PRINT("publish", "&a1, PUBSUB_CHANGE", "");
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    YF_TEST_PRINT("publish", "&a1, PUBSUB_DEINIT", "");
    yf_publish(&a1, YF_PUBSUB_DEINIT);

    YF_TEST_PRINT("setpub", "&a1, PUBSUB_DEINIT", "");
    if (yf_setpub(&a1, YF_PUBSUB_DEINIT) != 0)
        return -1;
    snprintf(s, sizeof s, "0x%x", yf_checkpub(&a1));
    YF_TEST_PRINT("checkpub", "&a1", s);

    YF_TEST_PRINT("setpub", "&a1, PUBSUB_NONE", "");
    if (yf_setpub(&a1, YF_PUBSUB_NONE) != 0)
        return -1;
    snprintf(s, sizeof s, "0x%x", yf_checkpub(&a1));
    YF_TEST_PRINT("checkpub", "&a1", s);

    YF_TEST_PRINT("setpub", "&a1, PUBSUB_DEINIT|PUBSUB_CHANGE", "");
    if (yf_setpub(&a1, YF_PUBSUB_DEINIT|YF_PUBSUB_CHANGE) != 0)
        return -1;
    snprintf(s, sizeof s, "0x%x", yf_checkpub(&a1));
    YF_TEST_PRINT("checkpub", "&a1", s);

    YF_TEST_PRINT("subscribe", "&a1, &b1, PUBSUB_DEINIT, pubsub_cb1, 5", "");
    if (yf_subscribe(&a1, &b1, YF_PUBSUB_DEINIT, pubsub_cb1, (void *)5) != 0)
        return -1;

    YF_TEST_PRINT("publish", "&a1, PUBSUB_DEINIT", "");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    YF_TEST_PRINT("publish", "&a1, PUBSUB_CHANGE", "");
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    YF_TEST_PRINT("subscribe", "&a1, &b1, PUBSUB_CHANGE, pubsub_cb1, 6", "");
    if (yf_subscribe(&a1, &b1, YF_PUBSUB_CHANGE, pubsub_cb1, (void *)6) != 0)
        return -1;

    YF_TEST_PRINT("publish", "&a1, PUBSUB_DEINIT", "");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    YF_TEST_PRINT("publish", "&a1, PUBSUB_CHANGE", "");
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    YF_TEST_PRINT("subscribe",
                  "&a1, &b1, PUBSUB_DEINIT|PUBSUB_CHANGE, pubsub_cb1, 7", "");
    if (yf_subscribe(&a1, &b1, YF_PUBSUB_DEINIT|YF_PUBSUB_CHANGE,
                     pubsub_cb1, (void *)7) != 0)
        return -1;

    YF_TEST_PRINT("publish", "&a1, PUBSUB_DEINIT", "");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    YF_TEST_PRINT("publish", "&a1, PUBSUB_CHANGE", "");
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    YF_TEST_PRINT("subscribe", "&a1, &b1, PUBSUB_NONE, pubsub_cb1, 8", "");
    if (yf_subscribe(&a1, &b1, YF_PUBSUB_NONE, pubsub_cb1, (void *)8) != 0)
        return -1;

    YF_TEST_PRINT("publish", "&a1, PUBSUB_DEINIT", "");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    YF_TEST_PRINT("publish", "&a1, PUBSUB_CHANGE", "");
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    YF_TEST_PRINT("setpub", "&a2, PUBSUB_DEINIT", "");
    if (yf_setpub(&a2, YF_PUBSUB_DEINIT) != 0)
        return -1;
    snprintf(s, sizeof s, "0x%x", yf_checkpub(&a2));
    YF_TEST_PRINT("checkpub", "&a2", s);

    YF_TEST_PRINT("subscribe",
                  "&a1, &b1, PUBSUB_DEINIT|PUBSUB_CHANGE, pubsub_cb1, 9", "");
    if (yf_subscribe(&a1, &b1, YF_PUBSUB_DEINIT|YF_PUBSUB_CHANGE,
                     pubsub_cb1, (void *)9) != 0)
        return -1;

    YF_TEST_PRINT("subscribe", "&a2, &b1, PUBSUB_DEINIT, pubsub_cb2, 10", "");
    if (yf_subscribe(&a2, &b1, YF_PUBSUB_DEINIT, pubsub_cb2, (void *)10) != 0)
        return -1;

    YF_TEST_PRINT("publish", "&a1, PUBSUB_DEINIT", "");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    YF_TEST_PRINT("publish", "&a1, PUBSUB_CHANGE", "");
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    YF_TEST_PRINT("publish", "&a2, PUBSUB_DEINIT", "");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    YF_TEST_PRINT("subscribe", "&a2, &b1, PUBSUB_CHANGE, pubsub_cb2, 11", "");
    if (yf_subscribe(&a2, &b1, YF_PUBSUB_CHANGE, pubsub_cb2, (void *)11) != 0)
        return -1;

    YF_TEST_PRINT("publish", "&a2, PUBSUB_DEINIT", "");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    YF_TEST_PRINT("subscribe", "&a2, &a1, PUBSUB_DEINIT, pubsub_cb2, 12", "");
    if (yf_subscribe(&a2, &a1, YF_PUBSUB_DEINIT, pubsub_cb2, (void *)12) != 0)
        return -1;

    YF_TEST_PRINT("subscribe", "&a2, &b1, PUBSUB_DEINIT, pubsub_cb2, 13", "");
    if (yf_subscribe(&a2, &b1, YF_PUBSUB_DEINIT, pubsub_cb2, (void *)13) != 0)
        return -1;

    YF_TEST_PRINT("publish", "&a1, PUBSUB_DEINIT", "");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    YF_TEST_PRINT("publish", "&a1, PUBSUB_CHANGE", "");
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    YF_TEST_PRINT("publish", "&a2, PUBSUB_DEINIT", "");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    YF_TEST_PRINT("subscribe", "&a2, &a1, PUBSUB_NONE, NULL, NULL", "");
    if (yf_subscribe(&a2, &a1, YF_PUBSUB_NONE, NULL, NULL) != 0)
        return -1;

    YF_TEST_PRINT("publish", "&a1, PUBSUB_DEINIT", "");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    YF_TEST_PRINT("publish", "&a1, PUBSUB_CHANGE", "");
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    YF_TEST_PRINT("publish", "&a2, PUBSUB_DEINIT", "");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    YF_TEST_PRINT("subscribe", "&a2, &b1, PUBSUB_NONE, NULL, NULL", "");
    if (yf_subscribe(&a2, &b1, YF_PUBSUB_NONE, NULL, NULL) != 0)
        return -1;

    YF_TEST_PRINT("publish", "&a1, PUBSUB_DEINIT", "");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    YF_TEST_PRINT("publish", "&a1, PUBSUB_CHANGE", "");
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    YF_TEST_PRINT("publish", "&a2, PUBSUB_DEINIT", "");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    YF_TEST_PRINT("subscribe", "&a1, &b1, PUBSUB_NONE, NULL, NULL", "");
    if (yf_subscribe(&a1, &b1, YF_PUBSUB_NONE, NULL, NULL) != 0)
        return -1;

    YF_TEST_PRINT("publish", "&a1, PUBSUB_DEINIT", "");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    YF_TEST_PRINT("publish", "&a1, PUBSUB_CHANGE", "");
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    YF_TEST_PRINT("publish", "&a2, PUBSUB_DEINIT", "");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    YF_TEST_PRINT("setpub", "&a2, PUBSUB_NONE", "");
    yf_setpub(&a2, YF_PUBSUB_NONE);
    snprintf(s, sizeof s, "0x%x", yf_checkpub(&a2));
    YF_TEST_PRINT("checkpub", "&a2", s);

    YF_TEST_PRINT("setpub", "&a1, PUBSUB_NONE", "");
    yf_setpub(&a1, YF_PUBSUB_NONE);
    snprintf(s, sizeof s, "0x%x", yf_checkpub(&a1));
    YF_TEST_PRINT("checkpub", "&a1", s);

    YF_TEST_PRINT("publish", "&a2, PUBSUB_DEINIT", "");
    yf_publish(&a2, YF_PUBSUB_DEINIT);
    YF_TEST_PRINT("publish", "&a1, PUBSUB_DEINIT", "");
    yf_publish(&a1, YF_PUBSUB_DEINIT);

    if (yf_checkpub(&a2) != YF_PUBSUB_NONE ||
        yf_checkpub(&a1) != YF_PUBSUB_NONE)
        return -1;

    return 0;
}
