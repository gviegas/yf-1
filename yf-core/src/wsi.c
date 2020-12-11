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
#include "context.h"

/* Initializes surface. */
static int init_surface(YF_wsi wsi);

/* Queries surface. */
static int query_surface(YF_wsi wsi);

/* Creates swapchain. */
static int create_swapchain(YF_wsi wsi);

YF_wsi yf_wsi_init(YF_context ctx, YF_window win) {
  assert(ctx != NULL);
  assert(win != NULL);

  /* TODO */

  return NULL;
}

const YF_image *yf_wsi_getimages(YF_wsi wsi, unsigned *n) {
  assert(wsi != NULL);
  assert(n != NULL);

  /* TODO */

  return NULL;
}

unsigned yf_wsi_getlimit(YF_wsi wsi) {
  assert(wsi != NULL);

  /* TODO */

  return 0;
}

int yf_wsi_getindex(YF_wsi wsi, int nonblocking) {
  assert(wsi != NULL);

  /* TODO */

  return -1;
}

int yf_wsi_present(YF_wsi wsi, unsigned index) {
  assert(wsi != NULL);

  if (index >= wsi->img_n) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  /* TODO */

  return -1;
}

void yf_wsi_deinit(YF_wsi wsi) {
  if (wsi != NULL) {
    /* TODO */
    free(wsi);
  }
}

int yf_canpresent(VkPhysicalDevice phy_dev, int family) {
  assert(phy_dev != NULL);
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
  if (plat == YF_PLATFORM_XCB) {
    r = vkGetPhysicalDeviceXcbPresentaionSupportKHR(phy_dev, family,
        yf_getconnxcb(), yf_getvisualxcb());
  } else {
    yf_seterr(YF_ERR_OTHER, __func__);
    r = VK_FALSE;
  }
#else
  /* TODO: Check other platforms. */
  r = VK_FALSE;
#endif /* defined(VK_USE_PLATFORM_WAYLAND) */

  return r == VK_TRUE;
}

static int init_surface(YF_wsi wsi) {
  int plat = yf_getplatform();
  VkInstance inst = wsi->ctx->instance;
  VkResult res = ~VK_SUCCESS;

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
# if defined(VK_USE_PLATFORM_XCB_KHR)
  /* either wayland or xcb */
  switch (plat) {
    case YF_PLATFORM_WAYLAND:
      /* TODO */
      assert(0);
    case YF_PLATFORM_XCB: {
      VkXcbSurfaceCreateInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .connection = yf_getconnxcb(),
        .window = yf_getwindowxcb(wsi->win),
      };
      res = vkCreateXcbSurfaceKHR(inst, &info, NULL, &wsi->surface);
      if (res != VK_SUCCESS)
        yf_seterr(YF_ERR_DEVGEN, __func__);
    } break;
    default:
      yf_seterr(YF_ERR_OTHER, __func__);
      res = ~VK_SUCCESS;
  }
# else
  /* wayland */
  if (plat == YF_PLATFORM_WAYLAND) {
    /* TODO */
    assert(0);
  } else {
    yf_seterr(YF_ERR_OTHER, __func__);
    res = ~VK_SUCCESS;
  }
# endif /* defined(VK_USE_PLATFORM_XCB) */
#elif defined(VK_USE_PLATFORM_XCB)
  /* xcb */
  if (plat == YF_PLATFORM_XCB) {
    VkXcbSurfaceCreateInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
      .pNext = NULL,
      .flags = 0,
      .connection = yf_getconnxcb(),
      .window = yf_getwindowxcb(wsi->win),
    };
    res = vkCreateXcbSurfaceKHR(inst, &info, NULL, &wsi->surface);
    if (res != VK_SUCCESS)
      yf_seterr(YF_ERR_DEVGEN, __func__);
  } else {
    yf_seterr(YF_ERR_OTHER, __func__);
    res = ~VK_SUCCESS;
  }
#else
  /* TODO: Other platforms. */
  yf_seterr(YF_ERR_OTHER, __func__);
  res = ~VK_SUCCESS;
#endif

  return res == VK_SUCCESS ? 0 : -1;
}

static int query_surface(YF_wsi wsi) {
  /* TODO */
}

static int create_swapchain(YF_wsi wsi) {
  /* TODO */
}
