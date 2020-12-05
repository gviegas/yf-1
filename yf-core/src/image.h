/*
 * YF
 * image.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_IMAGE_H
#define YF_IMAGE_H

#include <limits.h>

#include "yf-image.h"
#include "vk.h"
#include "hashset.h"

typedef struct YF_image_o {
  YF_context ctx;
  int wrapped;
  int pixfmt;
  YF_dim3 dim;
  unsigned layers;
  unsigned levels;
  YF_hashset iviews;

  VkImage image;
  VkDeviceMemory memory;
  VkFormat format;
  VkImageType type;
  VkSampleCountFlagBits samples;
  VkImageAspectFlags aspect;
  VkImageViewType view_type;
  VkImageLayout layout;
} YF_image_o;

/* Type defining an image view. */
typedef struct {
  void *priv;
  VkImageView view;
} YF_iview;

/* Wraps an image handle.
   The caller is responsible for the explicity destruction of the image handle
   and its backing memory. A 'YF_image' created this way must be deinitialized
   before the image handle is destroyed. */
YF_image yf_image_wrap(
  YF_context ctx,
  VkImage image,
  VkFormat format,
  VkImageType type,
  YF_dim3 dim,
  unsigned layers,
  unsigned levels,
  VkSampleCountFlagBits samples,
  VkImageLayout layout);

/* Gets an image view.
   Every call to this function must be matched by a call to 'ungetiview'. */
int yf_image_getiview(
  YF_image img,
  YF_slice layers,
  YF_slice levels,
  YF_iview *iview);

/* Ungets an image view.
   This function must be called when the iview is not needed anymore. */
void yf_image_ungetiview(YF_image img, YF_iview *iview);

/* Encodes a general layout transition in the given command buffer.
   The command buffer is neither started nor finished by this function. */
void yf_image_transition(YF_image img, VkCommandBuffer cbuffer);

