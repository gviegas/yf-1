/*
 * YF
 * yf-image.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_IMAGE_H
#define YF_YF_IMAGE_H

#include <stddef.h>

#include "yf-common.h"
#include "yf-context.h"

YF_DECLS_BEGIN

/* Opaque type defining formatted multidimensional data. */
typedef struct YF_image_o *YF_image;

/* Undefined pixel format. */
#define YF_PIXFMT_UNDEF 0

/* Color formats - normalized, unsigned. */
#define YF_PIXFMT_R8UNORM     16
#define YF_PIXFMT_RG8UNORM    17
#define YF_PIXFMT_RGB8UNORM   18
#define YF_PIXFMT_RGBA8UNORM  19
#define YF_PIXFMT_BGR8UNORM   20
#define YF_PIXFMT_BGRA8UNORM  21
#define YF_PIXFMT_R16UNORM    22
#define YF_PIXFMT_RG16UNORM   23
#define YF_PIXFMT_RGB16UNORM  24
#define YF_PIXFMT_RGBA16UNORM 25

/* Color formats - sRGB. */
#define YF_PIXFMT_R8SRGB    48
#define YF_PIXFMT_RG8SRGB   49
#define YF_PIXFMT_RGB8SRGB  50
#define YF_PIXFMT_RGBA8SRGB 51
#define YF_PIXFMT_BGR8SRGB  52
#define YF_PIXFMT_BGRA8SRGB 53

/* Color formats - integer, signed. */
#define YF_PIXFMT_R8INT     80
#define YF_PIXFMT_RG8INT    81
#define YF_PIXFMT_RGB8INT   82
#define YF_PIXFMT_RGBA8INT  83
#define YF_PIXFMT_BGR8INT   84
#define YF_PIXFMT_BGRA8INT  85
#define YF_PIXFMT_R16INT    86
#define YF_PIXFMT_RG16INT   87
#define YF_PIXFMT_RGB16INT  88
#define YF_PIXFMT_RGBA16INT 89
#define YF_PIXFMT_R32INT    90
#define YF_PIXFMT_RG32INT   91
#define YF_PIXFMT_RGB32INT  92
#define YF_PIXFMT_RGBA32INT 93

/* Color formats - integer, unsigned. */
#define YF_PIXFMT_R8UINT     112
#define YF_PIXFMT_RG8UINT    113
#define YF_PIXFMT_RGB8UINT   114
#define YF_PIXFMT_RGBA8UINT  115
#define YF_PIXFMT_BGR8UINT   116
#define YF_PIXFMT_BGRA8UINT  117
#define YF_PIXFMT_R16UINT    118
#define YF_PIXFMT_RG16UINT   119
#define YF_PIXFMT_RGB16UINT  120
#define YF_PIXFMT_RGBA16UINT 121
#define YF_PIXFMT_R32UINT    122
#define YF_PIXFMT_RG32UINT   123
#define YF_PIXFMT_RGB32UINT  124
#define YF_PIXFMT_RGBA32UINT 125

/* Color formats - floating-point. */
#define YF_PIXFMT_R16FLOAT    144
#define YF_PIXFMT_RG16FLOAT   145
#define YF_PIXFMT_RGB16FLOAT  146
#define YF_PIXFMT_RGBA16FLOAT 147
#define YF_PIXFMT_R32FLOAT    148
#define YF_PIXFMT_RG32FLOAT   149
#define YF_PIXFMT_RGB32FLOAT  150
#define YF_PIXFMT_RGBA32FLOAT 151

/* Depth & stencil formats. */
#define YF_PIXFMT_D16UNORM       784
#define YF_PIXFMT_S8UINT         785
#define YF_PIXFMT_D16UNORMS8UINT 786
#define YF_PIXFMT_D24UNORMS8UINT 787

/* Initializes a new image. */
YF_image yf_image_init(
  YF_context ctx,
  int pixfmt,
  YF_dim3 dim,
  unsigned layers,
  unsigned levels,
  unsigned samples);

/* Copies local data to an image. */
int yf_image_copy(YF_image img, YF_slice layers, const void *data, size_t size);

