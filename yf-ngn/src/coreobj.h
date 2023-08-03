/*
 * YF
 * coreobj.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_COREOBJ_H
#define YF_COREOBJ_H

#include "yf/core/yf-context.h"
#include "yf/core/yf-pass.h"

/* Gets the shared context object. */
yf_context_t *yf_getctx(void);

/* Gets the shared pass object. */
/* TODO: Remove. */
yf_pass_t *yf_getpass(void);

#endif /* YF_COREOBJ_H */