/* Converts from a 'YF_PIXFMT' value. */
#define YF_PIXFMT_FROM(pf, to) do { \
  if (pf == YF_PIXFMT_R8UNORM) to = VK_FORMAT_R8_UNORM; \
  else if (pf == YF_PIXFMT_RG8UNORM) to = VK_FORMAT_R8G8_UNORM; \
  else if (pf == YF_PIXFMT_RGB8UNORM) to = VK_FORMAT_R8G8B8_UNORM; \
  else if (pf == YF_PIXFMT_RGBA8UNORM) to = VK_FORMAT_R8G8B8A8_UNORM; \
  else if (pf == YF_PIXFMT_BGR8UNORM) to = VK_FORMAT_B8G8R8_UNORM; \
  else if (pf == YF_PIXFMT_BGRA8UNORM) to = VK_FORMAT_B8G8R8A8_UNORM; \
  else if (pf == YF_PIXFMT_R16UNORM) to = VK_FORMAT_R16_UNORM; \
  else if (pf == YF_PIXFMT_RG16UNORM) to = VK_FORMAT_R16G16_UNORM; \
  else if (pf == YF_PIXFMT_RGB16UNORM) to = VK_FORMAT_R16G16B16_UNORM; \
  else if (pf == YF_PIXFMT_RGBA16UNORM) to = VK_FORMAT_R16G16B16A16_UNORM; \
  else if (pf == YF_PIXFMT_R8SRGB) to = VK_FORMAT_R8_SRGB; \
  else if (pf == YF_PIXFMT_RG8SRGB) to = VK_FORMAT_R8G8_SRGB; \
  else if (pf == YF_PIXFMT_RGB8SRGB) to = VK_FORMAT_R8G8B8_SRGB; \
  else if (pf == YF_PIXFMT_RGBA8SRGB) to = VK_FORMAT_R8G8B8A8_SRGB; \
  else if (pf == YF_PIXFMT_BGR8SRGB) to = VK_FORMAT_B8G8R8_SRGB; \
  else if (pf == YF_PIXFMT_BGRA8SRGB) to = VK_FORMAT_B8G8R8A8_SRGB; \
  else if (pf == YF_PIXFMT_R8INT) to = VK_FORMAT_R8_SINT; \
  else if (pf == YF_PIXFMT_RG8INT) to = VK_FORMAT_R8G8_SINT; \
  else if (pf == YF_PIXFMT_RGB8INT) to = VK_FORMAT_R8G8B8_SINT; \
  else if (pf == YF_PIXFMT_RGBA8INT) to = VK_FORMAT_R8G8B8A8_SINT; \
  else if (pf == YF_PIXFMT_BGR8INT) to = VK_FORMAT_B8G8R8_SINT; \
  else if (pf == YF_PIXFMT_BGRA8INT) to = VK_FORMAT_B8G8R8A8_SINT; \
  else if (pf == YF_PIXFMT_R16INT) to = VK_FORMAT_R16_SINT; \
  else if (pf == YF_PIXFMT_RG16INT) to = VK_FORMAT_R16G16_SINT; \
  else if (pf == YF_PIXFMT_RGB16INT) to = VK_FORMAT_R16G16B16_SINT; \
  else if (pf == YF_PIXFMT_RGBA16INT) to = VK_FORMAT_R16G16B16A16_SINT; \
  else if (pf == YF_PIXFMT_R32INT) to = VK_FORMAT_R32_SINT; \
  else if (pf == YF_PIXFMT_RG32INT) to = VK_FORMAT_R32G32_SINT; \
  else if (pf == YF_PIXFMT_RGB32INT) to = VK_FORMAT_R32G32B32_SINT; \
  else if (pf == YF_PIXFMT_RGBA32INT) to = VK_FORMAT_R32G32B32A32_SINT; \
  else if (pf == YF_PIXFMT_R8UINT) to = VK_FORMAT_R8_UINT; \
  else if (pf == YF_PIXFMT_RG8UINT) to = VK_FORMAT_R8G8_UINT; \
  else if (pf == YF_PIXFMT_RGB8UINT) to = VK_FORMAT_R8G8B8_UINT; \
  else if (pf == YF_PIXFMT_RGBA8UINT) to = VK_FORMAT_R8G8B8A8_UINT; \
  else if (pf == YF_PIXFMT_BGR8UINT) to = VK_FORMAT_B8G8R8_UINT; \
  else if (pf == YF_PIXFMT_BGRA8UINT) to = VK_FORMAT_B8G8R8A8_UINT; \
  else if (pf == YF_PIXFMT_R16UINT) to = VK_FORMAT_R16_UINT; \
  else if (pf == YF_PIXFMT_RG16UINT) to = VK_FORMAT_R16G16_UINT; \
  else if (pf == YF_PIXFMT_RGB16UINT) to = VK_FORMAT_R16G16B16_UINT; \
  else if (pf == YF_PIXFMT_RGBA16UINT) to = VK_FORMAT_R16G16B16A16_UINT; \
  else if (pf == YF_PIXFMT_R32UINT) to = VK_FORMAT_R32_UINT; \
  else if (pf == YF_PIXFMT_RG32UINT) to = VK_FORMAT_R32G32_UINT; \
  else if (pf == YF_PIXFMT_RGB32UINT) to = VK_FORMAT_R32G32B32_UINT; \
  else if (pf == YF_PIXFMT_RGBA32UINT) to = VK_FORMAT_R32G32B32A32_UINT; \
  else if (pf == YF_PIXFMT_R16FLOAT) to = VK_FORMAT_R16_SFLOAT; \
  else if (pf == YF_PIXFMT_RG16FLOAT) to = VK_FORMAT_R16G16_SFLOAT; \
  else if (pf == YF_PIXFMT_RGB16FLOAT) to = VK_FORMAT_R16G16B16_SFLOAT; \
  else if (pf == YF_PIXFMT_RGBA16FLOAT) to = VK_FORMAT_R16G16B16A16_SFLOAT; \
  else if (pf == YF_PIXFMT_R32FLOAT) to = VK_FORMAT_R32_SFLOAT; \
  else if (pf == YF_PIXFMT_RG32FLOAT) to = VK_FORMAT_R32G32_SFLOAT; \
  else if (pf == YF_PIXFMT_RGB32FLOAT) to = VK_FORMAT_R32G32B32_SFLOAT; \
  else if (pf == YF_PIXFMT_RGBA32FLOAT) to = VK_FORMAT_R32G32B32A32_SFLOAT; \
  else if (pf == YF_PIXFMT_D16UNORM) to = VK_FORMAT_D16_UNORM; \
  else if (pf == YF_PIXFMT_S8UINT) to = VK_FORMAT_S8_UINT; \
  else if (pf == YF_PIXFMT_D16UNORMS8UINT) to = VK_FORMAT_D16_UNORM_S8_UINT; \
  else if (pf == YF_PIXFMT_D24UNORMS8UINT) to = VK_FORMAT_D24_UNORM_S8_UINT; \
  else to = VK_FORMAT_UNDEFINED; } while (0)