/* Gets values of an image. */
void yf_image_getval(
  YF_image img,
  int *pixfmt,
  YF_dim3 *dim,
  unsigned *layers,
  unsigned *levels,
  unsigned *samples);

/* Deinitializes an image. */
void yf_image_deinit(YF_image img);

/* Computes the number of bytes necessary to store a single pixel of a
   given 'YF_PIXFMT'. */
#define YF_PIXFMT_SIZEOF(pf, sz) do { \
  switch (pf) { \
    case YF_PIXFMT_R8UNORM: \
    case YF_PIXFMT_R8SRGB: \
    case YF_PIXFMT_R8INT: \
    case YF_PIXFMT_R8UINT: \
    case YF_PIXFMT_S8UINT: \
      sz = 1; \
      break; \
    case YF_PIXFMT_RG8UNORM: \
    case YF_PIXFMT_R16UNORM: \
    case YF_PIXFMT_RG8SRGB: \
    case YF_PIXFMT_RG8INT: \
    case YF_PIXFMT_R16INT: \
    case YF_PIXFMT_RG8UINT: \
    case YF_PIXFMT_R16UINT: \
    case YF_PIXFMT_R16FLOAT: \
    case YF_PIXFMT_D16UNORM: \
      sz = 2; \
      break; \
    case YF_PIXFMT_RGB8UNORM: \
    case YF_PIXFMT_BGR8UNORM: \
    case YF_PIXFMT_RGB8SRGB: \
    case YF_PIXFMT_BGR8SRGB: \
    case YF_PIXFMT_RGB8INT: \
    case YF_PIXFMT_BGR8INT: \
    case YF_PIXFMT_RGB8UINT: \
    case YF_PIXFMT_BGR8UINT: \
    case YF_PIXFMT_D16UNORMS8UINT: \
      sz = 3; \
      break; \
    case YF_PIXFMT_RGBA8UNORM: \
    case YF_PIXFMT_BGRA8UNORM: \
    case YF_PIXFMT_RG16UNORM: \
    case YF_PIXFMT_RGBA8SRGB: \
    case YF_PIXFMT_BGRA8SRGB: \
    case YF_PIXFMT_RGBA8INT: \
    case YF_PIXFMT_BGRA8INT: \
    case YF_PIXFMT_RG16INT: \
    case YF_PIXFMT_R32INT: \
    case YF_PIXFMT_RGBA8UINT: \
    case YF_PIXFMT_BGRA8UINT: \
    case YF_PIXFMT_RG16UINT: \
    case YF_PIXFMT_R32UINT: \
    case YF_PIXFMT_RG16FLOAT: \
    case YF_PIXFMT_R32FLOAT: \
    case YF_PIXFMT_D24UNORMS8UINT: \
      sz = 4; \
      break; \
    case YF_PIXFMT_RGB16UNORM: \
    case YF_PIXFMT_RGB16INT: \
    case YF_PIXFMT_RGB16UINT: \
    case YF_PIXFMT_RGB16FLOAT: \
      sz = 6; \
      break; \
    case YF_PIXFMT_RGBA16UNORM: \
    case YF_PIXFMT_RGBA16INT: \
    case YF_PIXFMT_RG32INT: \
    case YF_PIXFMT_RGBA16UINT: \
    case YF_PIXFMT_RG32UINT: \
    case YF_PIXFMT_RGBA16FLOAT: \
    case YF_PIXFMT_RG32FLOAT: \
      sz = 8; \
      break; \
    case YF_PIXFMT_RGB32INT: \
    case YF_PIXFMT_RGB32UINT: \
    case YF_PIXFMT_RGB32FLOAT: \
      sz = 12; \
      break; \
    case YF_PIXFMT_RGBA32INT: \
    case YF_PIXFMT_RGBA32UINT: \
    case YF_PIXFMT_RGBA32FLOAT: \
      sz = 16; \
      break; \
    default: \
      sz = 0; \
  } } while (0)

YF_DECLS_END

#endif /* YF_YF_IMAGE_H */
