/*
 * YF
 * yf-limits.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_LIMITS_H
#define YF_YF_LIMITS_H

#include <stddef.h>

#include "yf/com/yf-defs.h"
#include "yf/com/yf-types.h"

#include "yf-context.h"

YF_DECLS_BEGIN

/**
 * Limits.
 */
typedef struct YF_limits {
    struct {
        size_t obj_max;
    } memory;

    struct {
        size_t sz_max;
    } buffer;

    struct {
        unsigned dim_1d_max;
        unsigned dim_2d_max;
        unsigned dim_3d_max;
        unsigned layer_max;
        unsigned sample_mask_clr;
        unsigned sample_mask_dep;
        unsigned sample_mask_sten;
        unsigned sample_mask_mut;
    } image;

    struct {
        unsigned stg_res_max;
        unsigned unif_max;
        unsigned mut_max;
        unsigned img_max;
        unsigned sampd_max;
        unsigned sampr_max;
        unsigned isamp_max;
        size_t cpy_unif_align_min;
        size_t cpy_unif_sz_max;
        size_t cpy_mut_align_min;
        size_t cpy_mut_sz_max;
    } dtable;

    struct {
        unsigned attr_max;
        unsigned off_max;
        unsigned strd_max;
    } vinput;

    struct {
        unsigned color_max;
        YF_dim2 dim_max;
        unsigned layer_max;
        unsigned sample_mask_clr;
        unsigned sample_mask_dep;
        unsigned sample_mask_sten;
    } pass;

    struct {
        unsigned max;
        YF_dim2 dim_max;
        float bounds_min;
        float bounds_max;
    } viewport;

    struct {
        unsigned dtable_max;
        unsigned vinput_max;
    } state;

    struct {
        unsigned vert_out_max;
        unsigned frag_in_max;
        float point_sz_min;
        float point_sz_max;
        float point_sz_gran;
        float line_wdt_min;
        float line_wdt_max;
        float line_wdt_gran;
    } shader;

    struct {
        unsigned draw_idx_max;
        YF_dim3 disp_dim_max;
    } cmdbuf;
} YF_limits;

/**
 * Gets the limits of a given context.
 *
 * @param ctx: The context.
 * @return: The limits for the context.
 */
const YF_limits *yf_getlimits(YF_context ctx);

YF_DECLS_END

#endif /* YF_YF_LIMITS_H */