/* Converts to a 'YF_PIXFMT' value. */
#define YF_PIXFMT_TO(from, pf) do { \
  if (from == VK_FORMAT_R8_UNORM) pf = YF_PIXFMT_R8UNORM; \
  else if (from == VK_FORMAT_R8G8_UNORM) pf = YF_PIXFMT_RG8UNORM; \
  else if (from == VK_FORMAT_R8G8B8_UNORM) pf = YF_PIXFMT_RGB8UNORM; \
  else if (from == VK_FORMAT_R8G8B8A8_UNORM) pf = YF_PIXFMT_RGBA8UNORM; \
  else if (from == VK_FORMAT_B8G8R8_UNORM) pf = YF_PIXFMT_BGR8UNORM; \
  else if (from == VK_FORMAT_B8G8R8A8_UNORM) pf = YF_PIXFMT_BGRA8UNORM; \
  else if (from == VK_FORMAT_R16_UNORM) pf = YF_PIXFMT_R16UNORM; \
  else if (from == VK_FORMAT_R16G16_UNORM) pf = YF_PIXFMT_RG16UNORM; \
  else if (from == VK_FORMAT_R16G16B16_UNORM) pf = YF_PIXFMT_RGB16UNORM; \
  else if (from == VK_FORMAT_R16G16B16A16_UNORM) pf = YF_PIXFMT_RGBA16UNORM; \
  else if (from == VK_FORMAT_R8_SRGB) pf = YF_PIXFMT_R8SRGB; \
  else if (from == VK_FORMAT_R8G8_SRGB) pf = YF_PIXFMT_RG8SRGB; \
  else if (from == VK_FORMAT_R8G8B8_SRGB) pf = YF_PIXFMT_RGB8SRGB; \
  else if (from == VK_FORMAT_R8G8B8A8_SRGB) pf = YF_PIXFMT_RGBA8SRGB; \
  else if (from == VK_FORMAT_B8G8R8_SRGB) pf = YF_PIXFMT_BGR8SRGB; \
  else if (from == VK_FORMAT_B8G8R8A8_SRGB) pf = YF_PIXFMT_BGRA8SRGB; \
  else if (from == VK_FORMAT_R8_SINT) pf = YF_PIXFMT_R8INT; \
  else if (from == VK_FORMAT_R8G8_SINT) pf = YF_PIXFMT_RG8INT; \
  else if (from == VK_FORMAT_R8G8B8_SINT) pf = YF_PIXFMT_RGB8INT; \
  else if (from == VK_FORMAT_R8G8B8A8_SINT) pf = YF_PIXFMT_RGBA8INT; \
  else if (from == VK_FORMAT_B8G8R8_SINT) pf = YF_PIXFMT_BGR8INT; \
  else if (from == VK_FORMAT_B8G8R8A8_SINT) pf = YF_PIXFMT_BGRA8INT; \
  else if (from == VK_FORMAT_R16_SINT) pf = YF_PIXFMT_R16INT; \
  else if (from == VK_FORMAT_R16G16_SINT) pf = YF_PIXFMT_RG16INT; \
  else if (from == VK_FORMAT_R16G16B16_SINT) pf = YF_PIXFMT_RGB16INT; \
  else if (from == VK_FORMAT_R16G16B16A16_SINT) pf = YF_PIXFMT_RGBA16INT; \
  else if (from == VK_FORMAT_R32_SINT) pf = YF_PIXFMT_R32INT; \
  else if (from == VK_FORMAT_R32G32_SINT) pf = YF_PIXFMT_RG32INT; \
  else if (from == VK_FORMAT_R32G32B32_SINT) pf = YF_PIXFMT_RGB32INT; \
  else if (from == VK_FORMAT_R32G32B32A32_SINT) pf = YF_PIXFMT_RGBA32INT; \
  else if (from == VK_FORMAT_R8_UINT) pf = YF_PIXFMT_R8UINT; \
  else if (from == VK_FORMAT_R8G8_UINT) pf = YF_PIXFMT_RG8UINT; \
  else if (from == VK_FORMAT_R8G8B8_UINT) pf = YF_PIXFMT_RGB8UINT; \
  else if (from == VK_FORMAT_R8G8B8A8_UINT) pf = YF_PIXFMT_RGBA8UINT; \
  else if (from == VK_FORMAT_B8G8R8_UINT) pf = YF_PIXFMT_BGR8UINT; \
  else if (from == VK_FORMAT_B8G8R8A8_UINT) pf = YF_PIXFMT_BGRA8UINT; \
  else if (from == VK_FORMAT_R16_UINT) pf = YF_PIXFMT_R16UINT; \
  else if (from == VK_FORMAT_R16G16_UINT) pf = YF_PIXFMT_RG16UINT; \
  else if (from == VK_FORMAT_R16G16B16_UINT) pf = YF_PIXFMT_RGB16UINT; \
  else if (from == VK_FORMAT_R16G16B16A16_UINT) pf = YF_PIXFMT_RGBA16UINT; \
  else if (from == VK_FORMAT_R32_UINT) pf = YF_PIXFMT_R32UINT; \
  else if (from == VK_FORMAT_R32G32_UINT) pf = YF_PIXFMT_RG32UINT; \
  else if (from == VK_FORMAT_R32G32B32_UINT) pf = YF_PIXFMT_RGB32UINT; \
  else if (from == VK_FORMAT_R32G32B32A32_UINT) pf = YF_PIXFMT_RGBA32UINT; \
  else if (from == VK_FORMAT_R16_SFLOAT) pf = YF_PIXFMT_R16FLOAT; \
  else if (from == VK_FORMAT_R16G16_SFLOAT) pf = YF_PIXFMT_RG16FLOAT; \
  else if (from == VK_FORMAT_R16G16B16_SFLOAT) pf = YF_PIXFMT_RGB16FLOAT; \
  else if (from == VK_FORMAT_R16G16B16A16_SFLOAT) pf = YF_PIXFMT_RGBA16FLOAT; \
  else if (from == VK_FORMAT_R32_SFLOAT) pf = YF_PIXFMT_R32FLOAT; \
  else if (from == VK_FORMAT_R32G32_SFLOAT) pf = YF_PIXFMT_RG32FLOAT; \
  else if (from == VK_FORMAT_R32G32B32_SFLOAT) pf = YF_PIXFMT_RGB32FLOAT; \
  else if (from == VK_FORMAT_R32G32B32A32_SFLOAT) pf = YF_PIXFMT_RGBA32FLOAT; \
  else if (from == VK_FORMAT_D16_UNORM) pf = YF_PIXFMT_D16UNORM; \
  else if (from == VK_FORMAT_S8_UINT) pf = YF_PIXFMT_S8UINT; \
  else if (from == VK_FORMAT_D16_UNORM_S8_UINT) pf = YF_PIXFMT_D16UNORMS8UINT; \
  else if (from == VK_FORMAT_D24_UNORM_S8_UINT) pf = YF_PIXFMT_D24UNORMS8UINT; \
  else from = YF_PIXFMT_UNDEF; } while (0)

