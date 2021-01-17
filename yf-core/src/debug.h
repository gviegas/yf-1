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
# include "limits.h"

void yf_debug_ctx(YF_context ctx);
void yf_debug_lim(const YF_limits *lim);

#endif /* defined(YF_DEBUG) */

#endif /* YF_DEBUG_H */
