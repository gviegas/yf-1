/*
 * YF
 * image.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_IMAGE_H
#define YF_IMAGE_H

#include <limits.h>

#include "yf/com/yf-dict.h"

#include "yf-image.h"
#include "vk.h"

struct yf_image {
    yf_context_t *ctx;
    int wrapped;
    int pixfmt;
    yf_dim3_t dim;
    unsigned layers;
    unsigned levels;
    yf_dict_t *iviews;

    VkImage image;
    VkDeviceMemory memory;
    VkFormat format;
    VkImageType type;
    VkSampleCountFlagBits samples;
    VkImageAspectFlags aspect;
    VkImageCreateFlags flags;
    VkImageUsageFlags usage;
    VkImageTiling tiling;
    VkImageViewType view_type;
    VkImageLayout layout;
    VkImageLayout next_layout;
    void *data;
};

/* Type defining an image view. */
typedef struct yf_iview {
    void *priv;
    VkImageView view;
} yf_iview_t;

/* Wraps an image handle.
   The caller is responsible for destroying the image handle and for
   deallocating its backing memory. A 'yf_image_t' created this way must
   be deinitialized before the image handle is destroyed. */
yf_image_t *yf_image_wrap(yf_context_t *ctx, VkImage image, VkFormat format,
                          VkImageType type, yf_dim3_t dim, unsigned layers,
                          unsigned levels, VkSampleCountFlagBits samples,
                          VkImageUsageFlags usage, VkImageLayout layout);

/* Gets an image view.
   Every call to this function must be matched by a call to 'ungetiview()'. */
int yf_image_getiview(yf_image_t *img, yf_slice_t layers, yf_slice_t levels,
                      yf_iview_t *iview);

/* Ungets an image view.
   This function must be called when a 'YF_iview' is not needed anymore. */
void yf_image_ungetiview(yf_image_t *img, yf_iview_t *iview);

/* Changes the layout of an image.
   The layout change is encoded in the priority command buffer provided by
   'cmdpool_getprio()'. */
int yf_image_chglayout(yf_image_t *img, VkImageLayout layout);