/* Retrieves the aspect of a 'YF_PIXFMT' value. */
#define YF_PIXFMT_ASPECT(pf, asp) do { \
  switch (pf) { \
    case YF_PIXFMT_R8UNORM: \
    case YF_PIXFMT_RG8UNORM: \
    case YF_PIXFMT_RGB8UNORM: \
    case YF_PIXFMT_RGBA8UNORM: \
    case YF_PIXFMT_BGR8UNORM: \
    case YF_PIXFMT_BGRA8UNORM: \
    case YF_PIXFMT_R16UNORM: \
    case YF_PIXFMT_RG16UNORM: \
    case YF_PIXFMT_RGB16UNORM: \
    case YF_PIXFMT_RGBA16UNORM: \
    case YF_PIXFMT_R8SRGB: \
    case YF_PIXFMT_RG8SRGB: \
    case YF_PIXFMT_RGB8SRGB: \
    case YF_PIXFMT_RGBA8SRGB: \
    case YF_PIXFMT_BGR8SRGB: \
    case YF_PIXFMT_BGRA8SRGB: \
    case YF_PIXFMT_R8INT: \
    case YF_PIXFMT_RG8INT: \
    case YF_PIXFMT_RGB8INT: \
    case YF_PIXFMT_RGBA8INT: \
    case YF_PIXFMT_BGR8INT: \
    case YF_PIXFMT_BGRA8INT: \
    case YF_PIXFMT_R16INT: \
    case YF_PIXFMT_RG16INT: \
    case YF_PIXFMT_RGB16INT: \
    case YF_PIXFMT_RGBA16INT: \
    case YF_PIXFMT_R32INT: \
    case YF_PIXFMT_RG32INT: \
    case YF_PIXFMT_RGB32INT: \
    case YF_PIXFMT_RGBA32INT: \
    case YF_PIXFMT_R8UINT: \
    case YF_PIXFMT_RG8UINT: \
    case YF_PIXFMT_RGB8UINT: \
    case YF_PIXFMT_RGBA8UINT: \
    case YF_PIXFMT_BGR8UINT: \
    case YF_PIXFMT_BGRA8UINT: \
    case YF_PIXFMT_R16UINT: \
    case YF_PIXFMT_RG16UINT: \
    case YF_PIXFMT_RGB16UINT: \
    case YF_PIXFMT_RGBA16UINT: \
    case YF_PIXFMT_R32UINT: \
    case YF_PIXFMT_RG32UINT: \
    case YF_PIXFMT_RGB32UINT: \
    case YF_PIXFMT_RGBA32UINT: \
    case YF_PIXFMT_R16FLOAT: \
    case YF_PIXFMT_RG16FLOAT: \
    case YF_PIXFMT_RGB16FLOAT: \
    case YF_PIXFMT_RGBA16FLOAT: \
    case YF_PIXFMT_R32FLOAT: \
    case YF_PIXFMT_RG32FLOAT: \
    case YF_PIXFMT_RGB32FLOAT: \
    case YF_PIXFMT_RGBA32FLOAT: \
      asp = VK_IMAGE_ASPECT_COLOR_BIT; \
      break; \
    case YF_PIXFMT_D16UNORM: \
      asp = VK_IMAGE_ASPECT_DEPTH_BIT; \
      break; \
    case YF_PIXFMT_S8UINT: \
      asp = VK_IMAGE_ASPECT_STENCIL_BIT; \
      break; \
    case YF_PIXFMT_D16UNORMS8UINT: \
    case YF_PIXFMT_D24UNORMS8UINT: \
      asp = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT; \
      break; \
    default: \
      asp = INT_MAX; \
  } } while (0)

