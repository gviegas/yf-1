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
#include "image.h"

#undef YF_MIN
#undef YF_MAX
#undef YF_CLAMP
#define YF_MIN(a, b) (a) < (b) ? (a) : (b)
#define YF_MAX(a, b) (a) > (b) ? (a) : (b)
#define YF_CLAMP(x, a, b) YF_MAX(a, YF_MIN(b, x))

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
  assert(wsi != NULL);
  assert(wsi->surface == VK_NULL_HANDLE);

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
  assert(wsi != NULL);
  assert(wsi->surface != VK_NULL_HANDLE);

  VkResult res;

  VkSurfaceCapabilitiesKHR capab;
  res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(wsi->ctx->phy_dev,
      wsi->surface, &capab);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  /* TODO: Make this configurable. */
  if (capab.maxImageCount == 0)
    wsi->img_n = YF_MAX(capab.minImageCount, 3);
  else
    wsi->img_n = capab.minImageCount;

  wsi->min_img_n = capab.minImageCount;

  VkCompositeAlphaFlagBitsKHR comp_alpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
  if (!(comp_alpha & capab.supportedCompositeAlpha)) {
    comp_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (!(comp_alpha & capab.supportedCompositeAlpha)) {
      yf_seterr(YF_ERR_UNSUP, __func__);
      return -1;
    }
  }

  unsigned width, height;
  yf_window_getsize(wsi->win, &width, &height);
  if (capab.currentExtent.width == 0xffffffff) {
    width = YF_CLAMP(width, capab.minImageExtent.width,
        capab.maxImageExtent.width);
    height = YF_CLAMP(height, capab.minImageExtent.height,
        capab.maxImageExtent.height);
  }

  VkSurfaceFormatKHR *fmts;
  unsigned fmt_n;
  res = vkGetPhysicalDeviceSurfaceFormatsKHR(wsi->ctx->phy_dev, wsi->surface,
      &fmt_n, NULL);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  fmts = malloc(fmt_n * sizeof *fmts);
  if (fmts == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  res = vkGetPhysicalDeviceSurfaceFormatsKHR(wsi->ctx->phy_dev, wsi->surface,
      &fmt_n, fmts);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    free(fmts);
    return -1;
  }

  const VkFormat pref_fmts[] = {
    VK_FORMAT_B8G8R8A8_SRGB,
    VK_FORMAT_B8G8R8A8_UNORM
  };
  int fmt_i = -1;
  for (size_t i = 0; i < (sizeof pref_fmts / sizeof pref_fmts[0]); ++i) {
    for (size_t j = 0; j < fmt_n; ++j) {
      if (pref_fmts[i] == fmts[j].format &&
          fmts[j].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        fmt_i = j;
        break;
      }
    }
  }
  if (fmt_i == -1) {
    for (size_t i = 0; i < fmt_n; ++i) {
      int pixfmt;
      YF_PIXFMT_TO(fmts[i].format, pixfmt);
      if (pixfmt != YF_PIXFMT_UNDEF) {
        fmt_i = i;
        break;
      }
    }
  }
  if (fmt_i == -1) {
    yf_seterr(YF_ERR_UNSUP, __func__);
    free(fmts);
    return -1;
  }

  VkPresentModeKHR pres_mode = VK_PRESENT_MODE_FIFO_KHR;

  unsigned queue_is[2];
  unsigned queue_i_n = 0;
  VkSharingMode shar_mode = VK_SHARING_MODE_EXCLUSIVE;
  if (wsi->ctx->graph_queue_i != wsi->ctx->pres_queue_i) {
    queue_is[0] = wsi->ctx->graph_queue_i;
    queue_is[1] = wsi->ctx->pres_queue_i;
    shar_mode = VK_SHARING_MODE_CONCURRENT;
    queue_i_n = 2;
  }

  wsi->sc_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  wsi->sc_info.pNext = NULL;
  wsi->sc_info.flags = 0;
  wsi->sc_info.surface = wsi->surface;
  wsi->sc_info.minImageCount = wsi->img_n;
  wsi->sc_info.imageFormat = fmts[fmt_i].format;
  wsi->sc_info.imageColorSpace = fmts[fmt_i].colorSpace;
  wsi->sc_info.imageExtent = (VkExtent2D){width, height};
  wsi->sc_info.imageArrayLayers = 1;
  wsi->sc_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  wsi->sc_info.imageSharingMode = shar_mode;
  wsi->sc_info.queueFamilyIndexCount = queue_i_n;
  wsi->sc_info.pQueueFamilyIndices = queue_is;
  wsi->sc_info.preTransform = capab.currentTransform;
  wsi->sc_info.compositeAlpha = comp_alpha;
  wsi->sc_info.presentMode = pres_mode;
  wsi->sc_info.clipped = VK_TRUE;
  wsi->sc_info.oldSwapchain = VK_NULL_HANDLE;

  free(fmts);
}

static int create_swapchain(YF_wsi wsi) {
  /* TODO */
}
