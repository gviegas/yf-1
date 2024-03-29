/*
 * YF
 * sampler.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "yf/com/yf-dict.h"
#include "yf/com/yf-error.h"

#include "sampler.h"
#include "context.h"

/* Default sampler. */
static const yf_sampler_t splr_ = {
    .wrapmode = {
        .u = YF_WRAPMODE_REPEAT,
        .v = YF_WRAPMODE_REPEAT,
        .w = YF_WRAPMODE_REPEAT
    },
    .filter = {
        .mag = YF_FILTER_NEAREST,
        .min = YF_FILTER_NEAREST,
        .mipmap = YF_FILTER_NEAREST
    }
};

/* Creates a sampler handle. */
static VkSampler create_handle(yf_context_t *ctx, const yf_sampler_t *splr)
{
    assert(ctx != NULL);
    assert(splr != NULL);

    VkSamplerCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.pNext = NULL;
    info.flags = 0;
    /* TODO: Additional members for setting some of these parameters. */
    info.mipLodBias = 0.0f;
    info.anisotropyEnable = VK_FALSE;
    info.maxAnisotropy = 0.0f;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_NEVER;
    info.minLod = 0.0f;
    info.maxLod = VK_LOD_CLAMP_NONE;
    info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;

    YF_WRAPMODE_FROM(splr->wrapmode.u, info.addressModeU);
    YF_WRAPMODE_FROM(splr->wrapmode.v, info.addressModeV);
    YF_WRAPMODE_FROM(splr->wrapmode.w, info.addressModeW);
    YF_FILTER_FROM(splr->filter.mag, info.magFilter);
    YF_FILTER_FROM(splr->filter.min, info.minFilter);
    YF_FILTER_MIP_FROM(splr->filter.mipmap, info.mipmapMode);

    assert(info.addressModeU != INT_MAX);
    assert(info.addressModeV != INT_MAX);
    assert(info.addressModeW != INT_MAX);
    assert(info.magFilter != INT_MAX);
    assert(info.minFilter != INT_MAX);
    assert(info.mipmapMode != INT_MAX);

    VkSampler sampler;
    VkResult res = vkCreateSampler(ctx->device, &info, NULL, &sampler);
    if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        return NULL;
    }
    return sampler;
}

/* Destroys the dictionary of 'yf_splrh_t' stored in a context. */
static void destroy_splrhs(yf_context_t *ctx)
{
    assert(ctx != NULL);

    yf_dict_t *splrhs = ctx->splr.priv;
    if (splrhs == NULL)
        return;

    yf_iter_t it = YF_NILIT;
    yf_splrh_t *splrh;
    while ((splrh = yf_dict_next(splrhs, &it, NULL)) != NULL) {
        vkDestroySampler(ctx->device, splrh->handle, NULL);
        free(splrh);
    }

    yf_dict_deinit(splrhs);
    ctx->splr.priv = NULL;
}

/* Hashes a 'yf_sampler_t'. */
static size_t hash_splr(const void *x)
{
    const yf_sampler_t *splr = x;
    return yf_hashv(splr, sizeof *splr, NULL);

    static_assert(sizeof(yf_sampler_t) == 6*sizeof(int), "!sizeof");
}

/* Compares a 'yf_sampler_t' to another. */
static int cmp_splr(const void *a, const void *b)
{
    const yf_sampler_t *splr1 = a;
    const yf_sampler_t *splr2 = b;
    return memcmp(splr1, splr2, sizeof *splr1);

    static_assert(sizeof(yf_sampler_t) == 6*sizeof(int), "!sizeof");
}

const yf_splrh_t *yf_sampler_get(yf_context_t *ctx, const yf_sampler_t *splr,
                                 const yf_splrh_t *subs)
{
    assert(ctx != NULL);

    if (ctx->splr.priv == NULL) {
        ctx->splr.priv = yf_dict_init(hash_splr, cmp_splr);
        if (ctx->splr.priv == NULL)
            return NULL;
        ctx->splr.deinit_callb = destroy_splrhs;
    }

    if (splr == NULL)
        splr = &splr_;

    if (subs != NULL) {
        if (cmp_splr(splr, &subs->splr) == 0)
            return subs;
        yf_sampler_unget(ctx, subs);
    }

    yf_dict_t *splrhs = ctx->splr.priv;
    yf_splrh_t *splrh = yf_dict_search(splrhs, splr);

    if (splrh == NULL) {
        splrh = malloc(sizeof *splrh);
        if (splrh == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return NULL;
        }
        if ((splrh->handle = create_handle(ctx, splr)) == VK_NULL_HANDLE) {
            free(splrh);
            return NULL;
        }
        splrh->splr = *splr;
        if (yf_dict_insert(splrhs, &splrh->splr, splrh) != 0) {
            vkDestroySampler(ctx->device, splrh->handle, NULL);
            free(splrh);
            return NULL;
        }
        splrh->count = 1;
    } else {
        splrh->count++;
    }

    return splrh;
}

void yf_sampler_unget(yf_context_t *ctx, const yf_splrh_t *splrh)
{
    assert(ctx != NULL);

    if (splrh == NULL)
        return;

    yf_dict_t *splrhs = ctx->splr.priv;
    assert(splrhs != NULL);

    yf_splrh_t *val = yf_dict_search(splrhs, &splrh->splr);
    assert(val != NULL);

    if (val->count == 1) {
        yf_dict_remove(splrhs, &splrh->splr);
        vkDestroySampler(ctx->device, val->handle, NULL);
        free(val);
    } else {
        val->count--;
    }
}
