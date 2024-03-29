/*
 * YF
 * yf-buffer.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_BUFFER_H
#define YF_YF_BUFFER_H

#include <stddef.h>

#include "yf/com/yf-defs.h"

#include "yf-context.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining unformatted linear data in device memory.
 */
typedef struct yf_buffer yf_buffer_t;

/**
 * Initializes a new buffer.
 *
 * @param size: The size of the buffer to allocate, in bytes.
 * @return: On success, returns a new buffer. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_buffer_t *yf_buffer_init(yf_context_t *ctx, size_t size);

/**
 * Copies local data to a buffer.
 *
 * @param buf: The buffer.
 * @param offset: The offset from the beginning of the buffer.
 * @param data: The data to copy.
 * @param size: The number of bytes to copy.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_buffer_copy(yf_buffer_t *buf, size_t offset, const void *data,
                   size_t size);

/**
 * Gets the size of a buffer.
 *
 * @param buf: The buffer.
 * @return: The buffer's size.
 */
size_t yf_buffer_getsize(yf_buffer_t *buf);

/**
 * Deinitializes a buffer.
 *
 * @param buf: The buffer to deinitialize. Can be 'NULL'.
 */
void yf_buffer_deinit(yf_buffer_t *buf);

YF_DECLS_END

#endif /* YF_YF_BUFFER_H */
