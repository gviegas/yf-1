/*
 * YF
 * buffer.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_BUFFER_H
#define YF_BUFFER_H

#include "yf-buffer.h"
#include "vk.h"

typedef struct YF_buffer_o {
    YF_context ctx;
    VkBuffer buffer;
    VkDeviceMemory memory;
    size_t size;
    void *data;
} YF_buffer_o;

#endif /* YF_BUFFER_H */
