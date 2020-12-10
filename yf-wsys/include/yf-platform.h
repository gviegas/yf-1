/*
 * YF
 * yf-platform.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_PLATFORM_H
#define YF_YF_PLATFORM_H

#include <yf/com/yf-defs.h>

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

/* Types of platform. */
#define YF_PLATFORM_NONE    0
#define YF_PLATFORM_WAYLAND 1
#define YF_PLATFORM_XCB     2
#define YF_PLATFORM_WIN32   3
#define YF_PLATFORM_METAL   4
#define YF_PLATFORM_OTHER   32767

/* Gets the platform in use. */
int yf_getplatform(void);

typedef struct YF_window_o *YF_window;

#if defined(__linux__)
/* Gets the global xcb connection. */
xcb_connection_t *yf_getconnxcb(void);

/* Gets the global xcb visual ID. */
xcb_visualid_t yf_getvisualxcb(void);

/* Gets the xcb window ID associated with a given 'YF_window' object. */
xcb_window_t yf_getwindowxcb(YF_window win);

#else
/* TODO */
# error "Invalid platform"
#endif /* defined(__linux__) */

YF_DECLS_END

#endif /* YF_YF_PLATFORM_H */
