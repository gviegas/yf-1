/*
 * YF
 * print.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_PRINT_H
#define YF_PRINT_H

#include "context.h"
#include "yf-limits.h"

void yf_print_ctx(yf_context_t *ctx);
void yf_print_lim(const yf_limits_t *lim);
void yf_print_capab(void);

#endif /* YF_PRINT_H */
