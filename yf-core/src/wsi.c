/*
 * YF
 * wsi.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include <yf/com/yf-error.h>
#include <yf/wsys/yf-platform.h>

#include "wsi.h"

YF_wsi yf_wsi_init(YF_context ctx, YF_window win);

const YF_image *yf_wsi_getimages(YF_wsi wsi, unsigned *n) {
  /* TODO */
}

unsigned yf_wsi_getlimit(YF_wsi wsi) {
  /* TODO */
}

int yf_wsi_getindex(YF_wsi wsi, int nonblocking) {
  /* TODO */
}

int yf_wsi_present(YF_wsi wsi, unsigned index) {
  /* TODO */
}

void yf_wsi_deinit(void) {
  /* TODO */
}

int yf_canpresent(VkPhysicalDevice phy_dev, int family) {
  assert(phy_dev != VK_NULL_HANDLE);
  assert(family > -1);

  int plat = yf_getplatform();
  VkBool32 r;

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
# if defined(VK_USE_PLATFORM_XCB_KHR)
  /* either wayland or xcb */
  switch (plat) {
    case YF_PLATFORM_WAYLAND:
      /* TODO */
      assert(0);
    case YF_PLATFORM_XCB:
      r = vkGetPhysicalDeviceXcbPresentationSupportKHR(phy_dev, family,
          yf_getconnxcb(), yf_getvisualxcb());
      break;
    default:
      yf_seterr(YF_ERR_OTHER, __func__);
      r = VK_FALSE;
  }
# else
  /* wayland */
  if (plat == YF_PLATFORM_WAYLAND) {
    /* TODO */
    assert(0);
  } else {
    yf_seterr(YF_ERR_OTHER, __func__);
    r = VK_FALSE;
  }
# endif /* defined(VK_USE_PLATFORM_XCB) */
#elif defined(VK_USE_PLATFORM_XCB)
  /* xcb */
  r = vkGetPhysicalDeviceXcbPresentaionSupportKHR(phy_dev, family,
      yf_getconnxcb(), yf_getvisualxcb());
#else
/* TODO: Check other platforms. */
  r = VK_FALSE;
#endif /* defined(VK_USE_PLATFORM_WAYLAND) */

  return r == VK_TRUE;
}
