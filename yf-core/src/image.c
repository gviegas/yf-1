/*
 * YF
 * image.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-pubsub.h"
#include "yf/com/yf-error.h"

#include "image.h"
#include "context.h"
#include "memory.h"
#include "cmdpool.h"
#include "cmdexec.h"
#include "cmdbuf.h"
#include "buffer.h"
#include "yf-limits.h"

/* Type defining the private data of a 'YF_iview'. */
typedef struct {
    struct {
        YF_slice layers;
        YF_slice levels;
    } key;
    unsigned count;
} T_priv;

/* Deallocates a staging buffer. */
static void dealloc_stgbuf(int res, void *arg)
{
    yf_buffer_deinit((YF_buffer)arg);

    if (res != 0) {
        /* TODO */
    }
}

/* Sets image layout. */
static void set_layout(int res, void *arg)
{
    YF_image img = arg;
    if (res == 0)
        /* succeeded */
        img->layout = img->next_layout;
    else
        /* failed */
        img->next_layout = img->layout;
}

/* Hashes a 'T_priv'. */
static size_t hash_priv(const void *x)
{
    const T_priv *pv = x;
    return yf_hashv(&pv->key, sizeof pv->key, NULL);

    static_assert(sizeof pv->key == 4*sizeof(unsigned), "!sizeof");
}

/* Compares a 'T_priv' to another. */
static int cmp_priv(const void *a, const void *b)
{
    const T_priv *pv1 = a;
    const T_priv *pv2 = b;

    return pv1->key.layers.i != pv2->key.layers.i ||
           pv1->key.layers.n != pv2->key.layers.n ||
           pv1->key.levels.i != pv2->key.levels.i ||
           pv1->key.levels.n != pv2->key.levels.n;
}

