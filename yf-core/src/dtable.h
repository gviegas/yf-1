/*
 * YF
 * dtable.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_DTABLE_H
#define YF_DTABLE_H

#include "yf/com/yf-dict.h"

#include "yf-dtable.h"
#include "vk.h"

typedef struct YF_dtable_o {
    YF_context ctx;
    YF_dentry *entries;
    unsigned entry_n;

    struct {
        unsigned unif;
        unsigned mut;
        unsigned img;
        unsigned spld;
        unsigned splr;
        unsigned ispl;
    } count;

    YF_dict iviews;
    YF_dict samplers;
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    VkDescriptorSet *sets;
    unsigned set_n;
} YF_dtable_o;

/* Converts from a 'YF_DTYPE' value. */
#define YF_DTYPE_FROM(dtp, to) do { \
    switch (dtp) { \
    case YF_DTYPE_UNIFORM: \
        to = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; \
        break; \
    case YF_DTYPE_MUTABLE: \
        to = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; \
        break; \
    case YF_DTYPE_IMAGE: \
        to = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; \
        break; \
    case YF_DTYPE_SAMPLED: \
        to = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; \
        break; \
    case YF_DTYPE_SAMPLER: \
        to = VK_DESCRIPTOR_TYPE_SAMPLER; \
        break; \
    case YF_DTYPE_ISAMPLER: \
        to = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; \
        break; \
    default: \
        to = INT_MAX; \
    } } while (0)

#endif /* YF_DTABLE_H */
