/*
 * YF
 * yf-context.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_CONTEXT_H
#define YF_YF_CONTEXT_H

#include "yf/com/yf-defs.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining the execution context.
 *
 * This is the main object of yf-core, from which others are created.
 */
typedef struct yf_context yf_context_t;

/**
 * Initializes a new context.
 *
 * Multiple contexts are not supported.
 *
 * @return: On success, returns a new context. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_context_t *yf_context_init(void);

/**
 * Deinitializes a context.
 *
 * @param ctx: The context to deinitialize. Can be 'NULL'.
 */
void yf_context_deinit(yf_context_t *ctx);

YF_DECLS_END

#endif /* YF_YF_CONTEXT_H */