/* Converts from a 'YF_PIXFMT' value. */
#define YF_PIXFMT_FROM(pf, to) do { \
    switch (pf) { \
    case YF_PIXFMT_R8UNORM: \
        to = VK_FORMAT_R8_UNORM; \
        break; \
    case YF_PIXFMT_RG8UNORM: \
        to = VK_FORMAT_R8G8_UNORM; \
        break; \
    case YF_PIXFMT_RGB8UNORM: \
        to = VK_FORMAT_R8G8B8_UNORM; \
        break; \
    case YF_PIXFMT_RGBA8UNORM: \
        to = VK_FORMAT_R8G8B8A8_UNORM; \
        break; \
    case YF_PIXFMT_BGR8UNORM: \
        to = VK_FORMAT_B8G8R8_UNORM; \
        break; \
    case YF_PIXFMT_BGRA8UNORM: \
        to = VK_FORMAT_B8G8R8A8_UNORM; \
        break; \
    case YF_PIXFMT_R16UNORM: \
        to = VK_FORMAT_R16_UNORM; \
        break; \
    case YF_PIXFMT_RG16UNORM: \
        to = VK_FORMAT_R16G16_UNORM; \
        break; \
    case YF_PIXFMT_RGB16UNORM: \
        to = VK_FORMAT_R16G16B16_UNORM; \
        break; \
    case YF_PIXFMT_RGBA16UNORM: \
        to = VK_FORMAT_R16G16B16A16_UNORM; \
        break; \
    case YF_PIXFMT_R8SRGB: \
        to = VK_FORMAT_R8_SRGB; \
        break; \
    case YF_PIXFMT_RG8SRGB: \
        to = VK_FORMAT_R8G8_SRGB; \
        break; \
    case YF_PIXFMT_RGB8SRGB: \
        to = VK_FORMAT_R8G8B8_SRGB; \
        break; \
    case YF_PIXFMT_RGBA8SRGB: \
        to = VK_FORMAT_R8G8B8A8_SRGB; \
        break; \
    case YF_PIXFMT_BGR8SRGB: \
        to = VK_FORMAT_B8G8R8_SRGB; \
        break; \
    case YF_PIXFMT_BGRA8SRGB: \
        to = VK_FORMAT_B8G8R8A8_SRGB; \
        break; \
    case YF_PIXFMT_R8INT: \
        to = VK_FORMAT_R8_SINT; \
        break; \
    case YF_PIXFMT_RG8INT: \
        to = VK_FORMAT_R8G8_SINT; \
        break; \
    case YF_PIXFMT_RGB8INT: \
        to = VK_FORMAT_R8G8B8_SINT; \
        break; \
    case YF_PIXFMT_RGBA8INT: \
        to = VK_FORMAT_R8G8B8A8_SINT; \
        break; \
    case YF_PIXFMT_BGR8INT: \
        to = VK_FORMAT_B8G8R8_SINT; \
        break; \
    case YF_PIXFMT_BGRA8INT: \
        to = VK_FORMAT_B8G8R8A8_SINT; \
        break; \
    case YF_PIXFMT_R16INT: \
        to = VK_FORMAT_R16_SINT; \
        break; \
    case YF_PIXFMT_RG16INT: \
        to = VK_FORMAT_R16G16_SINT; \
        break; \
    case YF_PIXFMT_RGB16INT: \
        to = VK_FORMAT_R16G16B16_SINT; \
        break; \
    case YF_PIXFMT_RGBA16INT: \
        to = VK_FORMAT_R16G16B16A16_SINT; \
        break; \
    case YF_PIXFMT_R32INT: \
        to = VK_FORMAT_R32_SINT; \
        break; \
    case YF_PIXFMT_RG32INT: \
        to = VK_FORMAT_R32G32_SINT; \
        break; \
    case YF_PIXFMT_RGB32INT: \
        to = VK_FORMAT_R32G32B32_SINT; \
        break; \
    case YF_PIXFMT_RGBA32INT: \
        to = VK_FORMAT_R32G32B32A32_SINT; \
        break; \
    case YF_PIXFMT_R8UINT: \
        to = VK_FORMAT_R8_UINT; \
        break; \
    case YF_PIXFMT_RG8UINT: \
        to = VK_FORMAT_R8G8_UINT; \
        break; \
    case YF_PIXFMT_RGB8UINT: \
        to = VK_FORMAT_R8G8B8_UINT; \
        break; \
    case YF_PIXFMT_RGBA8UINT: \
        to = VK_FORMAT_R8G8B8A8_UINT; \
        break; \
    case YF_PIXFMT_BGR8UINT: \
        to = VK_FORMAT_B8G8R8_UINT; \
        break; \
    case YF_PIXFMT_BGRA8UINT: \
        to = VK_FORMAT_B8G8R8A8_UINT; \
        break; \
    case YF_PIXFMT_R16UINT: \
        to = VK_FORMAT_R16_UINT; \
        break; \
    case YF_PIXFMT_RG16UINT: \
        to = VK_FORMAT_R16G16_UINT; \
        break; \
    case YF_PIXFMT_RGB16UINT: \
        to = VK_FORMAT_R16G16B16_UINT; \
        break; \
    case YF_PIXFMT_RGBA16UINT: \
        to = VK_FORMAT_R16G16B16A16_UINT; \
        break; \
    case YF_PIXFMT_R32UINT: \
        to = VK_FORMAT_R32_UINT; \
        break; \
    case YF_PIXFMT_RG32UINT: \
        to = VK_FORMAT_R32G32_UINT; \
        break; \
    case YF_PIXFMT_RGB32UINT: \
        to = VK_FORMAT_R32G32B32_UINT; \
        break; \
    case YF_PIXFMT_RGBA32UINT: \
        to = VK_FORMAT_R32G32B32A32_UINT; \
        break; \
    case YF_PIXFMT_R16FLOAT: \
        to = VK_FORMAT_R16_SFLOAT; \
        break; \
    case YF_PIXFMT_RG16FLOAT: \
        to = VK_FORMAT_R16G16_SFLOAT; \
        break; \
    case YF_PIXFMT_RGB16FLOAT: \
        to = VK_FORMAT_R16G16B16_SFLOAT; \
        break; \
    case YF_PIXFMT_RGBA16FLOAT: \
        to = VK_FORMAT_R16G16B16A16_SFLOAT; \
        break; \
    case YF_PIXFMT_R32FLOAT: \
        to = VK_FORMAT_R32_SFLOAT; \
        break; \
    case YF_PIXFMT_RG32FLOAT: \
        to = VK_FORMAT_R32G32_SFLOAT; \
        break; \
    case YF_PIXFMT_RGB32FLOAT: \
        to = VK_FORMAT_R32G32B32_SFLOAT; \
        break; \
    case YF_PIXFMT_RGBA32FLOAT: \
        to = VK_FORMAT_R32G32B32A32_SFLOAT; \
        break; \
    case YF_PIXFMT_D16UNORM: \
        to = VK_FORMAT_D16_UNORM; \
        break; \
    case YF_PIXFMT_S8UINT: \
        to = VK_FORMAT_S8_UINT; \
        break; \
    case YF_PIXFMT_D16UNORMS8UINT: \
        to = VK_FORMAT_D16_UNORM_S8_UINT; \
        break; \
    case YF_PIXFMT_D24UNORMS8UINT: \
        to = VK_FORMAT_D24_UNORM_S8_UINT; \
        break; \
    default: \
        to = VK_FORMAT_UNDEFINED; \
    } } while (0)

