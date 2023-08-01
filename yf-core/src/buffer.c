/*
 * YF
 * buffer.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-pubsub.h"
#include "yf/com/yf-error.h"

#include "buffer.h"
#include "context.h"
#include "memory.h"
#include "yf-limits.h"

yf_buffer_t *yf_buffer_init(yf_context_t *ctx, size_t size)
{
    assert(ctx != NULL);
    assert(size > 0);

    if (size > yf_getlimits(ctx)->buffer.sz_max) {
        yf_seterr(YF_ERR_LIMIT, __func__);
        return NULL;
    }

    yf_buffer_t *buf = calloc(1, sizeof(yf_buffer_t));
    if (buf == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    buf->ctx = ctx;
    buf->size = size;

    VkFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    /*VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |*/
                    /*VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |*/
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT/* |
                    VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT*/;

    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL
    };

    VkResult res = vkCreateBuffer(ctx->device, &info, NULL, &buf->buffer);
    if (res != VK_SUCCESS) {
        yf_buffer_deinit(buf);
        yf_seterr(YF_ERR_DEVGEN, __func__);
        return NULL;
    }

    if (yf_buffer_alloc(buf) != 0) {
        yf_buffer_deinit(buf);
        return NULL;
    }

    yf_setpub(buf, YF_PUBSUB_DEINIT);

    return buf;
}

int yf_buffer_copy(yf_buffer_t *buf, size_t offset, const void *data,
                   size_t size)
{
    assert(buf != NULL);
    assert(data != NULL);
    assert(size > 0);

    if (offset + size > buf->size) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    memcpy((char *)buf->data+offset, data, size);

    return 0;
}

size_t yf_buffer_getsize(yf_buffer_t *buf)
{
    assert(buf != NULL);
    return buf->size;
}

void yf_buffer_deinit(yf_buffer_t *buf)
{
    if (buf == NULL)
        return;

    yf_publish(buf, YF_PUBSUB_DEINIT);
    yf_setpub(buf, YF_PUBSUB_NONE);

    yf_buffer_free(buf);
    vkDestroyBuffer(buf->ctx->device, buf->buffer, NULL);
    free(buf);
}
