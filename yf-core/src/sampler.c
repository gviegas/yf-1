/*
 * YF
 * sampler.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
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
static const YF_sampler splr_ = {
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
static VkSampler create_handle(YF_context ctx, const YF_sampler *splr)
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
    info.maxLod = 0.0f;
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

/* Destroys the dictionary of 'YF_splrh' stored in a context. */
static void destroy_splrhs(YF_context ctx)
{
    assert(ctx != NULL);

    YF_dict splrhs = ctx->splr.priv;
    if (splrhs == NULL)
        return;

    YF_iter it = YF_NILIT;
    YF_splrh *splrh;
    while ((splrh = yf_dict_next(splrhs, &it, NULL)) != NULL) {
        vkDestroySampler(ctx->device, splrh->handle, NULL);
        free(splrh);
    }

    yf_dict_deinit(splrhs);
    ctx->splr.priv = NULL;
}

/* Hashes a 'YF_sampler'. */
static size_t hash_splr(const void *x)
{
    const YF_sampler *splr = x;
    return yf_hashv(splr, sizeof *splr, NULL);

    static_assert(sizeof(YF_sampler) == 6*sizeof(int), "!sizeof");
}

/* Compares a 'YF_sampler' to another. */
static int cmp_splr(const void *a, const void *b)
{
    const YF_sampler *splr1 = a;
    const YF_sampler *splr2 = b;
    return memcmp(splr1, splr2, sizeof *splr1);

    static_assert(sizeof(YF_sampler) == 6*sizeof(int), "!sizeof");
}

const YF_splrh *yf_sampler_get(YF_context ctx, const YF_sampler *splr,
                               const YF_splrh *subs)
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

    YF_dict splrhs = ctx->splr.priv;
    YF_splrh *splrh = yf_dict_search(splrhs, splr);

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

void yf_sampler_unget(YF_context ctx, const YF_splrh *splrh)
{
    assert(ctx != NULL);

    if (splrh == NULL)
        return;

    YF_dict splrhs = ctx->splr.priv;
    assert(splrhs != NULL);

    YF_splrh *val = yf_dict_search(splrhs, &splrh->splr);
    assert(val != NULL);

    if (val->count == 1) {
        yf_dict_remove(splrhs, &splrh->splr);
        vkDestroySampler(ctx->device, val->handle, NULL);
        free(val);
    } else {
        val->count--;
    }
}
