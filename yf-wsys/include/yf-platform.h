/*
 * YF
 * yf-platform.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_PLATFORM_H
#define YF_YF_PLATFORM_H

#include "yf/com/yf-defs.h"

#if defined(__linux__)
/*
# include <wayland-client-core.h>
 */
# include <xcb/xcb.h>
#else
/* TODO */
# error "Invalid platform"
#endif /* defined(__linux__) */

YF_DECLS_BEGIN

/**
 * Types of platform.
 */
#define YF_PLATFORM_NONE    0
#define YF_PLATFORM_WAYLAND 1
#define YF_PLATFORM_XCB     2
#define YF_PLATFORM_WIN32   3
#define YF_PLATFORM_METAL   4
#define YF_PLATFORM_OTHER   32767

/**
 * Gets the platform in use.
 *
 * If the current platform is 'YF_PLATFORM_NONE' at the time this function is
 * called, an attempt will be made to initialize an underlying implementation.
 * Thus, a return value of 'YF_PLATFORM_NONE' can be interpreted as failure
 * to find a suitable platform.
 *
 * @return: A 'YF_PLATFORM' value indicating the platform in use.
 */
int yf_getplatform(void);

typedef struct yf_window yf_window_t;

#if defined(__linux__)
/**
 * Gets the global xcb connection.
 *
 * @return: The connection.
 */
xcb_connection_t *yf_getconnxcb(void);

/**
 * Gets the global xcb visual ID.
 *
 * @return: The visual ID.
 */
xcb_visualid_t yf_getvisualxcb(void);

/**
 * Gets the xcb window ID associated with a given 'yf_window_t'.
 *
 * @param win: The window to query.
 * @return: The window ID.
 */
xcb_window_t yf_getwindowxcb(yf_window_t *win);

#else
/* TODO */
# error "Invalid platform"
#endif /* defined(__linux__) */

YF_DECLS_END

#endif /* YF_YF_PLATFORM_H */