/* Converts from a sample count. */
#define YF_SAMPLES_FROM(s, to) do { \
  if (s == 1) to = VK_SAMPLE_COUNT_1_BIT; \
  else if (s == 2) to = VK_SAMPLE_COUNT_2_BIT; \
  else if (s == 4) to = VK_SAMPLE_COUNT_4_BIT; \
  else if (s == 8) to = VK_SAMPLE_COUNT_8_BIT; \
  else if (s == 16) to = VK_SAMPLE_COUNT_16_BIT; \
  else if (s == 32) to = VK_SAMPLE_COUNT_32_BIT; \
  else if (s == 64) to = VK_SAMPLE_COUNT_64_BIT; \
  else to = INT_MAX; } while (0)

/* Converts to a sample count. */
#define YF_SAMPLES_TO(from, s) do { \
  if (from == VK_SAMPLE_COUNT_1_BIT) s = 1; \
  else if (from == VK_SAMPLE_COUNT_2_BIT) s = 2; \
  else if (from == VK_SAMPLE_COUNT_4_BIT) s = 4; \
  else if (from == VK_SAMPLE_COUNT_8_BIT) s = 8; \
  else if (from == VK_SAMPLE_COUNT_16_BIT) s = 16; \
  else if (from == VK_SAMPLE_COUNT_32_BIT) s = 32; \
  else if (from == VK_SAMPLE_COUNT_64_BIT) s = 64; \
  else  s = INT_MAX; } while (0)

#endif /* YF_IMAGE_H */
