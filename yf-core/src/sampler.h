/*
 * YF
 * sampler.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_SAMPLER_H
#define YF_SAMPLER_H

#include "yf-context.h"
#include "yf-sampler.h"
#include "vk.h"

/* Managed sampler. */
typedef struct yf_splrh {
    VkSampler handle;
    yf_sampler_t splr;
    unsigned count;
} yf_splrh_t;

/* Gets a managed sampler.
   If 'splr' is 'NULL', the default sampler is used. A non-null 'subs'
   argument indicates that the caller is exchanging samplers. */
const yf_splrh_t *yf_sampler_get(yf_context_t *ctx, const yf_sampler_t *splr,
                                 const yf_splrh_t *subs);

/* Ungets a managed sampler.
   When exchanging samplers, one should use 'sampler_get()' instead to avoid
   unnecessary recreation of sampler handles. */
void yf_sampler_unget(yf_context_t *ctx, const yf_splrh_t *splrh);

/* Converts from a 'YF_WRAPMODE' value. */
#define YF_WRAPMODE_FROM(wm, to) do { \
    switch (wm) { \
    case YF_WRAPMODE_REPEAT: \
        to = VK_SAMPLER_ADDRESS_MODE_REPEAT; \
        break; \
    case YF_WRAPMODE_MIRROR: \
        to = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; \
        break; \
    case YF_WRAPMODE_CLAMP: \
        to = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; \
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
