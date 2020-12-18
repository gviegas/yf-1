/*
 * YF
 * yf-buffer.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_BUFFER_H
#define YF_YF_BUFFER_H

#include <stddef.h>

#include <yf/com/yf-defs.h>

#include "yf-context.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining unformatted linear data.
 */
typedef struct YF_buffer_o *YF_buffer;

/**
 * Initializes a new buffer.
 *
 * @param size: The size of the buffer to allocate.
 * @return: On success, returns a new buffer. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_buffer yf_buffer_init(YF_context ctx, size_t size);

/**
 * Copies local data to a buffer.
 *
 * @param buf: The buffer.
 * @param offset: The offset from the beginning of the buffer.
 * @param data: The data to copy.
 * @param size: The number of bytes to copy.
 * @return: On success, returns '0'. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_buffer_copy(YF_buffer buf, size_t offset, const void *data, size_t size);

/**
 * Gets values of a buffer.
 *
 * @param buf: The buffer.
 * @param size: The destination for the size value.
 */
void yf_buffer_getval(YF_buffer buf, size_t *size);

/**
 * Deinitializes a buffer.
 *
 * @param buf: The buffer to deinitialize. Can be 'NULL'.
 */
void yf_buffer_deinit(YF_buffer buf);

YF_DECLS_END

#endif /* YF_YF_BUFFER_H */
