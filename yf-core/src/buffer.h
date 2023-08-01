/*
 * YF
 * buffer.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_BUFFER_H
#define YF_BUFFER_H

#include "yf-buffer.h"
#include "vk.h"

struct yf_buffer {
    yf_context_t *ctx;
    VkBuffer buffer;
    VkDeviceMemory memory;
    size_t size;
    void *data;
};

#endif /* YF_BUFFER_H */