/* Sets usage for a given image tiling. */
static int set_usage(YF_image img, VkImageTiling tiling)
{
    assert(img != NULL);
    assert(tiling == VK_IMAGE_TILING_LINEAR ||
           tiling == VK_IMAGE_TILING_OPTIMAL);

    VkFormatProperties prop;
    vkGetPhysicalDeviceFormatProperties(img->ctx->phy_dev, img->format, &prop);

    VkImageUsageFlags usage = 0;
    VkFormatFeatureFlags feat = tiling == VK_IMAGE_TILING_LINEAR ?
                                prop.linearTilingFeatures :
                                prop.optimalTilingFeatures;

    if (feat & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (feat & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (feat & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    /* XXX: This assumes that multisample storage is not supported. */
    if (img->samples == VK_SAMPLE_COUNT_1_BIT &&
        (feat & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
        usage |= VK_IMAGE_USAGE_STORAGE_BIT;

    if (usage == 0) {
        yf_seterr(YF_ERR_UNSUP, __func__);
        return -1;
    }

    /* XXX: This may prevent writes to image with optimal tiling. */
    if (img->ctx->dev_prop.apiVersion >= VK_API_VERSION_1_1) {
        if (feat & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT)
            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if (feat & VK_FORMAT_FEATURE_TRANSFER_DST_BIT)
            usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    } else {
        /* XXX: Not in v1.0, assume its valid. */
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    img->usage = usage;
    return 0;
}

/* Sets image tiling. */
static int set_tiling(YF_image img, VkImageTiling tiling)
{
    assert(img != NULL);
    assert(tiling == VK_IMAGE_TILING_LINEAR ||
           tiling == VK_IMAGE_TILING_OPTIMAL);

    VkImageFormatProperties prop;
    VkResult res;
    res = vkGetPhysicalDeviceImageFormatProperties(img->ctx->phy_dev,
                                                   img->format, img->type,
                                                   tiling, img->usage,
                                                   img->flags, &prop);
    switch (res) {
    case VK_SUCCESS:
        if (prop.maxExtent.width < img->dim.width ||
            prop.maxExtent.height < img->dim.height ||
            prop.maxExtent.depth < img->dim.depth ||
            prop.maxMipLevels < img->levels ||
            prop.maxArrayLayers < img->layers ||
            !(prop.sampleCounts & img->samples)) {

            yf_seterr(YF_ERR_UNSUP, __func__);
            return -1;
        }
        img->tiling = tiling;
        break;

    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        yf_seterr(YF_ERR_UNSUP, __func__);
        return -1;

    default:
        yf_seterr(YF_ERR_DEVGEN, __func__);
        return -1;
    }

    return 0;
}

YF_image yf_image_init(YF_context ctx, int pixfmt, YF_dim3 dim,
                       unsigned layers, unsigned levels, unsigned samples)
{
    assert(ctx != NULL);
    assert(dim.width > 0 && dim.height > 0 && dim.depth > 0);
    assert(layers > 0 && levels > 0 && samples > 0);

    YF_image img = calloc(1, sizeof(YF_image_o));
    if (img == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    img->ctx = ctx;
    img->wrapped = 0;
    img->pixfmt = pixfmt;
    img->dim = dim;
    img->layers = layers;
    img->levels = levels;

    img->iviews = yf_dict_init(hash_priv, cmp_priv);
    if (img->iviews == NULL) {
        free(img);
        return NULL;
    }

    YF_PIXFMT_FROM(pixfmt, img->format);
    if (img->format == VK_FORMAT_UNDEFINED) {
        yf_seterr(YF_ERR_INVARG, __func__);
        yf_image_deinit(img);
        return NULL;
    }

    YF_SAMPLES_FROM(samples, img->samples);
    if (img->samples == INT_MAX) {
        yf_seterr(YF_ERR_INVARG, __func__);
        yf_image_deinit(img);
        return NULL;
    }

    YF_PIXFMT_ASPECT(pixfmt, img->aspect);

    img->flags = 0;
    if (dim.depth > 1) {
        /* 3D */
        if (layers != 1) {
            yf_seterr(YF_ERR_INVARG, __func__);
            yf_image_deinit(img);
            return NULL;
        }
        img->type = VK_IMAGE_TYPE_3D;
        img->view_type = VK_IMAGE_VIEW_TYPE_3D;
        if (ctx->dev_prop.apiVersion >= VK_API_VERSION_1_1)
            img->flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;

    } else if (dim.height > 1) {
        /* 2D */
        img->type = VK_IMAGE_TYPE_2D;
        img->view_type = layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY :
                                      VK_IMAGE_VIEW_TYPE_2D;
        if (dim.width == dim.height && dim.width >= 6)
            img->flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    } else {
        /* 1D */
        img->type = VK_IMAGE_TYPE_1D;
        img->view_type = layers > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY :
                                      VK_IMAGE_VIEW_TYPE_1D;
    }

    /* prefer linear tiling */
    if (samples != 1 ||
        set_usage(img, VK_IMAGE_TILING_LINEAR) != 0 ||
        set_tiling(img, VK_IMAGE_TILING_LINEAR) != 0) {

        /* linear tiling won't work, try optimal tiling */
        if (set_usage(img, VK_IMAGE_TILING_OPTIMAL) != 0 ||
            set_tiling(img, VK_IMAGE_TILING_OPTIMAL) != 0) {

            yf_image_deinit(img);
            return NULL;
        }
        img->layout = VK_IMAGE_LAYOUT_UNDEFINED;

    } else {
        img->layout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    }

    img->next_layout = img->layout;

    VkImageCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = img->flags,
        .imageType = img->type,
        .format = img->format,
        .extent = {img->dim.width, img->dim.height, img->dim.depth},
        .mipLevels = img->levels,
        .arrayLayers = img->layers,
        .samples = img->samples,
        .tiling = img->tiling,
        .usage = img->usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .initialLayout = img->layout
    };

    VkResult res = vkCreateImage(ctx->device, &info, NULL, &img->image);
    if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        yf_image_deinit(img);
        return NULL;
    }

    if (yf_image_alloc(img) != 0) {
        yf_image_deinit(img);
        return NULL;
    }

    yf_setpub(img, YF_PUBSUB_DEINIT);

    return img;
}

int yf_image_copy(YF_image img, YF_off3 off, YF_dim3 dim, unsigned layer,
                  unsigned level, const void *data)
{
    assert(img != NULL);
    assert(data != NULL);
    assert(dim.width > 0 && dim.height > 0 && dim.depth > 0);

    if (layer >= img->layers || level >= img->levels ||
        off.x + dim.width > img->dim.width ||
        off.y + dim.height > img->dim.height ||
        off.z + dim.depth > img->dim.depth) {

        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    if (img->tiling == VK_IMAGE_TILING_LINEAR) {
        /* write data to image memory directly */
        switch (img->next_layout) {
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
        case VK_IMAGE_LAYOUT_GENERAL:
            break;
        default:
            /* must be host-accessible */
            if (yf_image_chglayout(img, VK_IMAGE_LAYOUT_GENERAL) != 0 ||
                yf_cmdexec_execprio(img->ctx) != 0)
                return -1;
        }

        if (img->aspect != VK_IMAGE_ASPECT_COLOR_BIT &&
            img->aspect != VK_IMAGE_ASPECT_DEPTH_BIT &&
            img->aspect != VK_IMAGE_ASPECT_STENCIL_BIT) {
            yf_seterr(YF_ERR_UNSUP, __func__);
            return -1;
        }

        VkImageSubresource subres = {
            .aspectMask = img->aspect,
            .mipLevel = level,
            .arrayLayer = 0
        };
        VkSubresourceLayout layout;
        vkGetImageSubresourceLayout(img->ctx->device, img->image, &subres,
                                    &layout);

        size_t tx_sz;
        YF_PIXFMT_SIZEOF(img->pixfmt, tx_sz);

        unsigned char *dst = img->data;
        dst += layout.offset +
               layer * layout.arrayPitch +
               off.z * layout.depthPitch +
               off.y * layout.rowPitch +
               off.x * tx_sz;

        const unsigned char *src = data;
        const size_t row_sz = dim.width * tx_sz;

        for (unsigned i = 0; i < dim.height; i++) {
            memcpy(dst, src, row_sz);
            dst += layout.rowPitch;
            src += row_sz;
        }

    } else {
        /* write data to buffer and then issue a copy to image command */
        switch (img->next_layout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_GENERAL:
            break;
        default:
            if (yf_image_chglayout(img, VK_IMAGE_LAYOUT_GENERAL) != 0)
                return -1;
        }

        size_t sz;
        YF_PIXFMT_SIZEOF(img->pixfmt, sz);
        sz *= dim.width * dim.height * dim.depth;

        YF_buffer stg_buf = yf_buffer_init(img->ctx, sz);
        if (stg_buf == NULL)
            return -1;

        memcpy(stg_buf->data, data, sz);

        const YF_cmdres *cmdr = yf_cmdpool_getprio(img->ctx, dealloc_stgbuf,
                                                   stg_buf);
        if (cmdr == NULL) {
            yf_buffer_deinit(stg_buf);
            return -1;
        }

        VkBufferImageCopy region = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .imageSubresource = {
                .aspectMask = img->aspect,
                .mipLevel = level,
                .baseArrayLayer = layer,
                .layerCount = 1
            },
            .imageOffset = {off.x, off.y, off.z},
            .imageExtent = {dim.width, dim.height, dim.depth}
        };

        vkCmdCopyBufferToImage(cmdr->pool_res, stg_buf->buffer, img->image,
                               img->next_layout, 1, &region);
    }

    return 0;
}

void yf_image_getval(YF_image img, int *pixfmt, YF_dim3 *dim,
                     unsigned *layers, unsigned *levels, unsigned *samples)
{
    assert(img != NULL);

    if (pixfmt != NULL)
        *pixfmt = img->pixfmt;
    if (dim != NULL)
        *dim = img->dim;
    if (layers != NULL)
        *layers = img->layers;
    if (levels != NULL)
        *levels = img->levels;
    if (samples != NULL)
        YF_SAMPLES_TO(img->samples, *samples);
}

void yf_image_deinit(YF_image img)
{
    if (img == NULL)
        return;

    yf_publish(img, YF_PUBSUB_DEINIT);
    yf_setpub(img, YF_PUBSUB_NONE);

    /* cannot let 'cmdpool' call 'set_layout()' with a dangling ptr */
    if (img->layout != img->next_layout)
        yf_cmdexec_execprio(img->ctx);

    YF_iter it = YF_NILIT;
    YF_iview *iv;

    while ((iv = yf_dict_next(img->iviews, &it, NULL)) != NULL) {
        vkDestroyImageView(img->ctx->device, iv->view, NULL);
        free(iv->priv);
        free(iv);
    }

    yf_dict_deinit(img->iviews);

    if (!img->wrapped) {
        yf_image_free(img);
        vkDestroyImage(img->ctx->device, img->image, NULL);
    }

    free(img);
}

YF_image yf_image_wrap(YF_context ctx, VkImage image, VkFormat format,
                       VkImageType type, YF_dim3 dim, unsigned layers,
                       unsigned levels, VkSampleCountFlagBits samples,
                       VkImageUsageFlags usage, VkImageLayout layout)
{
    assert(ctx != NULL);
    assert(image != NULL);
    assert(format != VK_FORMAT_UNDEFINED);
    assert(dim.width > 0 && dim.height > 0 && dim.depth > 0);
    assert(layers > 0 && levels > 0);

    YF_image img = calloc(1, sizeof(YF_image_o));
    if (img == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    img->ctx = ctx;
    img->wrapped = 1;
    img->image = image;
    img->memory = NULL;
    img->format = format;
    img->type = type;
    img->dim = dim;
    img->layers = layers;
    img->levels = levels;
    img->samples = samples;
    img->flags = 0;
    img->usage = usage;
    img->tiling = VK_IMAGE_TILING_OPTIMAL;
    img->layout = layout;
    img->next_layout = layout;
    img->data = NULL;

    img->iviews = yf_dict_init(hash_priv, cmp_priv);
    if (img->iviews == NULL) {
        free(img);
        return NULL;
    }

    YF_PIXFMT_TO(format, img->pixfmt);
    if (img->pixfmt == YF_PIXFMT_UNDEF) {
        yf_seterr(YF_ERR_INVARG, __func__);
        yf_image_deinit(img);
        return NULL;
    }

    YF_PIXFMT_ASPECT(img->pixfmt, img->aspect);

    switch (type) {
    case VK_IMAGE_TYPE_1D:
        img->view_type = layers > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY :
                                      VK_IMAGE_VIEW_TYPE_1D;
        break;
    case VK_IMAGE_TYPE_2D:
        img->view_type = layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY :
                                      VK_IMAGE_VIEW_TYPE_2D;
        break;
    case VK_IMAGE_TYPE_3D:
        if (layers > 1) {
            yf_seterr(YF_ERR_INVARG, __func__);
            yf_image_deinit(img);
            return NULL;
        }
        img->view_type = VK_IMAGE_VIEW_TYPE_3D;
        break;
    default:
        yf_seterr(YF_ERR_INVARG, __func__);
        yf_image_deinit(img);
        return NULL;
    }

    yf_setpub(img, YF_PUBSUB_DEINIT);

    return img;
}

int yf_image_getiview(YF_image img, YF_slice layers, YF_slice levels,
                      YF_iview *iview)
{
    assert(img != NULL);
    assert(layers.n > 0 && layers.i + layers.n <= img->layers);
    assert(levels.n > 0 && levels.i + levels.n <= img->levels);
    assert(iview != NULL);

    const T_priv priv = {{layers, levels}, 0};
    YF_iview *iv = yf_dict_search(img->iviews, &priv);

    if (iv == NULL) {
        iv = malloc(sizeof *iv);
        if (iv == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }

        iv->priv = malloc(sizeof priv);
        if (iv->priv == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            free(iv);
            return -1;
        }

        memcpy(iv->priv, &priv, sizeof priv);

        VkImageViewCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = img->image,
            .viewType = img->view_type,
            .format = img->format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_R,
                .g = VK_COMPONENT_SWIZZLE_G,
                .b = VK_COMPONENT_SWIZZLE_B,
                .a = VK_COMPONENT_SWIZZLE_A
            },
            .subresourceRange = {
                .aspectMask = img->aspect,
                .baseMipLevel = levels.i,
                .levelCount = levels.n,
                .baseArrayLayer = layers.i,
                .layerCount = layers.n
            }
        };

        VkResult res = vkCreateImageView(img->ctx->device, &info, NULL,
                                         &iv->view);
        if (res != VK_SUCCESS) {
            yf_seterr(YF_ERR_DEVGEN, __func__);
            free(iv->priv);
            free(iv);
            return -1;
        }

        if (yf_dict_insert(img->iviews, iv->priv, iv) != 0) {
            vkDestroyImageView(img->ctx->device, iv->view, NULL);
            free(iv->priv);
            free(iv);
            return -1;
        }
    }

    ((T_priv *)iv->priv)->count++;
    *iview = *iv;

    return 0;
}

void yf_image_ungetiview(YF_image img, YF_iview *iview)
{
    assert(img != NULL);
    assert(iview != NULL);

    YF_iview *iv = yf_dict_search(img->iviews, iview->priv);

    if (iv == NULL)
        return;

    T_priv *priv = iv->priv;

    if (--priv->count == 0) {
        vkDestroyImageView(img->ctx->device, iv->view, NULL);
        yf_dict_remove(img->iviews, iv->priv);
        free(iv->priv);
        free(iv);
    }
}

int yf_image_chglayout(YF_image img, VkImageLayout layout)
{
    assert(img != NULL);
    assert(layout != VK_IMAGE_LAYOUT_UNDEFINED &&
           layout != VK_IMAGE_LAYOUT_PREINITIALIZED);

    if (layout == img->next_layout)
        /* requested layout change ongoing */
        return 0;

    if (img->layout != img->next_layout) {
        /* different layout change pending */
        yf_seterr(YF_ERR_OTHER, __func__);
        return -1;
    }

    const YF_cmdres *cmdr = yf_cmdpool_getprio(img->ctx, set_layout, img);
    if (cmdr == NULL)
        return -1;

    img->next_layout = layout;

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT |
                         VK_ACCESS_MEMORY_WRITE_BIT,
        .oldLayout = img->layout,
        .newLayout = layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = img->image,
        .subresourceRange = {
            .aspectMask = img->aspect,
            .baseMipLevel = 0,
            .levelCount = img->levels,
            .baseArrayLayer = 0,
            .layerCount = img->layers
        }
    };

    vkCmdPipelineBarrier(cmdr->pool_res, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                         0, NULL, 0, NULL, 1, &barrier);

    return 0;
}
