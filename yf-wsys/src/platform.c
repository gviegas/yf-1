/*
 * YF
 * platform.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf-platform.h"

#if defined(__linux__)
/*
# include "platform-wayland.h"
 */
# include "platform-xcb.h"
#else
/* TODO */
# error "Invalid platform"
#endif /* defined(__linux__) */

/* The platform in use. */
static int plat_ = YF_PLATFORM_NONE;

#if defined(__linux__)
int yf_getplatform(void)
{
    if (plat_ != YF_PLATFORM_NONE)
        return plat_;

    if (getenv("WAYLAND_DISPLAY") != NULL) {
        /* TODO: Wayland imp. */
        /*
        if (yf_loadwayland() == 0 && atexit(yf_unldwayland) == 0)
            plat_ = YF_PLATFORM_WAYLAND;
        else
            yf_unldwayland();
         */
        goto x11;
    } else if (getenv("DISPLAY") != NULL) {
x11:
        if (yf_loadxcb() == 0 && atexit(yf_unldxcb) == 0)
            plat_ = YF_PLATFORM_XCB;
        else
            yf_unldxcb();
    }

    return plat_;
}

void yf_getwinimp(YF_win_imp *imp)
{
    switch (yf_getplatform()) {
    case YF_PLATFORM_WAYLAND:
        /* TODO */
        assert(0);
    case YF_PLATFORM_XCB:
        *imp = yf_g_winxcb;
        break;
    default:
        assert(0);
    }
}

void yf_getevtimp(YF_evt_imp *imp)
{
    switch (yf_getplatform()) {
    case YF_PLATFORM_WAYLAND:
        /* TODO */
        assert(0);
    case YF_PLATFORM_XCB:
        *imp = yf_g_evtxcb;
        break;
    default:
        assert(0);
    }
}
#else
/* TODO */
# error "Invalid platform"
#endif
