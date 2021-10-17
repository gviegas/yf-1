/*
 * YF
 * pubsub.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf-pubsub.h"
#include "yf-dict.h"
#include "yf-error.h"

/* TODO: Thread-safe. */

/* Publisher variables. */
typedef struct {
    unsigned pubsub_mask;
    YF_dict subs;
} T_pub;

/* Subscriber variables. */
typedef struct {
    unsigned pubsub_mask;
    void (*callb)(void *, int, void *);
    void *arg;
} T_sub;

/* Dictionary containing all publishers. */
static YF_dict pubs_ = NULL;

/* On exit deinitialization. */
static void deinit(void)
{
    if (pubs_ == NULL)
        return;

    YF_iter it = YF_NILIT;
    T_pub *pub;
    while ((pub = yf_dict_next(pubs_, &it, NULL)) != NULL) {
        YF_iter it2 = YF_NILIT;
        T_sub *sub;
        while ((sub = yf_dict_next(pub->subs, &it2, NULL)) != NULL)
            free(sub);
        yf_dict_deinit(pub->subs);
        free(pub);
    }

    yf_dict_deinit(pubs_);
}

int yf_setpub(const void *pub, unsigned pubsub_mask)
{
    if (pub == NULL) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    if (pubs_ == NULL) {
        if ((pubs_ = yf_dict_init(NULL, NULL)) == NULL)
            return -1;

        atexit(deinit);
    }

    T_pub *val = yf_dict_search(pubs_, pub);

    /* removal */
    if (pubsub_mask == YF_PUBSUB_NONE) {
        if (val != NULL) {
            yf_dict_remove(pubs_, pub);

            YF_iter it = YF_NILIT;
            T_sub *sub;
            while ((sub = yf_dict_next(val->subs, &it, NULL)) != NULL)
                free(sub);

            yf_dict_deinit(val->subs);
            free(val);
        }

        return 0;
    }

    /* insertion/update */
    if (val == NULL) {
        if ((val = malloc(sizeof *val)) == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }

        val->subs = yf_dict_init(NULL, NULL);
        if (val->subs == NULL) {
            free(val);
            return -1;
        }

        if (yf_dict_insert(pubs_, pub, val) != 0) {
            yf_dict_deinit(val->subs);
            free(val);
            return -1;
        }
    }

    val->pubsub_mask = pubsub_mask;

    return 0;
}

unsigned yf_checkpub(const void *pub)
{
    assert(pub != NULL);

    if (pubs_ == NULL)
        return YF_PUBSUB_NONE;

    T_pub *val = yf_dict_search(pubs_, pub);

    return val != NULL ? val->pubsub_mask : YF_PUBSUB_NONE;
}

void yf_publish(const void *pub, int pubsub)
{
    assert(pub != NULL);
    assert(pubs_ != NULL);

    T_pub *val = yf_dict_search(pubs_, pub);

    if (val == NULL)
        return;

    YF_iter it = YF_NILIT;
    T_sub *sub;
    while ((sub = yf_dict_next(val->subs, &it, NULL)) != NULL) {
        if (sub->pubsub_mask & pubsub)
            sub->callb((void *)pub, pubsub, sub->arg);
    }
}

int yf_subscribe(const void *pub, const void *sub, unsigned pubsub_mask,
                 void (*callb)(void *pub, int pubsub, void *arg), void *arg)
{
    assert(pub != NULL);
    assert(pubsub_mask == YF_PUBSUB_NONE || callb != NULL);

    if (sub == NULL || pubs_ == NULL) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    T_pub *pv = yf_dict_search(pubs_, pub);

    if (pv == NULL)
        return -1;

    T_sub *sv = yf_dict_search(pv->subs, sub);

    /* removal */
    if (pubsub_mask == YF_PUBSUB_NONE) {
        if (sv != NULL) {
            yf_dict_remove(pv->subs, sub);
            free(sv);
        }

        return 0;
    }

    /* insertion/update */
    if (sv == NULL) {
        if ((sv = malloc(sizeof *sv)) == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }

        if (yf_dict_insert(pv->subs, sub, sv) != 0) {
            free(sv);
            return -1;
        }
    }

    /* XXX: May want to compare pub/sub masks. */
    sv->pubsub_mask = pubsub_mask;
    sv->callb = callb;
    sv->arg = arg;

    return 0;
}
