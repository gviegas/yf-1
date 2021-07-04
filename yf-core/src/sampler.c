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

VkSampler yf_sampler_make(YF_context ctx, const YF_sampler *spl)
{
    assert(ctx != NULL);

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

    if (spl != NULL) {
        YF_WRAPMODE_FROM(spl->wrapmode.u, info.addressModeU);
        YF_WRAPMODE_FROM(spl->wrapmode.v, info.addressModeV);
        YF_WRAPMODE_FROM(spl->wrapmode.w, info.addressModeW);
        YF_FILTER_FROM(spl->filter.mag, info.magFilter);
        YF_FILTER_FROM(spl->filter.min, info.minFilter);
        YF_FILTER_MIP_FROM(spl->filter.mipmap, info.mipmapMode);

        assert(info.addressModeU != INT_MAX);
        assert(info.addressModeV != INT_MAX);
        assert(info.addressModeW != INT_MAX);
        assert(info.magFilter != INT_MAX);
        assert(info.minFilter != INT_MAX);
        assert(info.mipmapMode != INT_MAX);

    } else {
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.magFilter = VK_FILTER_NEAREST;
        info.minFilter = VK_FILTER_NEAREST;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }

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

/* Hashes a 'YF_splrh'. */
static size_t hash_splrh(const void *x)
{
    const YF_splrh *splrh = x;
    return yf_hashv(&splrh->splr, sizeof(YF_sampler), NULL);

    static_assert(sizeof(YF_sampler) == 6*sizeof(int), "!sizeof");
}

/* Compares a 'YF_splrh' to another. */
static int cmp_splrh(const void *a, const void *b)
{
    const YF_splrh *splrh1 = a;
    const YF_splrh *splrh2 = b;
    return memcmp(&splrh1->splr, &splrh2->splr, sizeof(YF_sampler));

    static_assert(sizeof(YF_sampler) == 6*sizeof(int), "!sizeof");
}

const YF_splrh *yf_sampler_get(YF_context ctx, const YF_sampler *splr)
{
    assert(ctx != NULL);

    /* TODO */
    return NULL;
}

void yf_sampler_unget(YF_context ctx, const YF_splrh *splrh)
{
    assert(ctx != NULL);
    assert(splrh != NULL);

    /* TODO */
}
