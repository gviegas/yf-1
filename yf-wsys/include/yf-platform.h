/*
 * YF
 * yf-platform.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_PLATFORM_H
#define YF_YF_PLATFORM_H

#include <yf/com/yf-defs.h>

YF_DECLS_BEGIN

/* Types of platform. */
#define YF_PLATFORM_NONE    0
#define YF_PLATFORM_WAYLAND 1
#define YF_PLATFORM_XCB     2
#define YF_PLATFORM_WIN32   3
#define YF_PLATFORM_METAL   4
#define YF_PLATFORM_OTHER   5

/* Gets the platform in use. */
int yf_getplatform(void);

YF_DECLS_END

#endif /* YF_YF_PLATFORM_H */
