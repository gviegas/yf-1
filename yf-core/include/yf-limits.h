/*
 * YF
 * yf-limits.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
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
typedef struct yf_limits {
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
        unsigned sample_mask_img;
    } image;

    struct {
        unsigned stg_res_max;
        unsigned unif_max;
        unsigned mut_max;
        unsigned img_max;
        unsigned spld_max;
        unsigned splr_max;
        unsigned ispl_max;
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
        yf_dim2_t dim_max;
        unsigned layer_max;
        unsigned sample_mask_clr;
        unsigned sample_mask_dep;
        unsigned sample_mask_sten;
    } pass;

    struct {
        unsigned max;
        yf_dim2_t dim_max;
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
        yf_dim3_t disp_dim_max;
    } cmdbuf;
} yf_limits_t;

/**
 * Gets the limits of a given context.
 *
 * @param ctx: The context.
 * @return: The limits for the context.
 */
const yf_limits_t *yf_getlimits(yf_context_t *ctx);

YF_DECLS_END

#endif /* YF_YF_LIMITS_H */
