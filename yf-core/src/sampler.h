/*
 * YF
 * sampler.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_SAMPLER_H
#define YF_SAMPLER_H

#include "yf-context.h"
#include "yf-sampler.h"
#include "vk.h"

/* Makes a new sampler from a given 'YF_sampler'.
   The caller is responsible for the object's destruction. */
VkSampler yf_sampler_make(YF_context ctx, const YF_sampler *spl);

/* Converts from a 'YF_WRAPMODE' value. */
#define YF_WRAPMODE_FROM(wm, to) do { \
    switch (wm) { \
    case YF_WRAPMODE_CLAMP: \
        to = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; \
        break; \
    case YF_WRAPMODE_MIRROR: \
        to = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; \
        break; \
    case YF_WRAPMODE_REPEAT: \
        to = VK_SAMPLER_ADDRESS_MODE_REPEAT; \
        break; \
    default: \
        to = INT_MAX; \
    } } while (0)

/* Converts from a 'YF_FILTER' value. */
#define YF_FILTER_FROM(ft, to) do { \
    switch (ft) { \
    case YF_FILTER_NEAREST: \
        to = VK_FILTER_NEAREST; \
        break; \
    case YF_FILTER_LINEAR: \
        to = VK_FILTER_LINEAR; \
        break; \
    default: \
        to = INT_MAX; \
    } } while (0)

/* Converts from a 'YF_FILTER' value (mipmap). */
#define YF_FILTER_MIP_FROM(ft, to) do { \
    switch (ft) { \
    case YF_FILTER_NEAREST: \
        to = VK_SAMPLER_MIPMAP_MODE_NEAREST; \
        break; \
    case YF_FILTER_LINEAR: \
        to = VK_SAMPLER_MIPMAP_MODE_LINEAR; \
        break; \
    default: \
        to = INT_MAX; \
    } } while (0)

#endif /* YF_SAMPLER_H */
