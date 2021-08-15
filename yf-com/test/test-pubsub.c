/*
 * YF
 * test-pubsub.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "yf-pubsub.h"

/* Variables & functions used by 'test_pubsub'. */
struct ps1 { const char *id; int val; };
struct ps2 { const char *id; double val; };

static void pubsub_cb1(void *pub, int pubsub, void *arg)
{
    struct ps1 *ps = pub;
    printf("got: pub=%p <%s, %d> pubsub=0x%x arg=%zu\n\n",
           pub, ps->id, ps->val, pubsub, (size_t)arg);
}

static void pubsub_cb2(void *pub, int pubsub, void *arg)
{
    struct ps2 *ps = pub;
    printf("got: pub=%p <%s, %.4f> pubsub=0x%x arg=%zu\n\n",
           pub, ps->id, ps->val, pubsub, (size_t)arg);
}

/* Tests publish-subscribe. */
int yf_test_pubsub(void)
{
    const struct ps1 a1 = {"a1", 43131};
    const struct ps1 b1 = {"b1", -2219};
    const struct ps2 a2 = {"a2", -9.991};

    puts("(setpub &a1,DEINIT|CHANGE)\n");
    if (yf_setpub(&a1, YF_PUBSUB_DEINIT|YF_PUBSUB_CHANGE) != 0)
        return -1;
    printf("(checkpub &a1)\n 0x%x\n\n", yf_checkpub(&a1));

    puts("(publish &a1,CHANGE)\n");
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    puts("(publish &a1,DEINIT)\n");
    yf_publish(&a1, YF_PUBSUB_DEINIT);

    puts("(setpub &a1,DEINIT)\n");
    if (yf_setpub(&a1, YF_PUBSUB_DEINIT) != 0)
        return -1;
    printf("(checkpub &a1)\n 0x%x\n\n", yf_checkpub(&a1));

    puts("(setpub &a1,NONE)\n");
    if (yf_setpub(&a1, YF_PUBSUB_NONE) != 0)
        return -1;
    printf("(checkpub &a1)\n 0x%x\n\n", yf_checkpub(&a1));

    puts("(setpub &a1,DEINIT|CHANGE)\n");
    if (yf_setpub(&a1, YF_PUBSUB_DEINIT|YF_PUBSUB_CHANGE) != 0)
        return -1;
    printf("(checkpub &a1)\n 0x%x\n\n", yf_checkpub(&a1));

    puts("(subscribe &a1,&b1,DEINIT,pubsub_cb1,5)\n");
    if (yf_subscribe(&a1, &b1, YF_PUBSUB_DEINIT, pubsub_cb1, (void *)5) != 0)
        return -1;

    puts("(publish &a1,DEINIT)\n");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    puts("(publish &a1,CHANGE)\n");
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    puts("(subscribe &a1,&b1,CHANGE,pubsub_cb1,6)\n");
    if (yf_subscribe(&a1, &b1, YF_PUBSUB_CHANGE, pubsub_cb1, (void *)6) != 0)
        return -1;

    puts("(publish &a1,DEINIT)\n");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    puts("(publish &a1,CHANGE)\n");
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    puts("(subscribe &a1,&b1,DEINIT|CHANGE,pubsub_cb1,7)\n");
    if (yf_subscribe(&a1, &b1, YF_PUBSUB_DEINIT|YF_PUBSUB_CHANGE,
                     pubsub_cb1, (void *)7) != 0)
        return -1;

    puts("(publish &a1,DEINIT)\n");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    puts("(publish &a1,CHANGE)\n");
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    puts("(subscribe &a1,&b1,NONE,pubsub_cb1,8)\n");
    if (yf_subscribe(&a1, &b1, YF_PUBSUB_NONE, pubsub_cb1, (void *)8) != 0)
        return -1;

    puts("(publish &a1,DEINIT)\n");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    puts("(publish &a1,CHANGE)\n");
    yf_publish(&a1, YF_PUBSUB_CHANGE);

    puts("(setpub &a2,DEINIT)\n");
    if (yf_setpub(&a2, YF_PUBSUB_DEINIT) != 0)
        return -1;
    printf("(checkpub &a2)\n 0x%x\n\n", yf_checkpub(&a2));

    puts("(subscribe &a1,&b1,DEINIT|CHANGE,pubsub_cb1,9)\n");
    if (yf_subscribe(&a1, &b1, YF_PUBSUB_DEINIT|YF_PUBSUB_CHANGE,
                     pubsub_cb1, (void *)9) != 0)
        return -1;

    puts("(subscribe &a2,&b1,DEINIT,pubsub_cb2,10)\n");
    if (yf_subscribe(&a2, &b1, YF_PUBSUB_DEINIT, pubsub_cb2, (void *)10)
        != 0)
        return -1;

    puts("(publish &a1,DEINIT)\n");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    puts("(publish &a1,CHANGE)\n");
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    puts("(publish &a2,DEINIT)\n");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    puts("(subscribe &a2,&b1,CHANGE,pubsub_cb2,11)\n");
    if (yf_subscribe(&a2, &b1, YF_PUBSUB_CHANGE, pubsub_cb2, (void *)11)
        != 0)
        return -1;

    puts("(publish &a2,DEINIT)\n");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    puts("(subscribe &a2,&a1,DEINIT,pubsub_cb2,12)\n");
    if (yf_subscribe(&a2, &a1, YF_PUBSUB_DEINIT, pubsub_cb2, (void *)12)
        != 0)
        return -1;

    puts("(subscribe &a2,&b1,DEINIT,pubsub_cb2,13)\n");
    if (yf_subscribe(&a2, &b1, YF_PUBSUB_DEINIT, pubsub_cb2, (void *)13)
        != 0)
        return -1;

    puts("(publish &a1,DEINIT)\n");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    puts("(publish &a1,CHANGE)\n");
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    puts("(publish &a2,DEINIT)\n");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    puts("(subscribe &a2,&a1,NONE,NULL,NULL)\n");
    if (yf_subscribe(&a2, &a1, YF_PUBSUB_NONE, NULL, NULL) != 0)
        return -1;

    puts("(publish &a1,DEINIT)\n");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    puts("(publish &a1,CHANGE)\n");
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    puts("(publish &a2,DEINIT)\n");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    puts("(subscribe &a2,&b1,NONE,NULL,NULL)\n");
    if (yf_subscribe(&a2, &b1, YF_PUBSUB_NONE, NULL, NULL) != 0)
        return -1;

    puts("(publish &a1,DEINIT)\n");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    puts("(publish &a1,CHANGE)\n");
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    puts("(publish &a2,DEINIT)\n");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    puts("(subscribe &a1,&b1,NONE,NULL,NULL)\n");
    if (yf_subscribe(&a1, &b1, YF_PUBSUB_NONE, NULL, NULL) != 0)
        return -1;

    puts("(publish &a1,DEINIT)\n");
    yf_publish(&a1, YF_PUBSUB_DEINIT);
    puts("(publish &a1,CHANGE)\n");
    yf_publish(&a1, YF_PUBSUB_CHANGE);
    puts("(publish &a2,DEINIT)\n");
    yf_publish(&a2, YF_PUBSUB_DEINIT);

    puts("(setpub &a2,NONE)\n");
    yf_setpub(&a2, YF_PUBSUB_NONE);
    puts("(setpub &a1,NONE)\n");
    yf_setpub(&a1, YF_PUBSUB_NONE);

    puts("(publish &a2,DEINIT)\n");
    yf_publish(&a2, YF_PUBSUB_DEINIT);
    puts("(publish &a1,DEINIT)\n");
    yf_publish(&a1, YF_PUBSUB_DEINIT);

    if (yf_checkpub(&a2) != YF_PUBSUB_NONE ||
        yf_checkpub(&a1) != YF_PUBSUB_NONE)
        return -1;

    return 0;
}
