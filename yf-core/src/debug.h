/*
 * YF
 * debug.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_DEBUG_H
#define YF_DEBUG_H

#if defined(YF_DEBUG)
# include "context.h"
void yf_debug_ctx(YF_context ctx);
#endif /* defined(YF_DEBUG) */

#endif /* YF_DEBUG_H */
