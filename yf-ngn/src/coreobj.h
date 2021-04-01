/*
 * YF
 * coreobj.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_COREOBJ_H
#define YF_COREOBJ_H

#include <yf/core/yf-context.h>
#include <yf/core/yf-pass.h>

/* Gets the shared context object. */
YF_context yf_getctx(void);

/* Gets the shared pass object. */
YF_pass yf_getpass(void);

#endif /* YF_COREOBJ_H */
