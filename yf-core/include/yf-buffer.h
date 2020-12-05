/*
 * YF
 * yf-buffer.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_BUFFER_H
#define YF_YF_BUFFER_H

#include <stddef.h>

#include "yf-common.h"
#include "yf-context.h"

YF_DECLS_BEGIN

/* Opaque type defining unformatted linear data. */
typedef struct YF_buffer_o *YF_buffer;

/* Initializes a new buffer. */
YF_buffer yf_buffer_init(YF_context ctx, size_t size);

/* Copies local data to a buffer. */
int yf_buffer_copy(YF_buffer buf, size_t offset, const void *data, size_t size);

/* Gets values of a buffer. */
void yf_buffer_getval(YF_buffer buf, size_t *size);

/* Deinitializes a buffer. */
void yf_buffer_deinit(YF_buffer buf);

YF_DECLS_END

#endif /* YF_YF_BUFFER_H */
