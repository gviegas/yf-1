/*
 * YF
 * texture.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_TEXTURE_H
#define YF_TEXTURE_H

#include <yf/core/yf-cmdbuf.h>

#include "yf-texture.h"

/* Type defining the texture data. */
typedef struct {
  void *data;
  int pixfmt;
  YF_dim2 dim;
} YF_texdt;

/* Initializes a new texture object from texture data directly. */
YF_texture yf_texture_initdt(const YF_texdt *data);

/* Copies image data from a texture to a dtable resource. */
int yf_texture_copyres(YF_texture tex, YF_dtable dtb, unsigned alloc_i,
    unsigned binding, unsigned element);

#ifdef YF_DEBUG
# define YF_TEXDT_PRINT(dt_p) do { \
   printf("\n-- Texdt (debug) --"); \
   printf("\ndim: %u, %u", (dt_p)->dim.width, (dt_p)->dim.height); \
   if ((dt_p)->pixfmt == YF_PIXFMT_RGB8SRGB) { \
    printf("\npixfmt: %d (rgb8 sRGB)", (dt_p)->pixfmt); \
    if ((dt_p)->dim.width <= 64 && (dt_p)->dim.height <= 64) { \
     printf("\ndata:"); \
     for (unsigned i = 0; i < (dt_p)->dim.height; ++i) { \
      printf("\n"); \
      for (unsigned j = 0; j < (dt_p)->dim.width; ++j) \
       printf("[%02x %02x %02x] ", \
        ((unsigned char *)(dt_p)->data)[(dt_p)->dim.width*3*i+3*j], \
        ((unsigned char *)(dt_p)->data)[(dt_p)->dim.width*3*i+3*j+1], \
        ((unsigned char *)(dt_p)->data)[(dt_p)->dim.width*3*i+3*j+2]); \
     } \
    } \
   } else if ((dt_p)->pixfmt == YF_PIXFMT_RGBA8SRGB) { \
    printf("\npixfmt: %d (rgba8 sRGB)", (dt_p)->pixfmt); \
    if ((dt_p)->dim.width <= 64 && (dt_p)->dim.height <= 64) { \
     printf("\ndata:"); \
     for (unsigned i = 0; i < (dt_p)->dim.height; ++i) { \
      printf("\n"); \
      for (unsigned j = 0; j < (dt_p)->dim.width; ++j) \
       printf("[%02x %02x %02x %02x] ", \
        ((unsigned char *)(dt_p)->data)[(dt_p)->dim.width*4*i+4*j], \
        ((unsigned char *)(dt_p)->data)[(dt_p)->dim.width*4*i+4*j+1], \
        ((unsigned char *)(dt_p)->data)[(dt_p)->dim.width*4*i+4*j+2], \
        ((unsigned char *)(dt_p)->data)[(dt_p)->dim.width*4*i+4*j+3]); \
     } \
    } \
   } else { printf("\n???"); } \
   printf("\n--\n"); } while (0)
#endif

#endif /* YF_TEXTURE_H */
