/*
 * YF
 * limits.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_LIMITS_H
#define YF_LIMITS_H

#include "yf-limits.h"

#ifdef YF_DEBUG
# define YF_LIMITS_PRINT(lim_p) do { \
   printf("\n-- Limits (debug) --"); \
   printf("\nmemory - max objects: %lu", (lim_p)->memory.obj_max); \
   printf("\nbuffer - max size: %lu", (lim_p)->buffer.sz_max); \
   printf("\nimage - max 1d: %u", (lim_p)->image.dim_1d_max); \
   printf("\nimage - max 2d: %u", (lim_p)->image.dim_2d_max); \
   printf("\nimage - max 3d: %u", (lim_p)->image.dim_3d_max); \
   printf("\nimage - max layers: %u", (lim_p)->image.layer_max); \
   printf("\ndtable - max resources: %u", (lim_p)->dtable.res_max); \
   printf("\ndtable - max uniforms buffers: %u", (lim_p)->dtable.unif_max); \
   printf("\ndtable - max mutables buffers: %u", (lim_p)->dtable.mut_max); \
   printf("\ndtable - max r/w images: %u", (lim_p)->dtable.img_max); \
   printf("\ndtable - max sampled images: %u", (lim_p)->dtable.sampd_max); \
   printf("\ndtable - max samplers: %u", (lim_p)->dtable.sampr_max); \
   printf("\ndtable - max image+samplers: %u", (lim_p)->dtable.isamp_max); \
   printf("\ndtable - max copy size (uniform): %lu", \
      (lim_p)->dtable.cpy_unif_sz_max); \
   printf("\ndtable - max copy size (mutable): %lu", \
      (lim_p)->dtable.cpy_mut_sz_max); \
   printf("\nvinput - max input attributes: %u", (lim_p)->vinput.attr_max); \
   printf("\nvinput - max input offset: %u", (lim_p)->vinput.offs_max); \
   printf("\nvinput - max input stride: %u", (lim_p)->vinput.strd_max); \
   printf("\npass - max color attachments: %u", (lim_p)->pass.color_max); \
   printf("\npass - max target dimensions: %u, %u", \
      (lim_p)->pass.dim_max.width, (lim_p)->pass.dim_max.height); \
   printf("\npass - max target layers: %u", (lim_p)->pass.layer_max); \
   printf("\nviewport - max: %u", (lim_p)->viewport.max); \
   printf("\nviewport - max dimensions: %u, %u", \
      (lim_p)->viewport.dim_max.width, (lim_p)->viewport.dim_max.height); \
   printf("\nviewport - min/max bounds: %.3f, %.3f", \
      (lim_p)->viewport.bounds_min, (lim_p)->viewport.bounds_max); \
   printf("\nstate - max bound dtables: %u", (lim_p)->state.dtable_max); \
   printf("\nstate - max vinputs: %u", (lim_p)->state.vinput_max); \
   printf("\ncmdbuf - max draw index value: %u", \
      (lim_p)->cmdbuf.draw_idx_max); \
   printf("\n--\n"); } while (0)
#endif

#endif /* YF_LIMITS_H */
