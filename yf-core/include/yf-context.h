/*
 * YF
 * yf-context.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_CONTEXT_HH
#define YF_YF_CONTEXT_HH

#include "yf-common.h"

YF_DECLS_BEGIN

/* Opaque type defining the execution context. */
typedef struct YF_context_o *YF_context;

/* Initializes a new context. */
YF_context yf_context_init(void);

/* Deinitializes a context. */
void yf_context_deinit(YF_context ctx);

YF_DECLS_END

#endif /* YF_YF_CONTEXT_H */
