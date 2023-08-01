/*
 * YF
 * pass.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_PASS_H
#define YF_PASS_H

#include "yf-pass.h"
#include "image.h"
#include "vk.h"

struct yf_pass {
    yf_context_t *ctx;
    unsigned color_n;
    unsigned resolve_n;
    unsigned depth_n;
    yf_target_t **tgts;
    unsigned tgt_cap;
    unsigned tgt_n;
    unsigned tgt_i;
    VkRenderPass ren_pass;
};

struct yf_target {
    yf_pass_t *pass;
    yf_dim2_t dim;
    unsigned layers;
    yf_iview_t *iviews;
    unsigned iview_n;
    yf_image_t **imgs;
    unsigned *lays_base;
    VkFramebuffer framebuf;
};

/* Converts from a load op value. */
#define YF_LOADOP_FROM(op, to) do { \
    switch (op) { \
    case YF_LOADOP_UNDEF: \
        to = VK_ATTACHMENT_LOAD_OP_DONT_CARE; \
        break; \
    case YF_LOADOP_LOAD: \
        to = VK_ATTACHMENT_LOAD_OP_LOAD; \
        break; \
    default: \
        to = INT_MAX; \
    } } while (0)

/* Converts from a store op value. */
#define YF_STOREOP_FROM(op, to) do { \
    switch (op) { \
    case YF_STOREOP_UNDEF: \
        to = VK_ATTACHMENT_STORE_OP_DONT_CARE; \
        break; \
    case YF_STOREOP_STORE: \
        to = VK_ATTACHMENT_STORE_OP_STORE; \
        break; \
    default: \
        to = INT_MAX; \
    } } while (0)

#endif /* YF_PASS_H */
