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
    assert(res == 0);
    yf_buffer_deinit((YF_buffer)arg);
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

    return (pv1->key.layers.i != pv2->key.layers.i) ||
           (pv1->key.layers.n != pv2->key.layers.n) ||
           (pv1->key.levels.i != pv2->key.levels.i) ||
           (pv1->key.levels.n != pv2->key.levels.n);
}

YF_image yf_image_init(YF_context ctx, int pixfmt, YF_dim3 dim,
                       unsigned layers, unsigned levels, unsigned samples)
{
    assert(ctx != NULL);
    assert(dim.width > 0 && dim.height > 0 && dim.depth > 0);
    assert(layers > 0 && levels > 0 && samples > 0);

    const YF_limits *lim = yf_getlimits(ctx);

    if (layers > lim->image.layer_max) {
        yf_seterr(YF_ERR_LIMIT, __func__);
        return NULL;
    }

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
    VkFlags flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

    if (dim.depth > 1) {
        if (layers != 1) {
            yf_image_deinit(img);
            yf_seterr(YF_ERR_INVARG, __func__);
            return NULL;
        }

        img->type = VK_IMAGE_TYPE_3D;
        img->view_type = VK_IMAGE_VIEW_TYPE_3D;

        if (ctx->dev_prop.apiVersion >= VK_API_VERSION_1_1)
            flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;

    } else if (dim.height > 1) {
        img->type = VK_IMAGE_TYPE_2D;

        if (layers > 1)
            img->view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        else
            img->view_type = VK_IMAGE_VIEW_TYPE_2D;

        if (dim.width == dim.height && dim.width >= 6)
            flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    } else {
        img->type = VK_IMAGE_TYPE_1D;

        if (layers > 1)
            img->view_type = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        else
            img->view_type = VK_IMAGE_VIEW_TYPE_1D;
    }

    int limit = 0;

    switch (img->type) {
    case VK_IMAGE_TYPE_1D:
        limit = dim.width > lim->image.dim_1d_max;
        break;
    case VK_IMAGE_TYPE_2D:
        /* TODO: Check limit for cube-compatible images. */
        limit =
            dim.width > lim->image.dim_2d_max ||
            dim.height > lim->image.dim_2d_max;
        break;
    case VK_IMAGE_TYPE_3D:
        limit =
            dim.width > lim->image.dim_3d_max ||
            dim.height > lim->image.dim_3d_max ||
            dim.depth > lim->image.dim_3d_max;
        break;
    default:
        break;
    }

    if (limit) {
        yf_seterr(YF_ERR_LIMIT, __func__);
        yf_image_deinit(img);
        return NULL;
    }

    /* TODO: Store format properties somewhere and use it to validate usage. */
    VkFlags usage = 0;
    VkFormatFeatureFlags fmt_feat;
    VkFormatProperties fmt_prop;
    vkGetPhysicalDeviceFormatProperties(ctx->phy_dev, img->format, &fmt_prop);
    fmt_feat = fmt_prop.optimalTilingFeatures;

    if (fmt_feat & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (fmt_feat & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
        usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (fmt_feat & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (fmt_feat & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    /* XXX: This may prevent writes to image with optimal tiling. */
    if (ctx->dev_prop.apiVersion >= VK_API_VERSION_1_1) {
        if (fmt_feat & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT)
            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if (fmt_feat & VK_FORMAT_FEATURE_TRANSFER_DST_BIT)
            usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    } else {
        /* XXX: Not in v1.0, assume its valid. */
        usage |=
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    /* TODO: Further validation. */
    if (usage == 0) {
        yf_seterr(YF_ERR_UNSUP, __func__);
        return NULL;
    }

    VkImageCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = flags,
        .imageType = img->type,
        .format = img->format,
        .extent = {img->dim.width, img->dim.height, img->dim.depth},
        .mipLevels = img->levels,
        .arrayLayers = img->layers,
        .samples = img->samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkResult res = vkCreateImage(ctx->device, &info, NULL, &img->image);

    if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        yf_image_deinit(img);
        return NULL;
    }

    img->layout = info.initialLayout;

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

    if (layer > img->layers || level > img->levels ||
        off.x + dim.width > img->dim.width ||
        off.y + dim.height > img->dim.height ||
        off.z + dim.depth > img->dim.depth) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    size_t sz;
    YF_PIXFMT_SIZEOF(img->pixfmt, sz);
    sz *= dim.width * dim.height * dim.depth;
    YF_buffer stg_buf = yf_buffer_init(img->ctx, sz);

    if (stg_buf == NULL)
        return -1;

    memcpy(stg_buf->data, data, sz);

    const YF_cmdres *cmdr;
    cmdr = yf_cmdpool_getprio(img->ctx, dealloc_stgbuf, stg_buf);

    if (cmdr == NULL) {
        yf_buffer_deinit(stg_buf);
        return -1;
    }

    if (img->layout != VK_IMAGE_LAYOUT_GENERAL)
        yf_image_transition(img, cmdr->pool_res);

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
                           VK_IMAGE_LAYOUT_GENERAL, 1, &region);

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
                       VkImageLayout layout)
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
    img->samples = samples;
    img->dim = dim;
    img->layers = layers;
    img->levels = levels;
    img->layout = layout;

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
        img->view_type =
            layers > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
        break;
    case VK_IMAGE_TYPE_2D:
        img->view_type =
            layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
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

/* TODO: Provide more parameters for this function. */
void yf_image_transition(YF_image img, VkCommandBuffer cbuffer)
{
    assert(img != NULL);
    assert(cbuffer != NULL);

    img->layout = VK_IMAGE_LAYOUT_GENERAL;

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
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

    vkCmdPipelineBarrier(cbuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                         0, NULL, 0, NULL, 1, &barrier);
}
