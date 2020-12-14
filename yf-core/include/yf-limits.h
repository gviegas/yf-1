/*
 * YF
 * yf-limits.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_LIMITS_H
#define YF_YF_LIMITS_H

#include <stddef.h>

#include <yf/com/yf-defs.h>
#include <yf/com/yf-types.h>

#include "yf-context.h"

YF_DECLS_BEGIN

/* Limits. */
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
  } image;

  struct {
    unsigned res_max;
    unsigned unif_max;
    unsigned mut_max;
    unsigned img_max;
    unsigned sampd_max;
    unsigned sampr_max;
    unsigned isamp_max;
    size_t cpy_unif_sz_max;
    size_t cpy_mut_sz_max;
  } dtable;

  struct {
    unsigned attr_max;
    unsigned offs_max;
    unsigned strd_max;
  } vinput;

  struct {
    unsigned color_max;
    YF_dim2 dim_max;
    unsigned layer_max;
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
    unsigned draw_idx_max;
  } cmdbuf;
} YF_limits;

/* Gets the limits of a given context. */
const YF_limits *yf_getlimits(YF_context ctx);

YF_DECLS_END

#endif /* YF_YF_LIMITS_H */