/* Converts to a 'YF_PIXFMT' value. */
#define YF_PIXFMT_TO(from, pf) do { \
    switch (from) { \
    case VK_FORMAT_R8_UNORM: \
        pf = YF_PIXFMT_R8UNORM; \
        break; \
    case VK_FORMAT_R8G8_UNORM: \
        pf = YF_PIXFMT_RG8UNORM; \
        break; \
    case VK_FORMAT_R8G8B8_UNORM: \
        pf = YF_PIXFMT_RGB8UNORM; \
        break; \
    case VK_FORMAT_R8G8B8A8_UNORM: \
        pf = YF_PIXFMT_RGBA8UNORM; \
        break; \
    case VK_FORMAT_B8G8R8_UNORM: \
        pf = YF_PIXFMT_BGR8UNORM; \
        break; \
    case VK_FORMAT_B8G8R8A8_UNORM: \
        pf = YF_PIXFMT_BGRA8UNORM; \
        break; \
    case VK_FORMAT_R16_UNORM: \
        pf = YF_PIXFMT_R16UNORM; \
        break; \
    case VK_FORMAT_R16G16_UNORM: \
        pf = YF_PIXFMT_RG16UNORM; \
        break; \
    case VK_FORMAT_R16G16B16_UNORM: \
        pf = YF_PIXFMT_RGB16UNORM; \
        break; \
    case VK_FORMAT_R16G16B16A16_UNORM: \
        pf = YF_PIXFMT_RGBA16UNORM; \
        break; \
    case VK_FORMAT_R8_SRGB: \
        pf = YF_PIXFMT_R8SRGB; \
        break; \
    case VK_FORMAT_R8G8_SRGB: \
        pf = YF_PIXFMT_RG8SRGB; \
        break; \
    case VK_FORMAT_R8G8B8_SRGB: \
        pf = YF_PIXFMT_RGB8SRGB; \
        break; \
    case VK_FORMAT_R8G8B8A8_SRGB: \
        pf = YF_PIXFMT_RGBA8SRGB; \
        break; \
    case VK_FORMAT_B8G8R8_SRGB: \
        pf = YF_PIXFMT_BGR8SRGB; \
        break; \
    case VK_FORMAT_B8G8R8A8_SRGB: \
        pf = YF_PIXFMT_BGRA8SRGB; \
        break; \
    case VK_FORMAT_R8_SINT: \
        pf = YF_PIXFMT_R8INT; \
        break; \
    case VK_FORMAT_R8G8_SINT: \
        pf = YF_PIXFMT_RG8INT; \
        break; \
    case VK_FORMAT_R8G8B8_SINT: \
        pf = YF_PIXFMT_RGB8INT; \
        break; \
    case VK_FORMAT_R8G8B8A8_SINT: \
        pf = YF_PIXFMT_RGBA8INT; \
        break; \
    case VK_FORMAT_B8G8R8_SINT: \
        pf = YF_PIXFMT_BGR8INT; \
        break; \
    case VK_FORMAT_B8G8R8A8_SINT: \
        pf = YF_PIXFMT_BGRA8INT; \
        break; \
    case VK_FORMAT_R16_SINT: \
        pf = YF_PIXFMT_R16INT; \
        break; \
    case VK_FORMAT_R16G16_SINT: \
        pf = YF_PIXFMT_RG16INT; \
        break; \
    case VK_FORMAT_R16G16B16_SINT: \
        pf = YF_PIXFMT_RGB16INT; \
        break; \
    case VK_FORMAT_R16G16B16A16_SINT: \
        pf = YF_PIXFMT_RGBA16INT; \
        break; \
    case VK_FORMAT_R32_SINT: \
        pf = YF_PIXFMT_R32INT; \
        break; \
    case VK_FORMAT_R32G32_SINT: \
        pf = YF_PIXFMT_RG32INT; \
        break; \
    case VK_FORMAT_R32G32B32_SINT: \
        pf = YF_PIXFMT_RGB32INT; \
        break; \
    case VK_FORMAT_R32G32B32A32_SINT: \
        pf = YF_PIXFMT_RGBA32INT; \
        break; \
    case VK_FORMAT_R8_UINT: \
        pf = YF_PIXFMT_R8UINT; \
        break; \
    case VK_FORMAT_R8G8_UINT: \
        pf = YF_PIXFMT_RG8UINT; \
        break; \
    case VK_FORMAT_R8G8B8_UINT: \
        pf = YF_PIXFMT_RGB8UINT; \
        break; \
    case VK_FORMAT_R8G8B8A8_UINT: \
        pf = YF_PIXFMT_RGBA8UINT; \
        break; \
    case VK_FORMAT_B8G8R8_UINT: \
        pf = YF_PIXFMT_BGR8UINT; \
        break; \
    case VK_FORMAT_B8G8R8A8_UINT: \
        pf = YF_PIXFMT_BGRA8UINT; \
        break; \
    case VK_FORMAT_R16_UINT: \
        pf = YF_PIXFMT_R16UINT; \
        break; \
    case VK_FORMAT_R16G16_UINT: \
        pf = YF_PIXFMT_RG16UINT; \
        break; \
    case VK_FORMAT_R16G16B16_UINT: \
        pf = YF_PIXFMT_RGB16UINT; \
        break; \
    case VK_FORMAT_R16G16B16A16_UINT: \
        pf = YF_PIXFMT_RGBA16UINT; \
        break; \
    case VK_FORMAT_R32_UINT: \
        pf = YF_PIXFMT_R32UINT; \
        break; \
    case VK_FORMAT_R32G32_UINT: \
        pf = YF_PIXFMT_RG32UINT; \
        break; \
    case VK_FORMAT_R32G32B32_UINT: \
        pf = YF_PIXFMT_RGB32UINT; \
        break; \
    case VK_FORMAT_R32G32B32A32_UINT: \
        pf = YF_PIXFMT_RGBA32UINT; \
        break; \
    case VK_FORMAT_R16_SFLOAT: \
        pf = YF_PIXFMT_R16FLOAT; \
        break; \
    case VK_FORMAT_R16G16_SFLOAT: \
        pf = YF_PIXFMT_RG16FLOAT; \
        break; \
    case VK_FORMAT_R16G16B16_SFLOAT: \
        pf = YF_PIXFMT_RGB16FLOAT; \
        break; \
    case VK_FORMAT_R16G16B16A16_SFLOAT: \
        pf = YF_PIXFMT_RGBA16FLOAT; \
        break; \
    case VK_FORMAT_R32_SFLOAT: \
        pf = YF_PIXFMT_R32FLOAT; \
        break; \
    case VK_FORMAT_R32G32_SFLOAT: \
        pf = YF_PIXFMT_RG32FLOAT; \
        break; \
    case VK_FORMAT_R32G32B32_SFLOAT: \
        pf = YF_PIXFMT_RGB32FLOAT; \
        break; \
    case VK_FORMAT_R32G32B32A32_SFLOAT: \
        pf = YF_PIXFMT_RGBA32FLOAT; \
        break; \
    case VK_FORMAT_D16_UNORM: \
        pf = YF_PIXFMT_D16UNORM; \
        break; \
    case VK_FORMAT_S8_UINT: \
        pf = YF_PIXFMT_S8UINT; \
        break; \
    case VK_FORMAT_D16_UNORM_S8_UINT: \
        pf = YF_PIXFMT_D16UNORMS8UINT; \
        break; \
    case VK_FORMAT_D24_UNORM_S8_UINT: \
        pf = YF_PIXFMT_D24UNORMS8UINT; \
        break; \
    default: \
        pf = YF_PIXFMT_UNDEF; \
    } } while (0)

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
    switch (s) { \
    case 1: \
        to = VK_SAMPLE_COUNT_1_BIT; \
        break; \
    case 2: \
        to = VK_SAMPLE_COUNT_2_BIT; \
        break; \
    case 4: \
        to = VK_SAMPLE_COUNT_4_BIT; \
        break; \
    case 8: \
        to = VK_SAMPLE_COUNT_8_BIT; \
        break; \
    case 16: \
        to = VK_SAMPLE_COUNT_16_BIT; \
        break; \
    case 32: \
        to = VK_SAMPLE_COUNT_32_BIT; \
        break; \
    case 64: \
        to = VK_SAMPLE_COUNT_64_BIT; \
        break; \
    default: \
        to = INT_MAX; \
    } } while (0)

/* Converts to a sample count. */
#define YF_SAMPLES_TO(from, s) do { \
    switch (from) { \
    case VK_SAMPLE_COUNT_1_BIT: \
        s = 1; \
        break; \
    case VK_SAMPLE_COUNT_2_BIT: \
        s = 2; \
        break; \
    case VK_SAMPLE_COUNT_4_BIT: \
        s = 4; \
        break; \
    case VK_SAMPLE_COUNT_8_BIT: \
        s = 8; \
        break; \
    case VK_SAMPLE_COUNT_16_BIT: \
        s = 16; \
        break; \
    case VK_SAMPLE_COUNT_32_BIT: \
        s = 32; \
        break; \
    case VK_SAMPLE_COUNT_64_BIT: \
        s = 64; \
        break; \
    default: \
        s = INT_MAX; \
    } } while (0)

#endif /* YF_IMAGE_H */
