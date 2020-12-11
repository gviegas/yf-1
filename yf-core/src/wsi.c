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

  if (ctx->pres_queue_i < 0) {
    yf_seterr(YF_ERR_UNSUP, __func__);
    return NULL;
  }

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

int yf_canpresent(VkPhysicalDevice phy_dev, int queue_i) {
  assert(phy_dev != NULL);
  assert(queue_i > -1);

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
      r = vkGetPhysicalDeviceXcbPresentationSupportKHR(phy_dev, queue_i,
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
# endif /* defined(VK_USE_PLATFORM_XCB_KHR) */
#elif defined(VK_USE_PLATFORM_XCB)
  /* xcb */
  if (plat == YF_PLATFORM_XCB) {
    r = vkGetPhysicalDeviceXcbPresentaionSupportKHR(phy_dev, queue_i,
        yf_getconnxcb(), yf_getvisualxcb());
  } else {
    yf_seterr(YF_ERR_OTHER, __func__);
    r = VK_FALSE;
  }
#else
  /* TODO: Check other platforms. */
  r = VK_FALSE;
#endif /* defined(VK_USE_PLATFORM_WAYLAND_KHR) */

  return r == VK_TRUE;
}

static int init_surface(YF_wsi wsi) {
  int plat = yf_getplatform();
  VkResult res;

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
      res = vkCreateXcbSurfaceKHR(wsi->ctx->instance, &info, NULL,
          &wsi->surface);
      if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        return -1;
      }
    } break;
    default:
      yf_seterr(YF_ERR_OTHER, __func__);
      return -1;
  }
# else
  /* wayland */
  if (plat == YF_PLATFORM_WAYLAND) {
    /* TODO */
    assert(0);
  } else {
    yf_seterr(YF_ERR_OTHER, __func__);
    return -1;
  }
# endif /* defined(VK_USE_PLATFORM_XCB_KHR) */
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
    if (res != VK_SUCCESS) {
      yf_seterr(YF_ERR_DEVGEN, __func__);
      return -1;
    }
  } else {
    yf_seterr(YF_ERR_OTHER, __func__);
    return -1;
  }
#else
  /* TODO: Other platforms. */
  yf_seterr(YF_ERR_OTHER, __func__);
  return -1;
#endif /* defined(VK_USE_PLATFORM_WAYLAND_KHR) */

  VkBool32 supported;

  res = vkGetPhysicalDeviceSurfaceSupportKHR(wsi->ctx->phy_dev,
      wsi->ctx->pres_queue_i, wsi->surface, &supported);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  return supported == VK_TRUE ? 0 : -1;
}

static int query_surface(YF_wsi wsi) {
  /* TODO */
}

static int create_swapchain(YF_wsi wsi) {
  /* TODO */
}
